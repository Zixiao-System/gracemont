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
 * Zero-Copy Buffer Unit Tests
 */

#include "test_framework.h"
#include "zbuf.h"

/*
 * Test: Basic zbuf allocation
 */
TEST_CASE(zbuf_alloc_basic)
{
    zbuf_t *zb = zbuf_alloc_tx(256);
    TEST_ASSERT_NOT_NULL(zb);
    TEST_ASSERT_NOT_NULL(zb->head);
    TEST_ASSERT_EQ(zb->refcount, 1);

    zbuf_free(zb);
    return TEST_PASS;
}

/*
 * Test: zbuf put operation
 */
TEST_CASE(zbuf_put_basic)
{
    zbuf_t *zb = zbuf_alloc_tx(256);
    TEST_ASSERT_NOT_NULL(zb);

    uint8_t *p = zbuf_put(zb, 64);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQ(zb->len, 64);

    /* Write data */
    for (int i = 0; i < 64; i++) {
        p[i] = i;
    }

    zbuf_free(zb);
    return TEST_PASS;
}

/*
 * Test: zbuf push operation (add header)
 */
TEST_CASE(zbuf_push_basic)
{
    zbuf_t *zb = zbuf_alloc_tx(256);
    TEST_ASSERT_NOT_NULL(zb);

    /* Put some data */
    zbuf_put(zb, 64);

    /* Push header */
    uint8_t *hdr = zbuf_push(zb, 14);
    TEST_ASSERT_NOT_NULL(hdr);
    TEST_ASSERT_EQ(zb->len, 78);
    TEST_ASSERT_EQ(zb->data, hdr);

    zbuf_free(zb);
    return TEST_PASS;
}

/*
 * Test: zbuf pull operation (remove header)
 */
TEST_CASE(zbuf_pull_basic)
{
    zbuf_t *zb = zbuf_alloc_tx(256);
    TEST_ASSERT_NOT_NULL(zb);

    /* Put data with header */
    uint8_t *p = zbuf_put(zb, 78);
    for (int i = 0; i < 78; i++) {
        p[i] = i;
    }

    /* Pull header */
    uint8_t *pulled = zbuf_pull(zb, 14);
    TEST_ASSERT_NOT_NULL(pulled);
    TEST_ASSERT_EQ(zb->len, 64);
    TEST_ASSERT_EQ(pulled[0], 0);
    TEST_ASSERT_EQ(zb->data[0], 14);

    zbuf_free(zb);
    return TEST_PASS;
}

/*
 * Test: zbuf reference counting
 */
TEST_CASE(zbuf_refcount)
{
    zbuf_t *zb = zbuf_alloc_tx(256);
    TEST_ASSERT_NOT_NULL(zb);
    TEST_ASSERT_EQ(zb->refcount, 1);

    zbuf_ref(zb);
    TEST_ASSERT_EQ(zb->refcount, 2);

    zbuf_ref(zb);
    TEST_ASSERT_EQ(zb->refcount, 3);

    zbuf_free(zb);
    TEST_ASSERT_EQ(zb->refcount, 2);

    zbuf_free(zb);
    TEST_ASSERT_EQ(zb->refcount, 1);

    zbuf_free(zb);
    return TEST_PASS;
}

/*
 * Test: zbuf queue operations
 */
TEST_CASE(zbuf_queue_basic)
{
    zbuf_queue_t queue;
    zbuf_queue_init(&queue);

    TEST_ASSERT_EQ(queue.count, 0);
    TEST_ASSERT_NULL(zbuf_queue_pop(&queue));

    /* Push some buffers */
    zbuf_t *zb1 = zbuf_alloc_tx(64);
    zbuf_t *zb2 = zbuf_alloc_tx(64);
    zbuf_t *zb3 = zbuf_alloc_tx(64);

    zbuf_queue_push(&queue, zb1);
    TEST_ASSERT_EQ(queue.count, 1);

    zbuf_queue_push(&queue, zb2);
    TEST_ASSERT_EQ(queue.count, 2);

    zbuf_queue_push(&queue, zb3);
    TEST_ASSERT_EQ(queue.count, 3);

    /* Pop in order */
    zbuf_t *p = zbuf_queue_pop(&queue);
    TEST_ASSERT_EQ(p, zb1);
    TEST_ASSERT_EQ(queue.count, 2);

    p = zbuf_queue_pop(&queue);
    TEST_ASSERT_EQ(p, zb2);
    TEST_ASSERT_EQ(queue.count, 1);

    p = zbuf_queue_pop(&queue);
    TEST_ASSERT_EQ(p, zb3);
    TEST_ASSERT_EQ(queue.count, 0);

    zbuf_free(zb1);
    zbuf_free(zb2);
    zbuf_free(zb3);

    return TEST_PASS;
}

/*
 * Test: zbuf clone
 */
TEST_CASE(zbuf_clone_basic)
{
    zbuf_t *zb = zbuf_alloc_tx(256);
    TEST_ASSERT_NOT_NULL(zb);

    uint8_t *p = zbuf_put(zb, 64);
    for (int i = 0; i < 64; i++) {
        p[i] = i;
    }

    zbuf_t *clone = zbuf_clone(zb);
    TEST_ASSERT_NOT_NULL(clone);
    TEST_ASSERT_EQ(clone->len, zb->len);
    TEST_ASSERT_NE(clone->head, zb->head);

    /* Verify data is copied */
    for (int i = 0; i < 64; i++) {
        TEST_ASSERT_EQ(clone->data[i], i);
    }

    zbuf_free(zb);
    zbuf_free(clone);

    return TEST_PASS;
}

/*
 * Test: zbuf reserve headroom
 */
TEST_CASE(zbuf_reserve_headroom)
{
    zbuf_t *zb = zbuf_alloc_rx(256);
    TEST_ASSERT_NOT_NULL(zb);

    /* Reserve headroom for headers */
    zbuf_reserve(zb, 64);

    /* Now data pointer should be ahead */
    TEST_ASSERT_EQ(zb->data - zb->head, 64);

    /* We can now push headers into reserved space */
    uint8_t *hdr = zbuf_push(zb, 14);
    TEST_ASSERT_NOT_NULL(hdr);
    TEST_ASSERT_EQ(zb->data - zb->head, 50);

    zbuf_free(zb);
    return TEST_PASS;
}

/*
 * Test Suite Definition
 */
static test_case_t zbuf_tests[] = {
    { "zbuf_alloc_basic", test_zbuf_alloc_basic },
    { "zbuf_put_basic", test_zbuf_put_basic },
    { "zbuf_push_basic", test_zbuf_push_basic },
    { "zbuf_pull_basic", test_zbuf_pull_basic },
    { "zbuf_refcount", test_zbuf_refcount },
    { "zbuf_queue_basic", test_zbuf_queue_basic },
    { "zbuf_clone_basic", test_zbuf_clone_basic },
    { "zbuf_reserve_headroom", test_zbuf_reserve_headroom },
};

test_suite_t zbuf_test_suite = {
    .name = "Zero-Copy Buffers",
    .tests = zbuf_tests,
    .test_count = sizeof(zbuf_tests) / sizeof(test_case_t),
    .setup = NULL,
    .teardown = NULL
};
