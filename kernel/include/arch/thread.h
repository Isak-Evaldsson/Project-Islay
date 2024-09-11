/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_THREAD_H
#define ARCH_THREAD_H
#include <arch/platfrom.h>
#include <stdint.h>

/* Import architecture independent defintion of struct thread_regs. Struct storing the register data
 * necessary for context switching */
#if ARCH(i386)
#include "i386/thread.h"
#else
#error "Unkown architecture"
#endif

/* asm routine that switches between kernel threads, by saving old_threads state and loading
 * new_threads state  */
void kernel_thread_switch(struct thread_regs* new_thread, struct thread_regs* old_thread);

/* Initialises a set of thread registers for a kernel thread, and sets up the stack with the
 * supplied instruction pointer and argument such that ip(arg) is called once the task is started */
void init_thread_regs_with_stack(struct thread_regs* regs, void* stack_top, void (*ip)(void*),
                                 void* arg);

/* Initialise the thread registers for the initial thread */
void init_initial_thread_regs(struct thread_regs* regs);

#endif /* ARCH_THREAD_H */
