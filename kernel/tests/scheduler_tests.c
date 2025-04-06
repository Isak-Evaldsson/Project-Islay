/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <tasks/locking.h>
#include <tasks/scheduler.h>
#include <utils.h>

#include "test.h"

static mutex_t *m;

static void sleeper()
{
    int i = 0;

    for (;;) {
        TEST_LOG("Sleeper %u", ++i);
        sleep(1);
    }
}

static void f1()
{
    mutex_lock(m);
    TEST_LOG("f1 acquired lock\n");
    mutex_unlock(m);
}

static void f2()
{
    mutex_lock(m);
    TEST_LOG("f2 acquired lock\n");
    mutex_unlock(m);
}

static int scheduler_test()
{
    //
    // Sleep tests
    //
    TEST_LOG("Spawn new task\n");
    m = mutex_create();
    create_task(&sleeper);
    create_task(&f1);
    create_task(&f2);
    mutex_lock(m);

    TEST_LOG("Back to main.");
    for (int i = 0; i < 5; i++) {
        sleep(1);
        TEST_LOG(".");
    }
    TEST_LOG("\n");
    mutex_unlock(m);
    return 0;  // TODO: Find a way to determine test failure, for example setting up some kind of
               // time event?
}

struct test_func scheduling_tests[] = {
    CREATE_TEST_FUNC(scheduler_test),
};

struct test_suite scheduler_test_suite = {
    .name     = "sched_tests",
    .setup    = NULL,
    .teardown = NULL,
    .tests    = scheduling_tests,
    .n_tests  = COUNT_ARRAY_ELEMS(scheduling_tests),
};
