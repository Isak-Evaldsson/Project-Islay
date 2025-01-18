/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_i386_PS2_H
#define ARCH_i386_PS2_H

#include <arch/interrupts.h>

#define PS2_KEYBOARD_INTERRUPT 1  // relative to pic

/*
    Initialises the PS/2 Controller
*/
void ps2_init();

/*
    Interrupt handlers to be run when the PS/2 device sends an interrupt through the PIC
*/
void ps2_top_irq(struct interrupt_stack_state *state, uint32_t interrupt_number);
void ps2_bottom_irq(uint32_t irq_no);

#endif /* ARCH_i386_PS2_H */
