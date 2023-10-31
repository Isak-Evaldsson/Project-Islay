#ifndef ARCH_ARCH_H
#define ARCH_ARCH_H
#include <arch/platfrom.h>

/*
    Header including architecture specific defintions
*/

#if ARCH(i386)
#include <stdint.h>
typedef uint32_t virtaddr_t;
typedef uint32_t physaddr_t;
#else
#error "Unkown architecture"
#endif

#endif /* ARCH_ARCH_H */