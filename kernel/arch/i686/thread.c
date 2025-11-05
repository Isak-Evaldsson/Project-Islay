/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/thread.h>
#include <utils.h>

#include "processor.h"

/* Initialises a set of thread registers for a kernel thread, and sets up the stack with the
 * supplied instruction pointer and argument such that ip(arg) is called once the task is started */
void init_thread_regs_with_stack(struct thread_regs* regs, void* stack_top, void (*ip)(void*),
                                 void* arg)
{
    uint16_t cs     = 0x08;   // Kernel code segment in gdt table
    uint32_t eflags = 0x202;  // Interrupts enabled, all other flags are cleared
                              // (except bit 1 which must be zero)
    /*
     * Since context switches occurs during interrupts, the initial stack needs to
     * be set up to return from the interrupt correctly. Hence, the following stack
     * layout:
     *
     * ----- stack top -----
     * ip arg                           <---+
     * ip return address                <---+- c calling conventions for func ip
     * EFLAGS (4B)                      <--+
     * CS (with 2B pading, 4B in total)    |
     * EIP/return pointer (4B)          <--+- iret implicit pops
     * Interrupt number (4B, unused)    <-+
     * Error code (4B, unused)            |
     * 7 * 4B general purpose registers <-+- explicit pops in common_interrupt_handler
     * --- stack bottom (esp) -----
     */
    regs->esp = (uintptr_t)stack_top - 56;

    *(uint32_t*)((uintptr_t)regs->esp + 36) = (uint32_t)ip;
    *(uint32_t*)((uintptr_t)regs->esp + 40) = (uint32_t)cs;
    *(uint32_t*)((uintptr_t)regs->esp + 44) = eflags;
    *(uint32_t*)((uintptr_t)regs->esp + 52) = (uint32_t)arg;

    // Set esp0 == esp for now, may change when we introduce a user-space
    regs->esp0 = regs->esp;

    // All kernel processes shares the same page tables
    regs->cr3 = get_cr3();
}

/* Create the thread registers for the initial thread */
void init_initial_thread_regs(struct thread_regs* regs)
{
    regs->esp  = 0;  // Will be correctly assigned at context switch
    regs->esp0 = 0;
    regs->cr3  = get_cr3();
}
