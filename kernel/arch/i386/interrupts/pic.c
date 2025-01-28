/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/i386/io.h>
#include <uapi/errno.h>
#include <utils.h>

#include "pic.h"

#define N_PIC_INTERRUPTS 16

/* Stores the ISR associated with the different pic interrupt numbers */
static top_half_handler_t handlers[N_PIC_INTERRUPTS];

/*
    Acknowledge the interrupts sent by the PIC
*/
void pic_acknowledge(unsigned int irq_num)
{
    // Check that the interrupt is within range
    if (irq_num > 15) {
        return;
    }

    if (irq_num >= 8)
        outb(PIC2_COMMAND, PIC_ACK);

    // Primary must always be ack:ed
    outb(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(unsigned int offset1, unsigned int offset2)
{
    uint8_t mask1, mask2;

    // save masks
    mask1 = inb(PIC1_DATA);
    mask2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND,
         ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC1_DATA, offset1);  // ICW2: Primary PIC vector offset
    outb(PIC2_DATA, offset2);  // ICW2: Secondary PIC vector offset
    outb(PIC1_DATA, 4);  // ICW3: tell Primary PIC that there is a secondary PIC at IRQ2 (0000 0100)
    outb(PIC2_DATA, 2);  // ICW3: tell secondary PIC its cascade identity (0000 0010)
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    // restore saved masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void pic_irq_disable_all()
{
    outb(PIC1_DATA, 0xff);
    outb(PIC2_DATA, 0xff);
}

void pic_irq_enable(uint8_t irq_num)
{
    // Invalid irq numbers
    if (irq_num >= N_PIC_INTERRUPTS)
        return;

    uint16_t port  = (irq_num < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t  value = inb(port) & ~(1 << irq_num);
    outb(port, value);
}

void pic_irq_disable(uint8_t irq_num)
{
    // Invalid irq numbers
    if (irq_num >= N_PIC_INTERRUPTS)
        return;

    uint16_t port  = (irq_num < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t  value = inb(port) | (1 << irq_num);
    outb(port, value);
}

static uint16_t pic_get_irq_reg(int ocw3)
{
    /* OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
     * represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain */
    outb(PIC1_COMMAND, ocw3);
    outb(PIC2_COMMAND, ocw3);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC2_COMMAND);
}

uint16_t pic_get_isr()
{
    return pic_get_irq_reg(PIC_READ_ISR);
}

/* Generic pic top half ensuring proper handling of the pic hardware */
static void pic_top_half_isr(struct interrupt_stack_state *state, uint32_t interrupt_number)
{
    (void)state;

    kassert(interrupt_number >= PIC1_START_INTERRUPT && interrupt_number <= PIC2_END_INTERRUPT);

    unsigned int irq = interrupt_number - PIC1_START_INTERRUPT;
    uint16_t     isr = pic_get_isr();

    // Spurious interrupt sent form the PIC 1, ignore it
    if (irq == 7 && (isr & (1 << 7))) {
        return;
    }

    // Spurious interrupt sent form the PIC 2, ignore it
    if (irq == 15 && (isr & (1 << 15))) {
        pic_acknowledge(0);  // Acknowledge PIC1 only
    }

    top_half_handler_t handler = handlers[irq];
    if (handler) {
        handler(state, interrupt_number);
    }

    pic_acknowledge(irq);
}

/*
    Register interrupts for the pic irq number, wraps the regular interrupt registration api with
    some additional logic to make sure that the PIC is correctly configured
*/
int pic_register_interrupt(uint32_t irq_num, top_half_handler_t top_half,
                           bottom_half_handler_t bottom_half)
{
    if (irq_num > N_PIC_INTERRUPTS) {
        return -EINVAL;
    }

    if (handlers[irq_num]) {
        return -EALREADY;
    }

    // To make sure proper handling of spurious interrupts and acknowledgement, the top half is
    // indirect call through the generic pic top half isr
    handlers[irq_num] = top_half;
    pic_irq_enable(irq_num);

    return register_interrupt_handler(PIC1_START_INTERRUPT + irq_num, pic_top_half_isr,
                                      bottom_half);
}
