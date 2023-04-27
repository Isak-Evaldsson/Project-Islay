#ifndef ARCH_INTERRUPT_H
#define ARCH_INTERRUPT_H

typedef void (*interrupt_handler_t)();

void init_interrupts();

int register_interrupt(unsigned char num, interrupt_handler_t handler);

#endif /* ARCH_INTERRUPT_H */