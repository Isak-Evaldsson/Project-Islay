#include <arch/tty.h>
#include <klib/klib.h>
#include <memory/page_frame_manager.h>
#include <stdbool.h>

/*
    Basic kernel shell
*/

// Internal helper function
static void print_help();
static void mem_stats();

typedef struct command_t {
    const char *name;
    const char *description;
    void (*function)();
} command_t;

static command_t commands[] = {
    {.name = "help",    .description = "prints all available shell commands", print_help},
    {.name = "clear",   .description = "clears the terminal window",          term_clear},
    {.name = "memstat", .description = "show kernel memory statistics",       mem_stats },
};

static void print_kernel_header()
{
    kprintf("Project Islay, version 0.0.1 (pre-alpha)\n");
    for (int i = 0; i < TERM_WIDTH; i++) kprintf("=");
}

static void print_help()
{
    kprintf("Available commands:\n");
    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
        kprintf("   %s: %s\n", commands[i].name, commands[i].description);
}

static void mem_stats()
{
    memory_stats_t mem;
    page_frame_manger_memory_stats(&mem);
    kprintf("Memory statistics:\n");
    kprintf("Amount of memory: %u MiB\n", mem.memory_amount >> 20);
    kprintf("%u of %u available page frames\n", mem.n_available_frames, mem.n_frames);
}

static void parse_command(const char *cmd)
{
    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        command_t command = commands[i];

        if (strcmp(cmd, command.name) == 0) {
            command.function();
            return;
        }
    }

    if (strcmp(cmd, "clear")) kprintf("Invalid command: %s\n", cmd);
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