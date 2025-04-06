/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef TESTS_TEST_H
#define TESTS_TEST_H

#include <utils.h>

struct test_func {
    int (*ptr)();
    const char* name;
};

#define CREATE_TEST_FUNC(func) {.ptr = func, .name = #func}

struct test_suite {
    char* name;
    int (*setup)();             // Runs once before executing the tests
    int (*teardown)();          // Runs once after executing the tests
    struct test_func* tests;    // The tests to be executed
    unsigned int      n_tests;  // Number of tests with the tests array
};

/* The currently running test suite */
extern struct test_suite* current_suite;

/* For test specific logging */
#define TEST_LOG(fmt, ...)                                                             \
    do {                                                                               \
        log("[KERNEL_TESTS]: %s:%s:%u: " fmt, current_suite->name, __FILE__, __LINE__, \
            ##__VA_ARGS__);                                                            \
    } while (0)

/* Test functions which return an integer 0 on success, or -ERRNO otherwise. If failure, it will log
 * and return */
#define TEST_ERRNO_FUNC(expr)                                   \
    do {                                                        \
        int _val = (expr);                                      \
        if (_val < 0) {                                         \
            TEST_LOG("'%s' failed, returning %i", #expr, _val); \
            return _val;                                        \
        }                                                       \
    } while (0)

/* Return if expression is false */
#define TEST_RETURN_IF_FALSE(expr)                        \
    do {                                                  \
        if (!(expr)) {                                    \
            TEST_LOG("'%s' failed, returning %i", #expr); \
            return -1;                                    \
        }                                                 \
    } while (0)

void run_post_boot_tests();

#endif /* TESTS_TEST_H */
