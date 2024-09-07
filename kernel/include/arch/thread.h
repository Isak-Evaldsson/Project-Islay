/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_THREAD_H
#define ARCH_THREAD_H

/* Struct storing architecture dependent thread data used for context switching */
typedef struct thread_regs thread_regs_t;

/* asm routine that switches between kernel threads, by saving old_threads state and loading
 * new_threads state  */
void kernel_thread_switch(thread_regs_t* new_thread, thread_regs_t* old_thread);

/* Creates a set of thread register for a kernel thread. And initiates the stack with the supplied
 * instruction pointer and argument such that ip(arg) is called once the task is started */
thread_regs_t* create_thread_regs_with_stack(void* stack_top, void (*ip)(void*), void* arg);

/* Create the thread registers for the initial thread */
thread_regs_t* create_initial_thread_regs();

/* Frees the thread_regs_t struct */
void free_thread_regs(thread_regs_t* regs);

#endif /* ARCH_THREAD_H */
