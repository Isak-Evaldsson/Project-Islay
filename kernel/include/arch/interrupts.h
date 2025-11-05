/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_INTERRUPT_H
#define ARCH_INTERRUPT_H
#include <arch/platfrom.h>
#include <stdbool.h>
#include <stdint.h>

#if ARCH(i686)
#include "i686/interrupts.h"
#endif

/* Architecture dependent struct representing the stack state when generic_interrupt_handler is
 * called */
typedef struct interrupt_stack_state interrupt_stack_state_t;

/* First part of the interrupt, runs in an atomic state (i.e. interrupts are disabled) and must
 * therefore be fast. It's therefore NOT allowed to block or sleep under any circumstances */
typedef void (*top_half_handler_t)(struct interrupt_stack_state *state, uint32_t interrupt_number);

/* Second part of the interrupt, runs in an reentrant state (i.e. can be preempted) so it's allowed
 * to block or sleep */
typedef void (*bottom_half_handler_t)(uint32_t interrupt_number);

/*
    Registers an interrupt at the specified interrupt number. The caller needs to provide at least a
    top or a bottom half handler.
*/
int register_interrupt_handler(uint32_t interrupt_number, top_half_handler_t top_half,
                               bottom_half_handler_t bottom_half);

/*
    Provides an architecture independet interrupt mechanism proving a atomic top half and a
    reentrant bottom half. Is supposed to be called by the arch specific low level interrupt code.

    How this is called is up to the arch specific low level interrupt code, however it assumes it
    runs in an atomic context (i.e. INTERRUPTS MUST BE DISABLED).
 */
void generic_interrupt_handler();

/*
   TODO: Split header into two, one or arch based functions and one for common defs?
*/
void init_interrupts();

void wait_for_interrupt();

void enable_interrupts();

void disable_interrupts();

uint32_t get_register_and_disable_interrupts();

void restore_interrupt_register(uint32_t);

int verify_valid_interrupt(unsigned int index);

bool interrupts_enabled();

#endif /* ARCH_INTERRUPT_H */
