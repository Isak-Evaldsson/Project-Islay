/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_i386_IO_H
#define ARCH_i386_IO_H
#include <stdint.h>

/*
    Sends a byte of data to the specified port
*/
void outb(uint16_t port, uint8_t data);

/*
    Reads a byte of data from the specified port
*/
uint8_t inb(uint16_t port);

#endif /* ARCH_i386_IO_H */
