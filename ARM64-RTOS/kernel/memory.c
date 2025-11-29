/*
 * Gracemont Industrial Control Framework - ARM64 RTOS
 * Copyright (C) 2024 Zixiao System <https://github.com/Zixiao-System>
 *
 * This file is part of Gracemont Industrial Control Framework.
 * Repository: https://github.com/Zixiao-System/gracemont
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * ARM64 RTOS Kernel - Memory Management
 */

#include "rtos_types.h"
#include "rtos_config.h"

/* Heap Memory Block Header */
typedef struct heap_block {
    size_t              size;           /* Block size including header */
    bool                used;           /* Allocation status */
    struct heap_block   *next;          /* Next block in free list */
    struct heap_block   *prev;          /* Previous block in free list */
} heap_block_t;

/* Memory Manager State */
static uint8_t heap_memory[CONFIG_HEAP_SIZE] ALIGNED(16) SECTION(".heap");
static heap_block_t *heap_free_list = NULL;
static spinlock_t heap_lock = SPINLOCK_INIT;

/* DMA Pool for Zero-Copy Buffers */
static uint8_t dma_pool[CONFIG_DMA_POOL_SIZE] ALIGNED(4096) SECTION(".dma");
static mempool_t dma_mempool;

#define HEAP_BLOCK_ALIGN    16
#define HEAP_MIN_SIZE       (sizeof(heap_block_t) + HEAP_BLOCK_ALIGN)

/*
 * Heap Initialization
 */
void heap_init(void)
{
    heap_block_t *block = (heap_block_t *)heap_memory;
    block->size = CONFIG_HEAP_SIZE;
    block->used = false;
    block->next = NULL;
    block->prev = NULL;
    heap_free_list = block;
}

/*
 * Heap Allocation (First-Fit)
 */
void *heap_alloc(size_t size)
{
    if (size == 0) {
        return NULL;
    }

    /* Align size */
    size = (size + sizeof(heap_block_t) + HEAP_BLOCK_ALIGN - 1) & ~(HEAP_BLOCK_ALIGN - 1);
    if (size < HEAP_MIN_SIZE) {
        size = HEAP_MIN_SIZE;
    }

    spin_lock_irq(&heap_lock);

    heap_block_t *block = heap_free_list;
    while (block != NULL) {
        if (!block->used && block->size >= size) {
            /* Found suitable block */
            if (block->size >= size + HEAP_MIN_SIZE) {
                /* Split block */
                heap_block_t *new_block = (heap_block_t *)((uint8_t *)block + size);
                new_block->size = block->size - size;
                new_block->used = false;
                new_block->next = block->next;
                new_block->prev = block;

                if (block->next != NULL) {
                    block->next->prev = new_block;
                }
                block->next = new_block;
                block->size = size;
            }

            block->used = true;

            /* Remove from free list */
            if (block->prev != NULL) {
                block->prev->next = block->next;
            } else {
                heap_free_list = block->next;
            }
            if (block->next != NULL) {
                block->next->prev = block->prev;
            }

            spin_unlock_irq(&heap_lock);
            return (void *)((uint8_t *)block + sizeof(heap_block_t));
        }
        block = block->next;
    }

    spin_unlock_irq(&heap_lock);
    return NULL;
}

/*
 * Heap Free with Coalescing
 */
void heap_free(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));

    spin_lock_irq(&heap_lock);

    block->used = false;

    /* Find position in free list (sorted by address) */
    heap_block_t *prev_free = NULL;
    heap_block_t *next_free = heap_free_list;

    while (next_free != NULL && next_free < block) {
        prev_free = next_free;
        next_free = next_free->next;
    }

    /* Insert into free list */
    block->prev = prev_free;
    block->next = next_free;
    if (prev_free != NULL) {
        prev_free->next = block;
    } else {
        heap_free_list = block;
    }
    if (next_free != NULL) {
        next_free->prev = block;
    }

    /* Coalesce with next block */
    if (next_free != NULL &&
        (uint8_t *)block + block->size == (uint8_t *)next_free) {
        block->size += next_free->size;
        block->next = next_free->next;
        if (next_free->next != NULL) {
            next_free->next->prev = block;
        }
    }

    /* Coalesce with previous block */
    if (prev_free != NULL &&
        (uint8_t *)prev_free + prev_free->size == (uint8_t *)block) {
        prev_free->size += block->size;
        prev_free->next = block->next;
        if (block->next != NULL) {
            block->next->prev = prev_free;
        }
    }

    spin_unlock_irq(&heap_lock);
}

/*
 * Aligned Allocation
 */
void *heap_alloc_aligned(size_t size, size_t alignment)
{
    if (alignment < HEAP_BLOCK_ALIGN) {
        alignment = HEAP_BLOCK_ALIGN;
    }

    /* Allocate extra space for alignment */
    size_t total = size + alignment + sizeof(void *);
    void *raw = heap_alloc(total);
    if (raw == NULL) {
        return NULL;
    }

    /* Calculate aligned address */
    uintptr_t aligned = ((uintptr_t)raw + sizeof(void *) + alignment - 1) & ~(alignment - 1);

    /* Store original pointer before aligned address */
    ((void **)aligned)[-1] = raw;

    return (void *)aligned;
}

void heap_free_aligned(void *ptr)
{
    if (ptr == NULL) {
        return;
    }
    void *raw = ((void **)ptr)[-1];
    heap_free(raw);
}

/*
 * Memory Pool Initialization
 */
status_t mempool_init(mempool_t *pool, void *base, size_t block_size, size_t block_count)
{
    if (pool == NULL || base == NULL || block_size == 0 || block_count == 0) {
        return STATUS_INVALID;
    }

    /* Align block size */
    block_size = (block_size + 15) & ~15;

    pool->base = base;
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->lock = (spinlock_t)SPINLOCK_INIT;

    /* Initialize free list */
    pool->free_list = base;
    uint8_t *block = (uint8_t *)base;
    for (size_t i = 0; i < block_count - 1; i++) {
        *(void **)block = block + block_size;
        block += block_size;
    }
    *(void **)block = NULL;

    return STATUS_OK;
}

/*
 * Memory Pool Allocation
 */
void *mempool_alloc(mempool_t *pool)
{
    spin_lock_irq(&pool->lock);

    void *block = (void *)pool->free_list;
    if (block != NULL) {
        pool->free_list = *(void **)block;
    }

    spin_unlock_irq(&pool->lock);
    return block;
}

/*
 * Memory Pool Free
 */
void mempool_free(mempool_t *pool, void *block)
{
    if (block == NULL) {
        return;
    }

    spin_lock_irq(&pool->lock);
    *(void **)block = (void *)pool->free_list;
    pool->free_list = block;
    spin_unlock_irq(&pool->lock);
}

/*
 * DMA Pool Management
 */
void dma_pool_init(void)
{
    mempool_init(&dma_mempool, dma_pool, 4096, CONFIG_DMA_POOL_SIZE / 4096);
}

void *dma_alloc(size_t size)
{
    /* For simplicity, allocate in 4K pages */
    size_t pages = (size + 4095) / 4096;
    if (pages > 1) {
        /* For large allocations, use heap with alignment */
        return heap_alloc_aligned(size, 4096);
    }
    return mempool_alloc(&dma_mempool);
}

void dma_free(void *ptr, size_t size)
{
    size_t pages = (size + 4095) / 4096;
    if (pages > 1) {
        heap_free_aligned(ptr);
    } else {
        mempool_free(&dma_mempool, ptr);
    }
}

/*
 * Memory Statistics
 */
typedef struct {
    size_t total_size;
    size_t used_size;
    size_t free_size;
    size_t largest_free;
    size_t block_count;
} heap_stats_t;

void heap_get_stats(heap_stats_t *stats)
{
    if (stats == NULL) {
        return;
    }

    spin_lock_irq(&heap_lock);

    stats->total_size = CONFIG_HEAP_SIZE;
    stats->used_size = 0;
    stats->free_size = 0;
    stats->largest_free = 0;
    stats->block_count = 0;

    heap_block_t *block = (heap_block_t *)heap_memory;
    uint8_t *end = heap_memory + CONFIG_HEAP_SIZE;

    while ((uint8_t *)block < end) {
        stats->block_count++;
        if (block->used) {
            stats->used_size += block->size;
        } else {
            stats->free_size += block->size;
            if (block->size > stats->largest_free) {
                stats->largest_free = block->size;
            }
        }
        block = (heap_block_t *)((uint8_t *)block + block->size);
    }

    spin_unlock_irq(&heap_lock);
}
