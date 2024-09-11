/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_I386_THREAD_H
#define ARCH_I386_THREAD_H

// The registers that needs to be stored
struct thread_regs {
    uint32_t esp;   // the contents of esp
    uint32_t cr3;   // the content of cr3
    uint32_t esp0;  // the content of the kernel tss, esp0 field
};

#endif /* ARCH_I386_THREAD_H */
