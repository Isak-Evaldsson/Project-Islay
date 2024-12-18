/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/interrupts.h>
#include <stdint.h>
#include <tasks/scheduler.h>
#include <uapi/errno.h>
#include <utils.h>

#include "../drivers/pit.h"
#include "../processor.h"
#include "../segmentation/gdt.h"
#include "pic.h"
#include "ps2.h"

/*
    Interrupt descriptor
*/
typedef struct interrupt_descriptor_t {
    uint16_t offset_low;       // offset bits 0..15
    uint16_t selector;         // a code segment selector in GDT or LDT
    uint8_t  reserved;         // unused, set to 0
    uint8_t  type_attributes;  // gate type, dpl, and p fields
    uint16_t offset_high;      // offset bits 16..31
} __attribute__((packed)) interrupt_descriptor_t;

/*
    Interrupt descriptor table
*/
static interrupt_descriptor_t idt[256];

/*
    Assembly routine for properly loading the ldt registers (see load_idt.S)
    Uses the same pointer formatting as the lgdt instruction
*/
extern void load_idt(gdt_ptr_t *ptr);

/*
    Assembly defined sub-routines, see interrupt_handlers.S.
    Declarations need here in order to avoid compiler errors.
*/
void interrupt_handler_0();
void interrupt_handler_1();
void interrupt_handler_2();
void interrupt_handler_3();
void interrupt_handler_4();
void interrupt_handler_5();
void interrupt_handler_6();
void interrupt_handler_7();
void interrupt_handler_8();
void interrupt_handler_9();
void interrupt_handler_10();
void interrupt_handler_11();
void interrupt_handler_12();
void interrupt_handler_13();
void interrupt_handler_14();
void interrupt_handler_15();
void interrupt_handler_16();
void interrupt_handler_17();
void interrupt_handler_18();
void interrupt_handler_19();
void interrupt_handler_20();
void interrupt_handler_21();
void interrupt_handler_22();
void interrupt_handler_23();
void interrupt_handler_24();
void interrupt_handler_25();
void interrupt_handler_26();
void interrupt_handler_27();
void interrupt_handler_28();
void interrupt_handler_29();
void interrupt_handler_30();
void interrupt_handler_31();
void interrupt_handler_32();
void interrupt_handler_33();
void interrupt_handler_34();
void interrupt_handler_35();
void interrupt_handler_36();
void interrupt_handler_37();
void interrupt_handler_38();
void interrupt_handler_39();
void interrupt_handler_40();
void interrupt_handler_41();
void interrupt_handler_42();
void interrupt_handler_43();
void interrupt_handler_44();
void interrupt_handler_45();
void interrupt_handler_46();
void interrupt_handler_47();
void interrupt_handler_252();
void interrupt_handler_253();
void interrupt_handler_254();
void interrupt_handler_255();

static void set_interrupt_descriptor(uint8_t index, uint32_t isr_addr)
{
    interrupt_descriptor_t *entry = idt + index;

    entry->offset_low      = isr_addr & 0xffff;
    entry->selector        = 0x08;  // kernel code segment on index 1
    entry->reserved        = 0x00;
    entry->type_attributes = (0x01 << 7) |                // P - set present
                             (0x00 << 6) | (0x00 << 5) |  // DPL - set ring 0
                             0xe;                         // 32-bit interrupt gate
    entry->offset_high = (isr_addr >> 16) & 0xffff;
}

#define N_EXCEPTIONS 31

void exception_handler(struct interrupt_stack_state *state, uint32_t interrupt_number)
{
    switch (interrupt_number) {
        case 0:
            kpanic("Division by zero in kernel at 0x%x\n", state->eip);
            break;

        case 14:
            kpanic("Page fault at (0x%x) when accessing address 0x%x error code %x\n", state->eip,
                   get_cr2(), state->error_code);
            break;

        default:
            kprintf("Received exception %u\n", interrupt_number);
            break;
    }
}

int verify_valid_interrupt(unsigned int index)
{
    interrupt_descriptor_t *entry = idt + index;

    if (!MASK_BIT(entry->type_attributes, 7)) {
        return -EINVAL;
    }
    return 0;
}

void init_interrupts()
{
    int ret;

    // Find smarter method to generate handlers
    set_interrupt_descriptor(0, (uint32_t)interrupt_handler_0);
    set_interrupt_descriptor(1, (uint32_t)interrupt_handler_1);
    set_interrupt_descriptor(2, (uint32_t)interrupt_handler_2);
    set_interrupt_descriptor(3, (uint32_t)interrupt_handler_3);
    set_interrupt_descriptor(4, (uint32_t)interrupt_handler_4);
    set_interrupt_descriptor(5, (uint32_t)interrupt_handler_5);
    set_interrupt_descriptor(6, (uint32_t)interrupt_handler_6);
    set_interrupt_descriptor(7, (uint32_t)interrupt_handler_7);
    set_interrupt_descriptor(8, (uint32_t)interrupt_handler_8);
    set_interrupt_descriptor(9, (uint32_t)interrupt_handler_9);
    set_interrupt_descriptor(10, (uint32_t)interrupt_handler_10);
    set_interrupt_descriptor(11, (uint32_t)interrupt_handler_11);
    set_interrupt_descriptor(12, (uint32_t)interrupt_handler_12);
    set_interrupt_descriptor(13, (uint32_t)interrupt_handler_13);
    set_interrupt_descriptor(14, (uint32_t)interrupt_handler_14);
    set_interrupt_descriptor(15, (uint32_t)interrupt_handler_15);
    set_interrupt_descriptor(16, (uint32_t)interrupt_handler_16);
    set_interrupt_descriptor(17, (uint32_t)interrupt_handler_17);
    set_interrupt_descriptor(18, (uint32_t)interrupt_handler_18);
    set_interrupt_descriptor(19, (uint32_t)interrupt_handler_19);
    set_interrupt_descriptor(20, (uint32_t)interrupt_handler_20);
    set_interrupt_descriptor(21, (uint32_t)interrupt_handler_21);
    set_interrupt_descriptor(22, (uint32_t)interrupt_handler_22);
    set_interrupt_descriptor(23, (uint32_t)interrupt_handler_23);
    set_interrupt_descriptor(24, (uint32_t)interrupt_handler_24);
    set_interrupt_descriptor(25, (uint32_t)interrupt_handler_25);
    set_interrupt_descriptor(26, (uint32_t)interrupt_handler_26);
    set_interrupt_descriptor(27, (uint32_t)interrupt_handler_27);
    set_interrupt_descriptor(28, (uint32_t)interrupt_handler_28);
    set_interrupt_descriptor(29, (uint32_t)interrupt_handler_29);
    set_interrupt_descriptor(30, (uint32_t)interrupt_handler_30);
    set_interrupt_descriptor(31, (uint32_t)interrupt_handler_31);
    set_interrupt_descriptor(32, (uint32_t)interrupt_handler_32);
    set_interrupt_descriptor(33, (uint32_t)interrupt_handler_33);
    set_interrupt_descriptor(34, (uint32_t)interrupt_handler_34);
    set_interrupt_descriptor(35, (uint32_t)interrupt_handler_35);
    set_interrupt_descriptor(36, (uint32_t)interrupt_handler_36);
    set_interrupt_descriptor(37, (uint32_t)interrupt_handler_37);
    set_interrupt_descriptor(38, (uint32_t)interrupt_handler_38);
    set_interrupt_descriptor(39, (uint32_t)interrupt_handler_39);
    set_interrupt_descriptor(40, (uint32_t)interrupt_handler_40);
    set_interrupt_descriptor(41, (uint32_t)interrupt_handler_41);
    set_interrupt_descriptor(42, (uint32_t)interrupt_handler_42);
    set_interrupt_descriptor(43, (uint32_t)interrupt_handler_43);
    set_interrupt_descriptor(44, (uint32_t)interrupt_handler_44);
    set_interrupt_descriptor(45, (uint32_t)interrupt_handler_45);
    set_interrupt_descriptor(46, (uint32_t)interrupt_handler_46);
    set_interrupt_descriptor(47, (uint32_t)interrupt_handler_47);

    // For testing purposes
    set_interrupt_descriptor(252, (uint32_t)interrupt_handler_252);
    set_interrupt_descriptor(253, (uint32_t)interrupt_handler_253);
    set_interrupt_descriptor(254, (uint32_t)interrupt_handler_254);
    set_interrupt_descriptor(255, (uint32_t)interrupt_handler_255);

    // Load interrupt table
    gdt_ptr_t ptr;
    ptr.size    = sizeof(idt) - 1;
    ptr.address = (uint32_t)&idt;
    load_idt(&ptr);

    // PIC setup
    pic_irq_disable_all();  // Each individual device is responsible for interrupt handling
    pic_remap(PIC1_START_INTERRUPT, PIC2_START_INTERRUPT);

    ps2_init();
    ret = pic_register_interrupt(PS2_KEYBOARD_INTERRUPT, ps2_interrupt_handler, NULL);
    if (ret < 0) {
        kpanic("x86: Failed to register ps2 controller, error: %i", ret);
    }

    pit_init();
    ret = pic_register_interrupt(PIT_INTERRUPT_NUM, pit_interrupt_handler, NULL);
    if (ret < 0) {
        kpanic("x86: Failed to register pit, error: %i", ret);
    }

    // Register exception handlers
    for (size_t i = 0; i < N_EXCEPTIONS; i++) {
        register_interrupt_handler(i, exception_handler, NULL);
        if (ret < 0) {
            kpanic("x86: Failed to register exception handler number: %u, error: %i", i, ret);
        }
    }

    enable_interrupts();
    kprintf("Interrupts initalized\n");
}

void wait_for_interrupt()
{
    asm volatile("hlt");
}

void enable_interrupts()
{
    asm volatile("sti");
}

void disable_interrupts()
{
    asm volatile("cli");
}

uint32_t get_register_and_disable_interrupts()
{
    unsigned flags;
    asm volatile("pushfl; cli; popl %0" : "=r"(flags)::"memory");
    return flags;
}

void restore_interrupt_register(uint32_t flags)
{
    asm("pushl %0; popfl" ::"r"(flags) : "memory", "cc");
}
