/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/

#
# Short assenbly rutine to load GDT for 32-bit flat model configuration
#
.section .text
.global load_gdt
load_gdt:
    mov   4(%esp), %eax
    lgdt  (%eax)

    ljmp $0x08, $.reload_CS
.reload_CS:
    mov $0x10, %ax # 0x10 is the offset in the GDT to our data segment
    mov %ax, %ds   # Load all data segment selectors
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    ret
