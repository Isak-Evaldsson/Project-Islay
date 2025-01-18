/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/

# outb - Writes a byte of data to the specified port
# args: 8(%esp) - the byte of data
#       4(%esp) - the port address
.section .text
.global asm_outb
asm_outb:
    mov 8(%esp), %al
    mov 4(%esp), %dx
    out %al, %dx
    ret

# inb - Reads a byte of data to the specified port
# args: 4(%esp) - the port address
.global asm_inb
asm_inb:
    mov 4(%esp), %dx
    in %dx, %al
    ret
