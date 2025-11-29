/*
 * Gracemont Industrial Control Framework
 * Zero-Copy Network Buffer Implementation
 */

#include "zbuf.h"

/* Global buffer pool */
zbuf_pool_t zbuf_pool;

/* Pool memory */
static uint8_t zbuf_memory[CONFIG_ZBUF_POOL_SIZE] ALIGNED(64) SECTION(".zbuf");

/*
 * Pool Initialization
 */
status_t zbuf_pool_init(void)
{
    zbuf_pool.lock = (spinlock_t)SPINLOCK_INIT;
    zbuf_pool.free_list = NULL;
    zbuf_pool.total_count = CONFIG_ZBUF_COUNT;
    zbuf_pool.free_count = CONFIG_ZBUF_COUNT;
    zbuf_pool.alloc_failures = 0;
    zbuf_pool.pool_memory = zbuf_memory;
    zbuf_pool.buf_size = CONFIG_ZBUF_SIZE;

    /* Calculate actual buffer size including header */
    size_t buf_total = sizeof(zbuf_t) + CONFIG_ZBUF_SIZE;
    buf_total = (buf_total + 63) & ~63;  /* 64-byte align */

    /* Initialize free list */
    uint8_t *ptr = zbuf_memory;
    for (uint32_t i = 0; i < CONFIG_ZBUF_COUNT; i++) {
        zbuf_t *zb = (zbuf_t *)ptr;

        /* Initialize buffer */
        zb->head = (uint8_t *)(zb + 1);
        zb->end = zb->head + CONFIG_ZBUF_SIZE;
        zb->data = zb->head + CONFIG_ZBUF_HEADROOM;
        zb->tail = zb->data;
        zb->len = 0;
        zb->size = CONFIG_ZBUF_SIZE;
        zb->refcount = 0;
        zb->flags = ZBUF_F_DMA;
        zb->protocol = 0;
        zb->netif = NULL;
        zb->dma_addr = (addr_t)zb->head;  /* Identity mapping */
        zb->timestamp = 0;

        /* Add to free list */
        zb->next = zbuf_pool.free_list;
        zb->prev = NULL;
        if (zbuf_pool.free_list != NULL) {
            zbuf_pool.free_list->prev = zb;
        }
        zbuf_pool.free_list = zb;

        ptr += buf_total;
    }

    return STATUS_OK;
}

/*
 * Pool Statistics
 */
void zbuf_pool_stats(uint32_t *total, uint32_t *free, uint32_t *failures)
{
    spin_lock_irq(&zbuf_pool.lock);
    if (total) *total = zbuf_pool.total_count;
    if (free) *free = zbuf_pool.free_count;
    if (failures) *failures = zbuf_pool.alloc_failures;
    spin_unlock_irq(&zbuf_pool.lock);
}

/*
 * Buffer Allocation
 */
zbuf_t *zbuf_alloc(uint16_t size)
{
    if (size > CONFIG_ZBUF_SIZE - CONFIG_ZBUF_HEADROOM) {
        return NULL;
    }

    spin_lock_irq(&zbuf_pool.lock);

    zbuf_t *zb = zbuf_pool.free_list;
    if (zb == NULL) {
        zbuf_pool.alloc_failures++;
        spin_unlock_irq(&zbuf_pool.lock);
        return NULL;
    }

    /* Remove from free list */
    zbuf_pool.free_list = zb->next;
    if (zbuf_pool.free_list != NULL) {
        zbuf_pool.free_list->prev = NULL;
    }
    zbuf_pool.free_count--;

    spin_unlock_irq(&zbuf_pool.lock);

    /* Reset buffer state */
    zb->data = zb->head + CONFIG_ZBUF_HEADROOM;
    zb->tail = zb->data;
    zb->len = 0;
    zb->refcount = 1;
    zb->flags = ZBUF_F_DMA;
    zb->protocol = 0;
    zb->l2_offset = 0;
    zb->l3_offset = 0;
    zb->l4_offset = 0;
    zb->netif = NULL;
    zb->hash = 0;
    zb->timestamp = 0;
    zb->next = NULL;
    zb->prev = NULL;

    return zb;
}

zbuf_t *zbuf_alloc_tx(uint16_t size)
{
    zbuf_t *zb = zbuf_alloc(size);
    if (zb != NULL) {
        zb->flags |= ZBUF_F_TX;
    }
    return zb;
}

zbuf_t *zbuf_alloc_rx(uint16_t size)
{
    zbuf_t *zb = zbuf_alloc(size);
    if (zb != NULL) {
        zb->flags |= ZBUF_F_RX;
        /* For RX, start at head to allow header room */
        zb->data = zb->head;
        zb->tail = zb->data;
    }
    return zb;
}

/*
 * Buffer Free
 */
void zbuf_free(zbuf_t *zb)
{
    if (zb == NULL) {
        return;
    }

    /* Check if shared */
    if (zb->flags & ZBUF_F_SHARED) {
        return;
    }

    /* Decrement refcount */
    if (atomic_sub((volatile uint32_t *)&zb->refcount, 1) > 1) {
        return;
    }

    /* Return to pool */
    spin_lock_irq(&zbuf_pool.lock);

    zb->next = zbuf_pool.free_list;
    zb->prev = NULL;
    if (zbuf_pool.free_list != NULL) {
        zbuf_pool.free_list->prev = zb;
    }
    zbuf_pool.free_list = zb;
    zbuf_pool.free_count++;

    spin_unlock_irq(&zbuf_pool.lock);
}

/*
 * Reference Counting
 */
zbuf_t *zbuf_ref(zbuf_t *zb)
{
    if (zb != NULL) {
        atomic_add((volatile uint32_t *)&zb->refcount, 1);
    }
    return zb;
}

void zbuf_unref(zbuf_t *zb)
{
    zbuf_free(zb);
}

zbuf_t *zbuf_clone(zbuf_t *zb)
{
    if (zb == NULL) {
        return NULL;
    }

    zbuf_t *clone = zbuf_alloc(zb->len);
    if (clone == NULL) {
        return NULL;
    }

    /* Copy data */
    uint8_t *src = zb->data;
    uint8_t *dst = clone->data;
    for (uint16_t i = 0; i < zb->len; i++) {
        dst[i] = src[i];
    }
    clone->tail = clone->data + zb->len;
    clone->len = zb->len;
    clone->flags = (zb->flags & ~ZBUF_F_CLONED) | ZBUF_F_CLONED;
    clone->protocol = zb->protocol;
    clone->l2_offset = zb->l2_offset;
    clone->l3_offset = zb->l3_offset;
    clone->l4_offset = zb->l4_offset;
    clone->netif = zb->netif;
    clone->hash = zb->hash;
    clone->timestamp = zb->timestamp;

    return clone;
}

/*
 * Data Manipulation - Push header at front
 */
uint8_t *zbuf_push(zbuf_t *zb, uint16_t len)
{
    if (zb->data - len < zb->head) {
        return NULL;  /* No headroom */
    }
    zb->data -= len;
    zb->len += len;
    return zb->data;
}

/*
 * Data Manipulation - Pull header from front
 */
uint8_t *zbuf_pull(zbuf_t *zb, uint16_t len)
{
    if (len > zb->len) {
        return NULL;  /* Not enough data */
    }
    zb->data += len;
    zb->len -= len;
    return zb->data;
}

/*
 * Data Manipulation - Add data at end
 */
uint8_t *zbuf_put(zbuf_t *zb, uint16_t len)
{
    if (zb->tail + len > zb->end) {
        return NULL;  /* No tailroom */
    }
    uint8_t *ptr = zb->tail;
    zb->tail += len;
    zb->len += len;
    return ptr;
}

/*
 * Data Manipulation - Remove data from end
 */
void zbuf_trim(zbuf_t *zb, uint16_t len)
{
    if (len > zb->len) {
        len = zb->len;
    }
    zb->tail -= len;
    zb->len -= len;
}

/*
 * Reserve headroom
 */
void zbuf_reserve(zbuf_t *zb, uint16_t len)
{
    zb->data += len;
    zb->tail = zb->data;
}

/*
 * Reset buffer
 */
void zbuf_reset(zbuf_t *zb)
{
    zb->data = zb->head + CONFIG_ZBUF_HEADROOM;
    zb->tail = zb->data;
    zb->len = 0;
    zb->protocol = 0;
    zb->l2_offset = 0;
    zb->l3_offset = 0;
    zb->l4_offset = 0;
}

/*
 * Buffer Queue Operations
 */
void zbuf_queue_init(zbuf_queue_t *q)
{
    q->head = NULL;
    q->tail = NULL;
    q->count = 0;
    q->lock = (spinlock_t)SPINLOCK_INIT;
}

void zbuf_queue_push(zbuf_queue_t *q, zbuf_t *zb)
{
    spin_lock_irq(&q->lock);

    zb->next = NULL;
    zb->prev = q->tail;

    if (q->tail != NULL) {
        q->tail->next = zb;
    } else {
        q->head = zb;
    }
    q->tail = zb;
    q->count++;

    spin_unlock_irq(&q->lock);
}

zbuf_t *zbuf_queue_pop(zbuf_queue_t *q)
{
    spin_lock_irq(&q->lock);

    zbuf_t *zb = q->head;
    if (zb != NULL) {
        q->head = zb->next;
        if (q->head != NULL) {
            q->head->prev = NULL;
        } else {
            q->tail = NULL;
        }
        zb->next = NULL;
        zb->prev = NULL;
        q->count--;
    }

    spin_unlock_irq(&q->lock);
    return zb;
}

zbuf_t *zbuf_queue_peek(zbuf_queue_t *q)
{
    zbuf_t *zb;
    spin_lock_irq(&q->lock);
    zb = q->head;
    spin_unlock_irq(&q->lock);
    return zb;
}

uint32_t zbuf_queue_len(zbuf_queue_t *q)
{
    uint32_t len;
    spin_lock_irq(&q->lock);
    len = q->count;
    spin_unlock_irq(&q->lock);
    return len;
}

void zbuf_queue_flush(zbuf_queue_t *q)
{
    zbuf_t *zb;
    while ((zb = zbuf_queue_pop(q)) != NULL) {
        zbuf_free(zb);
    }
}
