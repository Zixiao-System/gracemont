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
 * Memory Management Unit Tests
 */

#include "test_framework.h"
#include "rtos.h"

/*
 * Test: Basic heap allocation
 */
TEST_CASE(heap_alloc_basic)
{
    void *ptr = heap_alloc(64);
    TEST_ASSERT_NOT_NULL(ptr);

    /* Write to memory to ensure it's valid */
    uint8_t *p = (uint8_t *)ptr;
    for (int i = 0; i < 64; i++) {
        p[i] = i;
    }

    /* Verify */
    for (int i = 0; i < 64; i++) {
        TEST_ASSERT_EQ(p[i], i);
    }

    heap_free(ptr);
    return TEST_PASS;
}

/*
 * Test: Multiple allocations
 */
TEST_CASE(heap_alloc_multiple)
{
    void *ptrs[10];

    /* Allocate */
    for (int i = 0; i < 10; i++) {
        ptrs[i] = heap_alloc(128);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    /* Verify all pointers are different */
    for (int i = 0; i < 10; i++) {
        for (int j = i + 1; j < 10; j++) {
            TEST_ASSERT_NE(ptrs[i], ptrs[j]);
        }
    }

    /* Free all */
    for (int i = 0; i < 10; i++) {
        heap_free(ptrs[i]);
    }

    return TEST_PASS;
}

/*
 * Test: Aligned allocation
 */
TEST_CASE(heap_alloc_aligned)
{
    void *ptr = heap_alloc_aligned(256, 64);
    TEST_ASSERT_NOT_NULL(ptr);
    TEST_ASSERT_EQ((addr_t)ptr & 63, 0);

    heap_free(ptr);
    return TEST_PASS;
}

/*
 * Test: Large allocation
 */
TEST_CASE(heap_alloc_large)
{
    void *ptr = heap_alloc(4096);
    TEST_ASSERT_NOT_NULL(ptr);

    /* Write pattern */
    uint32_t *p = (uint32_t *)ptr;
    for (int i = 0; i < 1024; i++) {
        p[i] = 0xDEADBEEF;
    }

    /* Verify */
    for (int i = 0; i < 1024; i++) {
        TEST_ASSERT_EQ(p[i], 0xDEADBEEF);
    }

    heap_free(ptr);
    return TEST_PASS;
}

/*
 * Test: Memory pool allocation
 */
static uint8_t pool_buffer[64 * 8] __attribute__((aligned(8)));

TEST_CASE(mempool_basic)
{
    mempool_t pool;
    status_t ret = mempool_init(&pool, pool_buffer, 64, 8);
    TEST_ASSERT_EQ(ret, STATUS_OK);

    void *blocks[8];

    /* Allocate all blocks */
    for (int i = 0; i < 8; i++) {
        blocks[i] = mempool_alloc(&pool);
        TEST_ASSERT_NOT_NULL(blocks[i]);
    }

    /* Should fail - pool exhausted */
    void *fail = mempool_alloc(&pool);
    TEST_ASSERT_NULL(fail);

    /* Free one and reallocate */
    mempool_free(&pool, blocks[0]);
    blocks[0] = mempool_alloc(&pool);
    TEST_ASSERT_NOT_NULL(blocks[0]);

    /* Free all */
    for (int i = 0; i < 8; i++) {
        mempool_free(&pool, blocks[i]);
    }

    return TEST_PASS;
}

/*
 * Test: DMA allocation
 */
TEST_CASE(dma_alloc_basic)
{
    void *ptr = dma_alloc(4096);
    TEST_ASSERT_NOT_NULL(ptr);

    /* Must be page aligned */
    TEST_ASSERT_EQ((addr_t)ptr & 0xFFF, 0);

    dma_free(ptr, 4096);
    return TEST_PASS;
}

/*
 * Test Suite Definition
 */
static test_case_t memory_tests[] = {
    { "heap_alloc_basic", test_heap_alloc_basic },
    { "heap_alloc_multiple", test_heap_alloc_multiple },
    { "heap_alloc_aligned", test_heap_alloc_aligned },
    { "heap_alloc_large", test_heap_alloc_large },
    { "mempool_basic", test_mempool_basic },
    { "dma_alloc_basic", test_dma_alloc_basic },
};

test_suite_t memory_test_suite = {
    .name = "Memory Management",
    .tests = memory_tests,
    .test_count = sizeof(memory_tests) / sizeof(test_case_t),
    .setup = NULL,
    .teardown = NULL
};
