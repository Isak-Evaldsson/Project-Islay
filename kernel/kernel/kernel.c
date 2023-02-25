#include <kernel/gdt.h>
#include <kernel/tty.h>
#include <stdio.h>

void print_kernel_header()
{
    printf("Project Islay, version 0.0.1 (pre-alpha)\n");
    for (size_t i = 0; i < TERM_WIDTH; i++) putchar('=');
}

void kernel_main()
{
    term_init();
    printf("Starting boot sequence...\n");
    init_gdt();
    printf("Kernel successfully booted at vaddr 0xE0100000 (3.5 GiB + 1 MiB)\n\n");

    // TODO: nice looking boot animation (requires timers)
    print_kernel_header();
}
