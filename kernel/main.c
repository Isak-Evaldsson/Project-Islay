#include <arch/gdt.h>
#include <arch/interrupt.h>
#include <arch/serial.h>
#include <arch/tty.h>
#include <klib/klib.h>
#include <kshell.h>

void kernel_main()
{
    if (serial_init() == 1) {
        kpanic("No serial\n");
    }

    term_init();
    kprintf("Starting boot sequence...\n");
    init_gdt();
    init_interrupts();
    kprintf("Kernel successfully booted at vaddr 0xE0100000 (3.5 GiB + 1 MiB)\n\n");

    // TODO: nice looking boot animation (requires timers)
    kshell();

    // Keep kernel alive, waiting for interrupts
    for (;;) {
        wait_for_interrupt();
    }
}
