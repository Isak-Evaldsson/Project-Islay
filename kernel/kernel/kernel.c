#include <kernel/gdt.h>
#include <kernel/tty.h>
#include <stdio.h>

void kernel_main()
{
    term_init();
    init_gdt();

    // kernel header
    printf("Project Islay, version 0.0.1 (pre-alpha)\n");
    for (size_t i = 0; i < 80; i++) putchar('=');

    printf("Kernel successfully started at virtual addr 0xE0100000 (3.5 GiB + 1 MiB)");
}