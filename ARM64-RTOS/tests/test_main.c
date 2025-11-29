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
 * Unit Test Main Entry Point
 */

#include "test_framework.h"
#include "rtos.h"

/* External test suites */
extern test_suite_t memory_test_suite;
extern test_suite_t zbuf_test_suite;
extern test_suite_t sync_test_suite;
extern test_suite_t modbus_test_suite;

/*
 * Run all unit tests
 */
void test_run_all(void)
{
    test_init();

    test_run_suite(&memory_test_suite);
    test_run_suite(&zbuf_test_suite);
    test_run_suite(&sync_test_suite);
    test_run_suite(&modbus_test_suite);

    test_print_summary();
}

/*
 * Test task for RTOS
 */
void test_task(void *arg)
{
    (void)arg;

    /* Wait for system to stabilize */
    task_delay(100);

    /* Run tests */
    test_run_all();

    /* Done */
    while (1) {
        task_delay(1000);
    }
}

/*
 * Main entry point for test build
 */
#ifdef BUILD_TESTS
int test_main(void)
{
    /* Create test task */
    tcb_t *task = task_create("test", test_task, NULL, 2048, 10);
    if (task == NULL) {
        return -1;
    }

    task_start(task);

    return 0;
}
#endif
