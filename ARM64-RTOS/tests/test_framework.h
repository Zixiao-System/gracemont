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
 * RTOS Unit Test Framework
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include "rtos_types.h"

/*
 * Test Result Codes
 */
#define TEST_PASS       0
#define TEST_FAIL       1
#define TEST_SKIP       2

/*
 * Test Case Structure
 */
typedef struct {
    const char  *name;
    int         (*func)(void);
} test_case_t;

/*
 * Test Suite Structure
 */
typedef struct {
    const char      *name;
    test_case_t     *tests;
    uint32_t        test_count;
    void            (*setup)(void);
    void            (*teardown)(void);
} test_suite_t;

/*
 * Test Statistics
 */
typedef struct {
    uint32_t    total;
    uint32_t    passed;
    uint32_t    failed;
    uint32_t    skipped;
} test_stats_t;

/*
 * Assertion Macros
 */
#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            test_assert_failed(__FILE__, __LINE__, #cond); \
            return TEST_FAIL; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            test_assert_eq_failed(__FILE__, __LINE__, (uint64_t)(a), (uint64_t)(b)); \
            return TEST_FAIL; \
        } \
    } while (0)

#define TEST_ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            test_assert_ne_failed(__FILE__, __LINE__, (uint64_t)(a)); \
            return TEST_FAIL; \
        } \
    } while (0)

#define TEST_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            test_assert_null_failed(__FILE__, __LINE__); \
            return TEST_FAIL; \
        } \
    } while (0)

#define TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            test_assert_not_null_failed(__FILE__, __LINE__); \
            return TEST_FAIL; \
        } \
    } while (0)

#define TEST_ASSERT_MEM_EQ(a, b, len) \
    do { \
        if (!test_mem_equal((a), (b), (len))) { \
            test_assert_mem_failed(__FILE__, __LINE__); \
            return TEST_FAIL; \
        } \
    } while (0)

/*
 * Test API Functions
 */
void test_init(void);
void test_run_suite(test_suite_t *suite);
void test_run_all(void);
void test_print_summary(void);
test_stats_t *test_get_stats(void);

/* Internal functions for assertions */
void test_assert_failed(const char *file, int line, const char *cond);
void test_assert_eq_failed(const char *file, int line, uint64_t a, uint64_t b);
void test_assert_ne_failed(const char *file, int line, uint64_t val);
void test_assert_null_failed(const char *file, int line);
void test_assert_not_null_failed(const char *file, int line);
void test_assert_mem_failed(const char *file, int line);
bool test_mem_equal(const void *a, const void *b, size_t len);

/*
 * Test Registration Macro
 */
#define TEST_CASE(name) \
    static int test_##name(void)

#define TEST_SUITE(suite_name, ...) \
    static test_case_t suite_name##_tests[] = { __VA_ARGS__ }; \
    static test_suite_t suite_name = { \
        .name = #suite_name, \
        .tests = suite_name##_tests, \
        .test_count = sizeof(suite_name##_tests) / sizeof(test_case_t), \
        .setup = NULL, \
        .teardown = NULL \
    }

#endif /* TEST_FRAMEWORK_H */
