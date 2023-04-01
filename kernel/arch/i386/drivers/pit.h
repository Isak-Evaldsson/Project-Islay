#ifndef ARCH_i386_PIT_H
#define ARCH_i386_PIT_H

#include <arch/i386/interrupts/pic.h>
#include <stdbool.h>

#define PIT_INTERRUPT_NUM PIC1_START_INTERRUPT

bool pit_set_frequency(uint32_t freq);
void pit_set_default_frequency();
void pit_init();
void pit_interrupt_handler();

#endif /* ARCH_i386_PIT_H */
