#include "interrupts.h"

#include <arch/i386/drivers/pit.h>
#include <arch/i386/interrupts/pic.h>
#include <arch/i386/interrupts/ps2.h>
#include <arch/interrupt.h>
#include <klib/klib.h>
#include <stdint.h>

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
    Stores the interrupt handler associated with each interrupt number
*/
static interrupt_handler_t handlers[256];

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
    memset(handlers, 0, sizeof(handlers));

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

    // Load interrupt table
    gdt_ptr_t ptr;
    ptr.size    = sizeof(idt) - 1;
    ptr.address = (uint32_t)&idt;
    load_idt(&ptr);

    // PIC setup
    pic_irq_disable_all();  // Each individual device is responsible for interrupt handling
    pic_remap(PIC1_START_INTERRUPT, PIC2_START_INTERRUPT);

    ps2_init();
    register_interrupt(PS2_KEYBOARD_INTERRUPT, ps2_interrupt_handler);

    pit_init();
    register_interrupt(PIT_INTERRUPT_NUM, pit_interrupt_handler);

    enable_interrupts();
    kprintf("Interrupts initalized\n");
}

int register_interrupt(unsigned char num, interrupt_handler_t handler)
{
    // TODO: proper error handling

    if (handlers[num] != NULL) {
        // Throw error????
        kprintf("Overriding existing interrupt handler at num %u\n", num);
    }
    handlers[num] = handler;

    // register hardware interrupt managed by the pic controller
    if (num >= PIC1_START_INTERRUPT && num <= PIC2_END_INTERRUPT) {
        pic_irq_enable(num - PIC1_START_INTERRUPT);
    }

    return 1;
}

/*
    Generic interrupt handler, called from the 'common_interrupt_handler' in 'interrupt_handler.S'
*/
void interrupt_handler(cpu_state_t registers, uint32_t interrupt_number, stack_state_t stack)
{
    // avoid unused warnings
    (void)registers;
    (void)stack;

    // hardware interrupts managed by the pic controller
    if (interrupt_number >= PIC1_START_INTERRUPT && interrupt_number <= PIC2_END_INTERRUPT) {
        uint8_t irq = interrupt_number - PIC1_START_INTERRUPT;

        // Spurious interrupt sent form the PIC 1, ignore it
        if (irq == 7 && (pic_get_isr() & (1 << 7))) {
            return;
        }

        // Spurious interrupt sent form the PIC 2, ignore it
        if (irq == 15 && (pic_get_isr() & (1 << 15))) {
            pic_acknowledge(0);  // Acknowledge PIC1 only
        }

        interrupt_handler_t handler = handlers[interrupt_number];
        if (handler != NULL) {
            handler();
        } else {
            kprintf("Received interrupt %u without registered handler\n", interrupt_number);
        }

        pic_acknowledge(irq);

    } else if (interrupt_number < 31) {
        // TODO: Exception handling

        switch (interrupt_number) {
            case 0:
                kpanic("Division by zero in kernel at 0x%x\n", stack.eip);
                break;

            case 14:
                kpanic("Page fault at (0x%x) not currently handled, error code %x\n", stack.eip,
                       stack.error_code);
                break;

            default:
                kprintf("Received interrupt %u\n", interrupt_number);
                break;
        }
    } else {
        // TODO: Software interrupt
    }
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

void resture_interrupt_register(uint32_t flags)
{
    asm("pushl %0; popfl" ::"r"(flags) : "memory", "cc");
}
