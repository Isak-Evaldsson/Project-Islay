#include <kernel/tty.h>
#include <stdio.h>

void kernel_main()
{
    term_init();

    // kernel header
    printf("Project Islay, version 0.0.1 (pre-alpha)\n");
    for (size_t i = 0; i < 80; i++) putchar('=');

    printf("Kernel successfully started at addr %u", &kernel_main);
}