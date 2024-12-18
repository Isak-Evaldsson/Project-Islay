/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include "test.h"

extern struct test_suite interrupt_test_suite;
extern struct test_suite fs_test_suite;

static struct test_suite* post_boot_tests[] = {
    &interrupt_test_suite,
    &fs_test_suite,
};

struct test_suite* current_suite;

static bool run_suite(struct test_suite* suite)
{
    int  ret;
    bool failed = false;
    kprintf("Executing %s:\n", suite->name);

    if (suite->setup) {
        ret = suite->setup();
        if (ret != 0) {
            kprintf("%s setup failed (%i):\n", suite->name, ret);
            return true;
        }
    }

    for (size_t i = 0; i < suite->n_tests; i++) {
        kprintf("  Running test %u: ", i);
        ret = suite->tests[i]();
        if (ret != 0) {
            kprintf("Failed (%i)\n", ret);
            failed = true;
        } else {
            kprintf("Ok\n");
        }
    }

    if (suite->teardown) {
        ret = suite->teardown();
        if (ret != 0) {
            kprintf("%s teardown failed (%i):\n", suite->name, ret);
            return true;
        }
    }
    return failed;
}

void run_post_boot_tests()
{
    bool failure = false;
    kprintf("Executing post-boot tests...\n");

    for (size_t i = 0; i < COUNT_ARRAY_ELEMS(post_boot_tests); i++) {
        current_suite = post_boot_tests[i];
        if (run_suite(current_suite)) {
            failure = true;
        }
    }

    if (failure) {
        kpanic("post-boot tests failed");
    }
}
