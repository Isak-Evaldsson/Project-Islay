/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_SERIAL_H
#define ARCH_SERIAL_H
#include <stdbool.h>
#include <stddef.h>

/* Initialises the serial port, returns 0 on success */
int serial_init();

/* Writes the supplied char to the serial port */
void serial_put(char c);

/* Writes the supplied string to the serial port */
void serial_write(const char *data, size_t size);

#endif /* ARCH_SERIAL_H */
