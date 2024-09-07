/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/

#
# Loads the TSS segment at index 5 gdt
# 
.section .text
.global load_tss
load_tss:
    mov $0x28, %ax
    ltr %ax
    ret
