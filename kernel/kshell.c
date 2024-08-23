#include <arch/paging.h>
#include <arch/tty.h>
#include <devices/ps2_keyboard.h>
#include <fs.h>
#include <memory/page_frame_manager.h>
#include <memory/vmem_manager.h>
#include <stdbool.h>
#include <tasks/scheduler.h>
#include <utils.h>

/*
    Basic kernel shell
*/

/* Internal helper functions */
static void print_help();
static void mem_stats();
static void read_cmd(char *arg);

typedef struct command_t {
    const char *name;
    const char *description;
    void (*function)(char *arg);
} command_t;

static char *buf;

static command_t commands[] = {
    {.name = "help",    .description = "prints all available shell commands", print_help},
    {.name = "clear",   .description = "clears the terminal window",          term_clear},
    {.name = "memstat", .description = "show kernel memory statistics",       mem_stats },
    {.name = "sysread", .description = "read file from sysfs",                read_cmd  },
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

static void read_cmd(char *arg)
{
    char path[100];

    snprintf(path, sizeof(path), "/sys/%s", arg);
    int fd = open(&scheduler_get_current_task()->fs_data, path, 0);
    if (fd < 0) {
        kprintf("failed to open %s, errno -%u\n", path, -fd);
        return;
    }

    ssize_t nbytes = read(&scheduler_get_current_task()->fs_data, fd, buf, PAGE_SIZE - 1);
    if (nbytes < 0) {
        kprintf("Failed to read fd, errno -%u\n", fd, -nbytes);
        goto end;
    }

    // Ensure that we print a null-terminated string
    buf[nbytes] = '\0';
    kprintf("%s", buf);

end:
    int res = close(&scheduler_get_current_task()->fs_data, fd);
    if (res < 0) {
        kprintf("failed to close fd: %u, errno -%u", fd, -res);
    }
}

static void parse_command(const char *cmd)
{
    char *save_ptr;
    char *name = strtok(cmd, " ", &save_ptr);
    char *arg  = strtok(NULL, " ", &save_ptr);

    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        command_t command = commands[i];

        if (strcmp(name, command.name) == 0) {
            command.function(arg);
            return;
        }
    }

    if (strcmp(cmd, "clear"))
        kprintf("Invalid command: %s\n", cmd);
}

void kshell()
{
    char cmd[200];

    print_kernel_header();

    buf = vmem_request_free_page(0);
    if (!buf) {
        kprintf("Failed to allocate kshell buffer\n");
        return;
    }

    while (true) {
        kprintf("> ");
        kreadline(sizeof(cmd), cmd);
        parse_command(cmd);
    }
}
