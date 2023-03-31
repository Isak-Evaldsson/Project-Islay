#include <arch/gdt.h>
#include <arch/interrupt.h>
#include <arch/tty.h>
#include <klib/klib.h>

void print_kernel_header()
{
    kprintf("Project Islay, version 0.0.1 (pre-alpha)\n");
    for (size_t i = 0; i < TERM_WIDTH; i++) kprintf("=");
}

void kernel_main()
{
    term_init();
    kprintf("Starting boot sequence...\n");
    init_gdt();
    init_interrupts();
    kprintf("Kernel successfully booted at vaddr 0xE0100000 (3.5 GiB + 1 MiB)\n\n");

    // TODO: nice looking boot animation (requires timers)
    print_kernel_header();
    // asm("int $33");

    // Keep kernel alive, waiting for interrupts
    for (;;) {
        asm("hlt");
    }
}
