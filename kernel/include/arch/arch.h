/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_ARCH_H
#define ARCH_ARCH_H
#include <arch/platfrom.h>
#include <stdint.h>

/*
    Header including architecture specific defintions
*/

#define ENDIAN_BIG    1
#define ENDIAN_LITTLE 2

#if ARCH(i686)
#define ARCH_ENDIANNESS ENDIAN_LITTLE
#else
#error "Unkown architecture"
#endif

#define VIRTADDR_MAX UINTPTR_MAX
#define PHYSADDR_MAX UINTPTR_MAX

typedef uintptr_t virtaddr_t;
typedef uintptr_t physaddr_t;

/* To initialise arch-specific static devices that can be expected to always be there (e.g.
 * interrupt controllers) */
int arch_initialise_static_devices();

#endif /* ARCH_ARCH_H */
