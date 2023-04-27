#include <arch/tty.h>
#include <klib/klib.h>
#include <stdbool.h>

/*
    Basic kernel shell
*/

static void print_kernel_header()
{
    kprintf("Project Islay, version 0.0.1 (pre-alpha)\n");
    for (int i = 0; i < TERM_WIDTH; i++) kprintf("=");
}

static void print_help()
{
    kprintf("Available commands:\n");
    kprintf("   help: prints all available shell commands\n");
}

static void parse_command(const char* cmd)
{
    if (strcmp(cmd, "help") == 0) {
        print_help();
        return;
    }

    kprintf("Invalid command: %s\n", cmd);
}

void kshell()
{
    char cmd[200];

    print_kernel_header();
    while (true) {
        kprintf("> ");
        kreadline(sizeof(cmd), cmd);
        parse_command(cmd);
    }
}