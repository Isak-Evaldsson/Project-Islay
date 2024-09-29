/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_I386_INTERRUPTS_H
#define ARCH_I386_INTERRUPTS_H

/*
    Struct representing the stack state when the generic interrupt handler is called.
    Since the stack grows downwards but struck upwards, the latest pushed value are at offset 0.
*/
struct interrupt_stack_state {
    // Registers pushed to the stack in common_interrupt_handler
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    // Interrupt number pushed to the stack by the interrupt handler macros
    uint32_t int_no;

    // Registers automaticaly pushed to the stack when the interrupt is triggered
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} __attribute__((packed));

#define ARCH_N_INTERRUPTS 256

/* Fetches the interrupt number from the interrupt_stack_state struct */
#define ARCH_GET_INTERRUPT_NUMBER(state)               \
    ({                                                 \
        struct interrupt_stack_state *__state = state; \
        __state->int_no;                               \
    })

/*
    When called within generic_interrupt_handler() it fetches the address for the
    interrupt_stack_state.

    Since the struct represent the stack state before the call, it can be found by adjusting the
    base pointer two steps upwards (stack grows downwards) to compensate for the call
   pushing of the ip and the function prologue pushing old base pointer.
*/
#define ARCH_GET_INTERRUPT_STACK_STATE() __builtin_frame_address(0) + 2 * sizeof(void *)

#endif /* ARCH_I386_INTERRUPTS_H */
