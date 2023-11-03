#ifndef ARCH_INTERRUPT_H
#define ARCH_INTERRUPT_H
#include <stdint.h>

typedef void (*interrupt_handler_t)();

void init_interrupts();

int register_interrupt(unsigned char num, interrupt_handler_t handler);

void wait_for_interrupt();

void enable_interrupts();

void disable_interrupts();

uint32_t get_register_and_disable_interrupts();

void restore_interrupt_register(uint32_t);

#endif /* ARCH_INTERRUPT_H */