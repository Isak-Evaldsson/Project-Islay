/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_BOOT_H
#define ARCH_BOOT_H
#include <arch/arch.h>
#include <arch/sections.h>
#include <stddef.h>
#include <stdint.h>

/*
    Kernel linker script is required to expose a start and end address for the kernel
*/
#define KERNEL_START     GET_LINKER_SYMBOL(_kernel_start)       /* start symbol, assumed to be physical address */
#define KERNEL_END       GET_LINKER_SYMBOL(_kernel_end)         /* end symbol, virtual address since higher half kernel */
#define HIGHER_HALF_ADDR GET_LINKER_SYMBOL(_higher_half_addr)   /* indicating the start of higher half area */

/* Architecture independent boot data */
struct boot_data {
    // Initrd
    physaddr_t initrd_start;
    size_t     initrd_size;
};

#endif /* ARCH_BOOT_H */
