/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/serial.h>
#include <utils.h>

#include "internal.h"

static int serial_putchar(int ic)
{
    unsigned char c = (char)ic;
    serial_put(c);
    return ic;
}

/* Logs formatted message to serial output */
int log(const char *restrict format, ...)
{
    int                ret;
    va_list            args;
    struct fwriter_ops ops = {
        .type = FWRITER_CHARDEV,
        .args = {.putchar = serial_putchar},
    };

    va_start(args, format);
    ret = __fwriter(&ops, format, args);
    va_end(args);

    // Add proper line endings
    serial_put('\r');
    serial_put('\n');

    return ret;
}
