/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_i686_GDT_H
#define ARCH_i686_GDT_H

#include <stdint.h>

/*
    Properly formatted pointer for the gdt/idt tables
*/
typedef struct gdt_ptr_t {
    uint16_t size;     // The upper 16 bits of all selector limits.
    uint32_t address;  // The address of the first gdt_entry_t struct.
} __attribute__((packed)) gdt_ptr_t;

/*
    Assembly routine for properly loading the GDT registers (see load_gdt.S),
*/
extern void load_gdt(gdt_ptr_t *ptr);

#endif /* ARCH_i686_GDT_H */
