/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/

# Generic Interrupt Handler
.extern interrupt_handler

.macro no_error_code_interrupt_handler num
.global interrupt_handler_\num
interrupt_handler_\num:
	pushl	$0                   # push 0 as error code
	pushl	$\num                 # push the interrupt number
	jmp	common_interrupt_handler    # jump to the common handler
.endm

.macro error_code_interrupt_handler num
.global interrupt_handler_\num
interrupt_handler_\num:
	push    $\num                    # push the interrupt number
	jmp     common_interrupt_handler    # jump to the common handler
.endm

.section .text
common_interrupt_handler:               # the common parts of the generic interrupt handler
	# save the registers, pushed on the stack in the order specified by instruction 'pushad',
    # see https://c9x.me/x86/html/file_module_x86_id_270.html
	push %eax
	push %ebx
	push %ecx
	push %edx
	push %ebp
	push %esi
	push %edi

    # call the C function
    call generic_interrupt_handler

    # restore the registers
	pop	%edi
	pop	%esi
	pop	%ebp
	pop	%edx
	pop	%ecx
	pop	%ebx
    pop %eax

	# restore the esp
	add $8, %esp

	# return to the code that got interrupted
	iret


# Create generic interrupt handler here
.section .text
no_error_code_interrupt_handler 0 	# division by zero
no_error_code_interrupt_handler 1
no_error_code_interrupt_handler 2
no_error_code_interrupt_handler 3
no_error_code_interrupt_handler 4
no_error_code_interrupt_handler 5
no_error_code_interrupt_handler 6
no_error_code_interrupt_handler 7
error_code_interrupt_handler 8
no_error_code_interrupt_handler 9
error_code_interrupt_handler 10
error_code_interrupt_handler 11
error_code_interrupt_handler 12
error_code_interrupt_handler 13
error_code_interrupt_handler 14
no_error_code_interrupt_handler 15
no_error_code_interrupt_handler 16
error_code_interrupt_handler 17
no_error_code_interrupt_handler 18
no_error_code_interrupt_handler 19
no_error_code_interrupt_handler 20
error_code_interrupt_handler 21
no_error_code_interrupt_handler 22
no_error_code_interrupt_handler 23
no_error_code_interrupt_handler 24
no_error_code_interrupt_handler 25
no_error_code_interrupt_handler 26
no_error_code_interrupt_handler 27
no_error_code_interrupt_handler 28
error_code_interrupt_handler 29
error_code_interrupt_handler 30
no_error_code_interrupt_handler 31
no_error_code_interrupt_handler 32
no_error_code_interrupt_handler 33
no_error_code_interrupt_handler 34
no_error_code_interrupt_handler 35
no_error_code_interrupt_handler 36
no_error_code_interrupt_handler 37
no_error_code_interrupt_handler 38
no_error_code_interrupt_handler 39
no_error_code_interrupt_handler 40
no_error_code_interrupt_handler 41
no_error_code_interrupt_handler 42
no_error_code_interrupt_handler 43
no_error_code_interrupt_handler 44
no_error_code_interrupt_handler 45
no_error_code_interrupt_handler 46
no_error_code_interrupt_handler 47
no_error_code_interrupt_handler 252
no_error_code_interrupt_handler 253
no_error_code_interrupt_handler 254
no_error_code_interrupt_handler 255
