/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_i386_TSS_H
#define ARCH_i386_TSS_H

#include <stdint.h>

/*
    x86 tss structure
*/
typedef struct tss {
    uint32_t link;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldtr;
    uint32_t iopb;
    uint32_t ssp;
} __attribute__((packed)) tss_t;

/* Global variable holding a pointer to the kernel tss */
extern tss_t* kernel_tss;

/*
    Assembly routine for properly loading the TSS
*/
extern void load_tss();

/*
    Initialise the kernel tss
*/
void init_kernel_tss();

/*
    Set pointer to the stack receiving the syscall
*/
void tss_set_stack(uint32_t segment_index, uint32_t sp);

#endif /* ARCH_i386_TSS_H */
