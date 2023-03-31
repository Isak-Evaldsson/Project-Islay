#ifndef ARCH_i386_PIC_H
#define ARCH_i386_PIC_H
#include <stdint.h>

/* PIC IO ports */
#define PIC1         0x20 /* Primary PIC IO base address */
#define PIC2         0xA0 /* Secondary PIC IO base address */
#define PIC1_COMMAND PIC1
#define PIC1_DATA    (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA    (PIC2 + 1)

/* PIC interrupt vectors */
#define PIC1_START_INTERRUPT 0x20
#define PIC2_START_INTERRUPT 0x28
#define PIC2_END_INTERRUPT   PIC2_START_INTERRUPT + 7

/* PIC commands */
#define PIC_ACK 0x20 /** Acknowledge PIC command */

#define ICW1_ICW4      0x01 /* ICW4 (not) needed */
#define ICW1_SINGLE    0x02 /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL     0x08 /* Level triggered (edge) mode */
#define ICW1_INIT      0x10 /* Initialization - required! */

#define ICW4_8086       0x01 /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02 /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08 /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM       0x10 /* Special fully nested (not) */
#define PIC_READ_IRR    0x0a /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR    0x0b /* OCW3 irq service next CMD read */

/*
    Acknowledge the interrupts sent by the PIC
*/
void pic_acknowledge(unsigned int irq_num);

/*
    Reinitialize the PIC controllers, giving them specified vector offsets
    rather than 8h and 70h, as configured by default
*/
void pic_remap(unsigned int offset1, unsigned int offset2);

/*
    Disables all the irq lines on the pic
*/
void pic_irq_disable_all();

/*
    Enable interrupts at a irq number
*/
void pic_irq_enable(uint8_t irq_num);

/*
    Disable interrupts at a irq number
*/
void pic_irq_disable(uint8_t irq_num);

/*
    Returns the combined value of the cascaded PICs in-service register,
    low byte for PIC1 and upper for PIC2
*/
uint16_t pic_get_isr();

#endif /* ARCH_i386_PIC_H */
