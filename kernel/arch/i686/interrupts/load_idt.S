/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/

# load_idt - Loads the interrupt descriptor table (IDT).
.global load_idt
.section .text
load_idt:
    mov   4(%esp), %eax
    lidt  (%eax)
    ret
