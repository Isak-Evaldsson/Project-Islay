/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_ARCH_H
#define ARCH_ARCH_H
#include <arch/platfrom.h>

/*
    Header including architecture specific defintions
*/

#define ENDIAN_BIG    1
#define ENDIAN_LITTLE 2

#if ARCH(i386)
#include <stdint.h>
typedef uint32_t virtaddr_t;
typedef uint32_t physaddr_t;

#define ARCH_ENDIANNESS ENDIAN_LITTLE
#else
#error "Unkown architecture"
#endif

/* To initialise arch-specific static devices that can be expected to always be there (e.g.
 * interrupt controllers) */
int arch_initialise_static_devices();

#endif /* ARCH_ARCH_H */
