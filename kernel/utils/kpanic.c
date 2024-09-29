/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/interrupts.h>
#include <libc.h>
#include <utils.h>

__attribute__((__noreturn__)) void kpanic(const char *restrict format, ...)
{
    va_list args;

    // Panic header
    kprintf("kernel panic: ");

    // Message
    va_start(args, format);
    kvprintf(format, args);
    va_end(args);

    // Stop the kernel
    while (1) {
        disable_interrupts();
        wait_for_interrupt();
    }

    __builtin_unreachable();
}
