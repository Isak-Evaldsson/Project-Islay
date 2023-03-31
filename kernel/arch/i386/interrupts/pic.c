#include <arch/i386/interrupts/pic.h>
#include <arch/i386/io.h>

/*
    Acknowledge the interrupts sent by the PIC
*/
void pic_acknowledge(unsigned int irq_num)
{
    // Check that the interrupt is within range
    if (irq_num > 15) {
        return;
    }

    if (irq_num >= 8) outb(PIC2_COMMAND, PIC_ACK);

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
    if (irq_num >= 16) return;

    uint16_t port  = (irq_num < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t  value = inb(port) & ~(1 << irq_num);
    outb(port, value);
}

void pic_irq_disable(uint8_t irq_num)
{
    // Invalid irq numbers
    if (irq_num >= 16) return;

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
