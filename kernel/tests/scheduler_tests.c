/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <atomics.h>
#include <devices/timer.h>
#include <tasks/locking.h>
#include <tasks/scheduler.h>
#include <utils.h>

#include "test.h"

/* Util functions */
static void wait(unsigned int seconds)
{
    // By design not dependent on the scheduler)
    uint64_t start_time = timer_get_time_since_boot();
    while (timer_get_time_since_boot() < start_time + SECONDS_TO_NS(5))
        ;
}

static task_state_t get_task_state(tid_t tid)
{
    task_state_t state;
    task_t      *t = get_task(tid);
    if (!t) {
        return TASK_STATE_MAX;
    }
    state = t->state;
    put_task(t);
    return state;
}

/* Sleeper test */
static atomic_uint_t sleep_counter = ATOMIC_INIT();
static atomic_uint_t sleeper_stop  = ATOMIC_INIT();

static void sleeper()
{
    int i = 0;

    while (!atomic_load(&sleeper_stop)) {
        TEST_LOG("Sleeper %u", atomic_add_fetch(&sleep_counter, 1));
        sleep(1);
    }
}

static int sleep_test()
{
    uint64_t start_time = timer_get_time_since_boot();
    create_task(&sleeper);

    // Let sleeper run for 5 seconds
    wait(5);
    atomic_store(&sleeper_stop, 1);

    // Since the sleeper wait 1 second between each increment, the counter should have reach 5 by
    // now
    TEST_RETURN_IF_FALSE(atomic_load(&sleep_counter) == 5);
    return 0;
}

// Mutex test
static MUTEX_DEFINE(test_mutex);
static bool          main_done    = false;
static bool          failure      = false;
static atomic_uint_t threads_left = ATOMIC_INIT();

static void f1()
{
    mutex_lock(&test_mutex);
    TEST_LOG("f1 acquired lock\n");

    if (!main_done) {
        failure = true;
    }

    atomic_sub_fetch(&threads_left, 1);
    mutex_unlock(&test_mutex);
}

static void f2()
{
    mutex_lock(&test_mutex);
    TEST_LOG("f2 acquired lock\n");
    if (!main_done) {
        failure = true;
    }

    atomic_sub_fetch(&threads_left, 1);
    mutex_unlock(&test_mutex);
}

static int mutex_test()
{
    atomic_store(&threads_left, 2);

    // Take lock, blocking thread f1 and f2
    mutex_lock(&test_mutex);

    tid_t t1 = create_task(&f1);
    tid_t t2 = create_task(&f2);

    // Give the newly created tasks a chance to run
    scheduler_yield();

    // Ensure that they are blocked
    TEST_RETURN_IF_FALSE(get_task_state(t1) == WAITING_FOR_LOCK);
    TEST_RETURN_IF_FALSE(get_task_state(t2) == WAITING_FOR_LOCK);

    main_done = true;
    mutex_unlock(&test_mutex);
    while (atomic_load(&threads_left))
        ;

    TEST_RETURN_IF_FALSE(!failure);
    return 0;
}

/* Cleanup test */
static void void_thread()
{
    // Empty by design
}

static int cleanup_test()
{
    int     ret = 0;
    tid_t   tid = create_task(&void_thread);
    task_t *t   = get_task(tid);

    // Let the empty thread execute (and die)
    scheduler_yield();

    if (t->state != TERMINATED) {
        TEST_LOG("thread %x not terminated, in state %u", t->state);
        ret = -1;
    }

    // Once t is terminated, only this test calling get_task should have a reference to this object
    unsigned int count = atomic_load(&t->ref_count);
    if (count != 1) {
        TEST_LOG("thread %x has incorrect refcout value %u", count);
        ret = -2;
    }

    TEST_LOG("t: %x, %u, %u", t, t->state, count);
    put_task(t);  // Allow task to be free'd
    return ret;
}

struct test_func scheduling_tests[] = {
    CREATE_TEST_FUNC(sleep_test),
    CREATE_TEST_FUNC(mutex_test),
    CREATE_TEST_FUNC(cleanup_test),
};

struct test_suite scheduler_test_suite = {
    .name     = "sched_tests",
    .setup    = NULL,
    .teardown = NULL,
    .tests    = scheduling_tests,
    .n_tests  = COUNT_ARRAY_ELEMS(scheduling_tests),
};
