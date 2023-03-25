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
	# save the registers
	push %eax
	push %ebx
	push %ecx
	push %edx
	push %ebp
	push %esi
	push %edi

    # call the C function
    call    interrupt_handler

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
error_code_interrupt_handler 14 	# page fault