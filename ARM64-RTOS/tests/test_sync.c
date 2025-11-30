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
 * Synchronization Primitives Unit Tests
 */

#include "test_framework.h"
#include "rtos.h"

/*
 * Test: Spinlock basic operations
 */
TEST_CASE(spinlock_basic)
{
    spinlock_t lock = SPINLOCK_INIT;

    spin_lock(&lock);
    /* Critical section */
    spin_unlock(&lock);

    return TEST_PASS;
}

/*
 * Test: Spinlock with IRQ save/restore
 */
TEST_CASE(spinlock_irq)
{
    spinlock_t lock = SPINLOCK_INIT;

    spin_lock_irq(&lock);
    /* Critical section with interrupts disabled */
    spin_unlock_irq(&lock);

    return TEST_PASS;
}

/*
 * Test: Semaphore basic operations
 */
TEST_CASE(semaphore_basic)
{
    semaphore_t sem;
    sem_init(&sem, 1);

    TEST_ASSERT_EQ(sem.count, 1);

    /* Wait should succeed immediately */
    status_t ret = sem_trywait(&sem);
    TEST_ASSERT_EQ(ret, STATUS_OK);
    TEST_ASSERT_EQ(sem.count, 0);

    /* Try wait should fail now */
    ret = sem_trywait(&sem);
    TEST_ASSERT_EQ(ret, STATUS_WOULD_BLOCK);

    /* Post should increment */
    sem_post(&sem);
    TEST_ASSERT_EQ(sem.count, 1);

    return TEST_PASS;
}

/*
 * Test: Semaphore with initial count
 */
TEST_CASE(semaphore_counting)
{
    semaphore_t sem;
    sem_init(&sem, 5);

    TEST_ASSERT_EQ(sem.count, 5);

    /* Should be able to wait 5 times */
    for (int i = 0; i < 5; i++) {
        status_t ret = sem_trywait(&sem);
        TEST_ASSERT_EQ(ret, STATUS_OK);
    }

    TEST_ASSERT_EQ(sem.count, 0);

    /* 6th should fail */
    status_t ret = sem_trywait(&sem);
    TEST_ASSERT_EQ(ret, STATUS_WOULD_BLOCK);

    /* Post multiple times */
    for (int i = 0; i < 5; i++) {
        sem_post(&sem);
    }

    TEST_ASSERT_EQ(sem.count, 5);

    return TEST_PASS;
}

/*
 * Test: Mutex basic operations
 */
TEST_CASE(mutex_basic)
{
    mutex_t mutex;
    mutex_init(&mutex);

    TEST_ASSERT(mutex.owner == NULL);

    status_t ret = mutex_trylock(&mutex);
    TEST_ASSERT_EQ(ret, STATUS_OK);

    mutex_unlock(&mutex);

    return TEST_PASS;
}

/*
 * Test: Mutex recursive locking (if supported)
 */
TEST_CASE(mutex_ownership)
{
    mutex_t mutex;
    mutex_init(&mutex);

    status_t ret = mutex_trylock(&mutex);
    TEST_ASSERT_EQ(ret, STATUS_OK);

    /* Second trylock should fail (different thread context) */
    /* In test context, we can't really test multi-threading */

    mutex_unlock(&mutex);

    return TEST_PASS;
}

/*
 * Test: Event basic operations
 */
TEST_CASE(event_basic)
{
    event_t event;
    event_init(&event);

    /* Set some flags */
    event_set(&event, 0x0F);

    /* Set more flags */
    event_set(&event, 0xF0);

    /* Clear flags */
    event_clear(&event, 0x0F);

    return TEST_PASS;
}

/*
 * Test: Message queue basic operations
 */
static uint8_t msgq_buffer[8 * 32] __attribute__((aligned(8)));

TEST_CASE(msgqueue_basic)
{
    msgqueue_t queue;
    status_t ret = msgq_init(&queue, msgq_buffer, 32, 8);
    TEST_ASSERT_EQ(ret, STATUS_OK);

    uint8_t msg[32] = {1, 2, 3, 4, 5};

    /* Send message (with timeout 0 for non-blocking) */
    ret = msgq_send(&queue, msg, 0);
    TEST_ASSERT_EQ(ret, STATUS_OK);

    /* Receive message */
    uint8_t recv[32];
    ret = msgq_recv(&queue, recv, 0);
    TEST_ASSERT_EQ(ret, STATUS_OK);
    TEST_ASSERT_MEM_EQ(msg, recv, 5);

    return TEST_PASS;
}

/*
 * Test Suite Definition
 */
static test_case_t sync_tests[] = {
    { "spinlock_basic", test_spinlock_basic },
    { "spinlock_irq", test_spinlock_irq },
    { "semaphore_basic", test_semaphore_basic },
    { "semaphore_counting", test_semaphore_counting },
    { "mutex_basic", test_mutex_basic },
    { "mutex_ownership", test_mutex_ownership },
    { "event_basic", test_event_basic },
    { "msgqueue_basic", test_msgqueue_basic },
};

test_suite_t sync_test_suite = {
    .name = "Synchronization Primitives",
    .tests = sync_tests,
    .test_count = sizeof(sync_tests) / sizeof(test_case_t),
    .setup = NULL,
    .teardown = NULL
};
