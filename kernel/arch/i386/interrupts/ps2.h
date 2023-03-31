#ifndef ARCH_i386_PS2_H
#define ARCH_i386_PS2_H

#include <arch/i386/interrupts/pic.h>

#define PS2_KEYBOARD_INTERRUPT (PIC1_START_INTERRUPT + 1)

/*
    Initialises the PS/2 Controller
*/
void ps2_init();

/*
    Interrupt handler to be run when the PS/2 device sends an interrupt through the PIC
*/
void ps2_interrupt_handler();

#endif /* ARCH_i386_PS2_H */
