/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/interrupts.h>
#include <atomics.h>

#include "test.h"

#define TEST_INTERRUPT1 (ARCH_N_INTERRUPTS - 4)
#define TEST_INTERRUPT2 (ARCH_N_INTERRUPTS - 3)
#define TEST_INTERRUPT3 (ARCH_N_INTERRUPTS - 2)
#define TEST_INTERRUPT4 (ARCH_N_INTERRUPTS - 1)

#if ARCH(i386)
#define INT(i) asm volatile("int %0\n\t" ::"i"(i))
#else
#error "Test not implemented for this arch"
#endif

/* state variables for test_interrupt_ordering  */
static atomic_uint64_t fail             = ATOMIC_INIT();
static atomic_uint64_t isr1_top_done    = ATOMIC_INIT();
static atomic_uint64_t isr2_top_done    = ATOMIC_INIT();
static atomic_uint64_t isr1_bottom_done = ATOMIC_INIT();

/* state variables for test_successive_bottom_halfs */
static atomic_uint64_t bottom_half_count = ATOMIC_INIT();

static void test_isr_ordering_top(struct interrupt_stack_state *state, uint32_t interrupt_number)
{
    (void)(state);

    TEST_LOG("running isr %u top half!", interrupt_number);
    if (interrupt_number == TEST_INTERRUPT1) {
        INT(TEST_INTERRUPT2);
        atomic_store(&isr1_top_done, 1);
    } else {
        atomic_store(&isr2_top_done, 1);
    }
}

static void test_isr_ordering_bottom(uint32_t interrupt_number)
{
    TEST_LOG("running isr %u bottom half!", interrupt_number);

    // Both levels' top half should execute before any bottom half
    if (!atomic_load(&isr1_top_done)) {
        atomic_store(&fail, 1);
        TEST_LOG("wrong ordering for %i bottom - executing bottom half before isr1 top",
                 interrupt_number);
    }

    if (!atomic_load(&isr2_top_done)) {
        atomic_store(&fail, 1);
        TEST_LOG("wrong ordering for %i bottom - executing bottom half before isr2 top",
                 interrupt_number);
    }

    if (interrupt_number == TEST_INTERRUPT1) {
        atomic_store(&isr1_bottom_done, 1);
    } else {
        // Bottom halfs should run sequentially, i.e. isr1 bottom first, then isr2 bottom
        if (!atomic_load(&isr1_bottom_done)) {
            atomic_store(&fail, 1);
            TEST_LOG("wrong ordering for %i bottom - isr1 bottom has not run", interrupt_number);
        }
    }
}

static void test_isr_successive_top(struct interrupt_stack_state *state, uint32_t interrupt_number)
{
    (void)(state);

    TEST_LOG("running isr %u top half!", interrupt_number);
    for (size_t i = 0; i < 5; i++) {
        INT(TEST_INTERRUPT4);
    }
}

static void test_isr_successive_bottom(uint32_t interrupt_number)
{
    TEST_LOG("running isr %u bottom half!", interrupt_number);
    atomic_add_fetch(&bottom_half_count, 1);
}

static int interrupt_tests_setup()
{
    TEST_ERRNO_FUNC(register_interrupt_handler(TEST_INTERRUPT1, test_isr_ordering_top,
                                               test_isr_ordering_bottom));
    TEST_ERRNO_FUNC(register_interrupt_handler(TEST_INTERRUPT2, test_isr_ordering_top,
                                               test_isr_ordering_bottom));
    TEST_ERRNO_FUNC(register_interrupt_handler(TEST_INTERRUPT3, test_isr_successive_top, NULL));
    TEST_ERRNO_FUNC(register_interrupt_handler(TEST_INTERRUPT4, NULL, test_isr_successive_bottom));
    return 0;
}

static int interrupt_tests_teardown()
{
    // TODO: Unregister test interrupts...
    return -1;
}

/*
    Triggers nested interrupts and test that bottom half still runs sequentially

*/
static int test_interrupt_ordering()
{
    // Run multiple times to be sure that we just wan't lucky due to timing
    for (size_t i = 0; i < 5; i++) {
        atomic_store(&fail, 0);
        atomic_store(&isr1_top_done, 0);
        atomic_store(&isr2_top_done, 0);
        atomic_store(&isr1_bottom_done, 0);

        INT(TEST_INTERRUPT1);
        if (atomic_load(&fail)) {
            return -1;
        }

        nano_sleep(100 * 1e3 /* 100 miliseconds */);
    }

    return 0;
}

/*
    Test that we don't enqueue the same interrupt multiple times while one is running
*/
static int test_successive_bottom_halfs()
{
    INT(TEST_INTERRUPT3);

    int res = atomic_load(&bottom_half_count);
    if (res != 1) {
        TEST_LOG("bottom_half_count is %u", res);
        return -1;
    }

    return 0;
}

int (*interrupt_tests[])() = {
    test_interrupt_ordering,
    test_successive_bottom_halfs,
};

struct test_suite interrupt_test_suite = {
    .name     = "interrupt_tests",
    .setup    = interrupt_tests_setup,
    .teardown = interrupt_tests_teardown,
    .tests    = interrupt_tests,
    .n_tests  = COUNT_ARRAY_ELEMS(interrupt_tests),
};
