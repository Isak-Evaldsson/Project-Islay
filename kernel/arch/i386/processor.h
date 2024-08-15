#ifndef ARCH_I386_PROCESSOR_H
#define ARCH_I386_PROCESSOR_H
#include <stdint.h>

/*
    Functions to read registers
*/
uint32_t get_esp();
uint32_t get_cr2();
uint32_t get_cr3();

#endif /* ARCH_I386_PROCESSOR_H */
