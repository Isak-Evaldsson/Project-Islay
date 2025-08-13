/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/thread.h>
#include <utils.h>

#include "../processor.h"

/* Initialises a set of thread registers for a kernel thread, and sets up the stack with the
 * supplied instruction pointer and argument such that ip(arg) is called once the task is started */
void init_thread_regs_with_stack(struct thread_regs* regs, void* stack_top, void (*ip)(void*),
                                 void* arg)
{
    // New to make enough space on the stack to allow the 4 register pops in
    // switch_to_task, as well as the implicit pop when it returns and space for the argument of
    // function ip (need 8 bytes for unkown reasons)
    regs->esp = (uintptr_t)stack_top - 7 * sizeof(int32_t);

    // Set esp0 == esp for now, may change when we introduce a user-space
    regs->esp0 = regs->esp;

    // All kernel processes shares the same page tables
    regs->cr3 = get_cr3();

    // push ip and args to task so that when the task switch returns ip is called
    *(uint32_t*)(regs->esp + 4 * sizeof(uint32_t)) = (uint32_t)ip;

    // insert arg in stack frame
    *(uint32_t*)(regs->esp + 6 * sizeof(uint32_t)) = (uint32_t)arg;
}

/* Create the thread registers for the initial thread */
void init_initial_thread_regs(struct thread_regs* regs)
{
    regs->esp  = 0;  // Will be correctly assigned at context switch
    regs->esp0 = 0;
    regs->cr3  = get_cr3();
}
