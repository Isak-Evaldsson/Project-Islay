/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_i386_PS2_H
#define ARCH_i386_PS2_H

#define PS2_KEYBOARD_INTERRUPT 1  // relative to pic

/*
    Initialises the PS/2 Controller
*/
void ps2_init();

/*
    Interrupt handler to be run when the PS/2 device sends an interrupt through the PIC
*/
void ps2_interrupt_handler();

#endif /* ARCH_i386_PS2_H */
