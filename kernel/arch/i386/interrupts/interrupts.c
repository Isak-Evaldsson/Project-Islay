#include "interrupts.h"

#include <arch/i386/interrupts/pic.h>
#include <arch/i386/interrupts/ps2.h>
#include <arch/interrupt.h>
#include <klib/klib.h>
#include <stdint.h>

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
    The registers pushed on the stack in the order specified by instruction 'pushad',
    see https://c9x.me/x86/html/file_module_x86_id_270.html
*/
typedef struct cpu_state_t {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
} __attribute__((packed)) cpu_state_t;

/*
    Top of stack once the interrupt is called
*/
typedef struct stack_state_t {
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} __attribute__((packed)) stack_state_t;

void set_interrupt_descriptor(uint8_t index, uint32_t isr_addr)
{
    interrupt_descriptor_t* entry = idt + index;

    entry->offset_low      = isr_addr & 0xffff;
    entry->selector        = 0x08;  // kernel code segment on index 1
    entry->reserved        = 0x00;
    entry->type_attributes = (0x01 << 7) |                // P - set present
                             (0x00 << 6) | (0x00 << 5) |  // DPL - set ring 0
                             0xe;                         // 32-bit interrupt gate
    entry->offset_high = (isr_addr >> 16) & 0xffff;
}

void init_interrupts()
{
    // TODO: fill interrupt table...
    set_interrupt_descriptor(0, (uint32_t)interrupt_handler_0);
    set_interrupt_descriptor(14, (uint32_t)interrupt_handler_14);
    set_interrupt_descriptor(PS2_KEYBOARD_INTERRUPT, (uint32_t)interrupt_handler_33);

    // Load interrupt table
    gdt_ptr_t ptr;
    ptr.size    = sizeof(idt) - 1;
    ptr.address = (uint32_t)&idt;
    load_idt(&ptr);

    // PIC setup
    pic_irq_disable_all();  // Each individual device is responsible for interrupt handling
    pic_remap(PIC1_START_INTERRUPT, PIC2_START_INTERRUPT);

    // PS/2 keyboard setup (maybe to be moved somewhere else)
    pic_irq_enable(PS2_KEYBOARD_IRQ_NUM);
    ps2_init();

    asm volatile("sti");  // enable interrupts
    kprintf("Interrupts initalized\n");
}

/*
    Generic interrupt handler, called from the 'common_interrupt_handler' in 'interrupt_handler.S'
*/
void interrupt_handler(cpu_state_t registers, uint32_t interrupt_number, stack_state_t stack)
{
    // avoid unused warnings
    (void)registers;
    (void)stack;

    /*
        TODO:
            * handle more interrupts
            * handle spurious interrupts from PIC
    */
    switch (interrupt_number) {
        case 0:
            kpanic("Division by zero in kernel at 0x%x\n", stack.eip);
            break;

        case 14:
            kpanic("Page fault at (0x%x) not currently handled, error code %x\n", stack.eip,
                   stack.error_code);
            break;

        case PS2_KEYBOARD_INTERRUPT:
            unsigned char scancode = ps2_read_scancode();
            kprintf("Received scancode '%u' from keyboard input\n", scancode);
            pic_acknowledge(interrupt_number);
            break;

        default:
            kprintf("Received interrupt %u\n", interrupt_number);
            break;
    }
}