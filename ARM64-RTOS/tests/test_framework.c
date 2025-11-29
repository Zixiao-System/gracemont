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
 * Unit Test Framework Implementation
 */

#include "test_framework.h"
#include "rtos.h"

/* Global test statistics */
static test_stats_t test_stats;

/* UART output for test results */
extern void uart_puts(uint32_t port, const char *s);
extern void uart_putc(uint32_t port, char c);

static void test_print(const char *s)
{
    uart_puts(0, s);
}

static void test_print_num(uint32_t n)
{
    char buf[12];
    int i = 0;

    if (n == 0) {
        uart_putc(0, '0');
        return;
    }

    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }

    while (i > 0) {
        uart_putc(0, buf[--i]);
    }
}

void test_init(void)
{
    test_stats.total = 0;
    test_stats.passed = 0;
    test_stats.failed = 0;
    test_stats.skipped = 0;

    test_print("\n");
    test_print("========================================\n");
    test_print(" Gracemont ARM64 RTOS Unit Tests\n");
    test_print("========================================\n\n");
}

void test_run_suite(test_suite_t *suite)
{
    test_print("Suite: ");
    test_print(suite->name);
    test_print("\n");
    test_print("----------------------------------------\n");

    for (uint32_t i = 0; i < suite->test_count; i++) {
        test_case_t *tc = &suite->tests[i];

        /* Run setup if defined */
        if (suite->setup) {
            suite->setup();
        }

        test_print("  ");
        test_print(tc->name);
        test_print(" ... ");

        /* Run test */
        int result = tc->func();
        test_stats.total++;

        switch (result) {
        case TEST_PASS:
            test_print("[PASS]\n");
            test_stats.passed++;
            break;
        case TEST_FAIL:
            test_print("[FAIL]\n");
            test_stats.failed++;
            break;
        case TEST_SKIP:
            test_print("[SKIP]\n");
            test_stats.skipped++;
            break;
        }

        /* Run teardown if defined */
        if (suite->teardown) {
            suite->teardown();
        }
    }

    test_print("\n");
}

void test_print_summary(void)
{
    test_print("========================================\n");
    test_print(" Test Summary\n");
    test_print("========================================\n");
    test_print("  Total:   ");
    test_print_num(test_stats.total);
    test_print("\n");
    test_print("  Passed:  ");
    test_print_num(test_stats.passed);
    test_print("\n");
    test_print("  Failed:  ");
    test_print_num(test_stats.failed);
    test_print("\n");
    test_print("  Skipped: ");
    test_print_num(test_stats.skipped);
    test_print("\n");
    test_print("========================================\n");

    if (test_stats.failed == 0) {
        test_print("All tests passed!\n");
    } else {
        test_print("Some tests failed!\n");
    }
}

test_stats_t *test_get_stats(void)
{
    return &test_stats;
}

void test_assert_failed(const char *file, int line, const char *cond)
{
    test_print("\n    Assertion failed: ");
    test_print(cond);
    test_print("\n    at ");
    test_print(file);
    test_print(":");
    test_print_num(line);
    test_print("\n");
}

void test_assert_eq_failed(const char *file, int line, uint64_t a, uint64_t b)
{
    test_print("\n    Expected equal values at ");
    test_print(file);
    test_print(":");
    test_print_num(line);
    test_print("\n");
}

void test_assert_ne_failed(const char *file, int line, uint64_t val)
{
    test_print("\n    Expected different values at ");
    test_print(file);
    test_print(":");
    test_print_num(line);
    test_print("\n");
}

void test_assert_null_failed(const char *file, int line)
{
    test_print("\n    Expected NULL at ");
    test_print(file);
    test_print(":");
    test_print_num(line);
    test_print("\n");
}

void test_assert_not_null_failed(const char *file, int line)
{
    test_print("\n    Expected non-NULL at ");
    test_print(file);
    test_print(":");
    test_print_num(line);
    test_print("\n");
}

void test_assert_mem_failed(const char *file, int line)
{
    test_print("\n    Memory comparison failed at ");
    test_print(file);
    test_print(":");
    test_print_num(line);
    test_print("\n");
}

bool test_mem_equal(const void *a, const void *b, size_t len)
{
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;

    for (size_t i = 0; i < len; i++) {
        if (pa[i] != pb[i]) {
            return false;
        }
    }
    return true;
}
