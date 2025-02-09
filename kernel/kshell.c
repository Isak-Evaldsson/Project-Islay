/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/paging.h>
#include <arch/tty.h>
#include <devices/keyboard/ps2_keyboard.h>
#include <fs.h>
#include <memory/page_frame_manager.h>
#include <memory/vmem_manager.h>
#include <stdbool.h>
#include <tasks/scheduler.h>
#include <utils.h>

/*
    Basic kernel shell
*/

static int tty_fd;

/* Internal helper functions */
static void print_help();
static void mem_stats();
static void read_cmd(char *arg);
static void list_cmd();

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
    {.name = "syslist", .description = "list files in sysfs",                 list_cmd  },
};

static void kshell_print(const char *restrict format, ...)
{
    char    str[200];
    va_list args;
    kassert(tty_fd >= 0);

    va_start(args, format);
    vsnprintf(str, sizeof(str), format, args);
    va_end(args);

    write(&scheduler_get_current_task()->fs_data, tty_fd, str, sizeof(str));
}

static void kshell_readline(char *buff, size_t size)
{
    int ret;
    kassert(tty_fd >= 0);

    ret = read(&scheduler_get_current_task()->fs_data, tty_fd, buff, size);
    if (ret < 0) {
        return;
    }

    // ensure null termination and strip newline
    buff[ret] = '\0';
    if (buff[ret - 1] == '\n') {
        buff[ret - 1] = '\0';
    }
}

static void print_kernel_header()
{
    kshell_print("Project Islay, version 0.0.1 (pre-alpha)\n");
    for (int i = 0; i < TERM_WIDTH; i++)
        kshell_print("=");
}

static void print_help()
{
    kshell_print("Available commands:\n");
    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
        kshell_print("   %s: %s\n", commands[i].name, commands[i].description);
}

static void mem_stats()
{
    memory_stats_t mem;
    page_frame_manger_memory_stats(&mem);
    kshell_print("Memory statistics:\n");
    kshell_print("Amount of memory: %u MiB\n", mem.memory_amount >> 20);
    kshell_print("%u of %u available page frames\n", mem.n_available_frames, mem.n_frames);
}

static void list_cmd(char *arg)
{
    int  ret = 0;
    char path[100];

    snprintf(path, sizeof(path), "/%s", arg);
    int fd = open(&scheduler_get_current_task()->fs_data, path, O_DIRECTORY);
    if (fd < 0) {
        kshell_print("failed to open %s, errno -%u\n", path, -fd);
        return;
    }

    struct dirent dirs[10];
    int           count = COUNT_ARRAY_ELEMS(dirs);
    do {
        ret = readdirents(&scheduler_get_current_task()->fs_data, fd, dirs, count);
        if (ret < 0) {
            kshell_print("readdirents failed: errno -%u\n", -ret);
            goto end;
        }

        for (int i = 0; i < ret; i++) {
            kshell_print("(%u): %s\n", dirs[i].d_ino, dirs[i].d_name);
        }
    } while (ret == count);

end:
    int res = close(&scheduler_get_current_task()->fs_data, fd);
    if (res < 0) {
        kshell_print("failed to close fd: %u, errno -%u", fd, -res);
    }
}

static void read_cmd(char *arg)
{
    char path[100];

    snprintf(path, sizeof(path), "/dev/%s", arg);
    int fd = open(&scheduler_get_current_task()->fs_data, path, O_RDONLY);
    if (fd < 0) {
        kshell_print("failed to open %s, errno %i\n", path, fd);
        return;
    }

    ssize_t nbytes = read(&scheduler_get_current_task()->fs_data, fd, buf, PAGE_SIZE - 1);
    if (nbytes < 0) {
        kshell_print("Failed to read fd %u, errno %i\n", fd, nbytes);
        goto end;
    }

    // Ensure that we print a null-terminated string
    buf[nbytes] = '\0';
    kshell_print("%s\n", buf);

end:
    int res = close(&scheduler_get_current_task()->fs_data, fd);
    if (res < 0) {
        kshell_print("failed to close fd: %u, errno -%u", fd, -res);
    }
}

static void parse_command(char *cmd)
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
        kshell_print("Invalid command: %s\n", cmd);
}

void kshell()
{
    char cmd[200];

    tty_fd = open(&scheduler_get_current_task()->fs_data, "/dev/tty1", O_RDWR);
    if (tty_fd < 0) {
        kshell_print("Failed to open tty1\n");
        return;
    }
    print_kernel_header();

    buf = (char *)vmem_request_free_page(0);
    if (!buf) {
        kshell_print("Failed to allocate kshell buffer\n");
        return;
    }

    while (true) {
        kshell_print("kshell> ");
        kshell_readline(cmd, sizeof(cmd));
        parse_command(cmd);
    }
}
