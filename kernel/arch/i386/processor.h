#ifndef ARCH_i386_PROCESSOR_H
#define ARCH_i386_PROCESSOR_H
#include <stdint.h>

/*
    Functions to read registers
*/
uint32_t get_esp();
uint32_t get_cr3();

#endif /* ARCH_i386_PROCESSOR_H */