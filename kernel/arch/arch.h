#ifndef ARCH_ARCH_H
#define ARCH_ARCH_H
#include <arch/platfrom.h>

/*
    Header including architecture specific defintions
*/

#if ARCH(i386)
#include <arch/i386/types.h>
#else
#error "Unkown architecture"
#endif

#endif /* ARCH_ARCH_H */