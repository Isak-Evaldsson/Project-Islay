/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_I386_THREAD_H
#define ARCH_I386_THREAD_H

#define THREAD_REGS_ESP_OFFSET  0
#define THREAD_REGS_CR3_OFFSET  4
#define THREAD_REGS_ESP0_OFFSET 8

#ifndef ASM_FILE
#include <utils.h>

// The registers that needs to be stored
struct thread_regs {
    uint32_t esp;   // the contents of esp
    uint32_t cr3;   // the content of cr3
    uint32_t esp0;  // the content of the kernel tss, esp0 field
};

/* Ensure that the asm offset macros are in sync with the struct definition */
assert_offset(struct thread_regs, esp, THREAD_REGS_ESP_OFFSET);
assert_offset(struct thread_regs, cr3, THREAD_REGS_CR3_OFFSET);
assert_offset(struct thread_regs, esp0, THREAD_REGS_ESP0_OFFSET);

#endif /* ASM_FILE*/
#endif /* ARCH_I386_THREAD_H */
