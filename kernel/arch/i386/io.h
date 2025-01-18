/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_i386_IO_H
#define ARCH_i386_IO_H
#include <atomics.h>
#include <stdint.h>

/*
    Assembly definitons
*/
void    asm_outb(uint16_t port, uint8_t data);
uint8_t asm_inb(uint16_t port);

/*
    Sends a byte of data to the specified port
*/
#define outb(port, data)      \
    ({                        \
        mem_barrier_full();   \
        asm_outb(port, data); \
    })

/*
    Reads a byte of data from the specified port
*/
#define inb(port)           \
    ({                      \
        mem_barrier_full(); \
        asm_inb(port);      \
    })

#endif /* ARCH_i386_IO_H */
