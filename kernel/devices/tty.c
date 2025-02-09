/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/

#include <arch/paging.h>
#include <devices/display/text_mode_display.h>
#include <devices/input_manager.h>
#include <devices/tty.h>
#include <memory/vmem_manager.h>
#include <tasks/scheduler.h>

#include "internals.h"
#include "keyboard/keyboard.h"

#define TTY_MODE_CANONICAL 0x01

#define LOG(fmt, ...) __LOG(1, "[TTY]", fmt, ##__VA_ARGS__)

struct tty {
    struct device            device;
    struct text_mode_device* text_mode_dev;
    struct input_subscriber  subscriber;
    unsigned char            mode;

    // TODO: Replace with a task id so when so we are force to get the
    // pointer on every interacting, otherwise we might get problems if
    // it points to a dead process
    struct task*      waiting_proc;
    struct open_file* opened;

    // Escape codes spans multiple bytes, so this serves as an intermidiate buffer until they are
    // fully parsed, it's not until then know if they fit into the character buffer
    char   escape_code_buffer[10];
    size_t escape_code_idx;
    bool   parsing_escape_code;

    // Ring buffers containing all chars read to be read. In order to have line editing in canonical
    // mode it's split into two halfs, the commited part [read_idx, commited_idx] read to be read
    // and the current line part [commited_idx, write_idx] which can be edited
    size_t char_buffer_read_idx;
    size_t char_buffer_write_idx;
    size_t char_buffer_commit_idx;
    char*  char_buffer;

    bool         keys_dropped;     // Has the keys been dropped due to the char buffer being full
    unsigned int saved_kbd_state;  // Keyboard state for that tty
};

#define MAX_TTYS 8
static struct tty  tty_table[MAX_TTYS];
static size_t      num_ttys;
static struct tty* current_tty;

static void tty_switch(size_t index)
{
    struct tty* old = current_tty;
    if (index >= num_ttys) {
        return;
    }

    LOG("Switch from %u to %u", old ? old : 0, index);
    if (old) {
        input_manger_unsubscribe(&old->subscriber);
        old->saved_kbd_state = get_keyboard_state();
    }

    current_tty = tty_table + index;
    input_manger_subscribe(&current_tty->subscriber);  // TODO: Error check...
    set_keyboard_state(current_tty->saved_kbd_state);
    text_mode_set_active_display(current_tty->text_mode_dev);
}

static void tty_append_char(char c)
{
    bool buffer_full;
    text_mode_putc(current_tty->text_mode_dev, c);

    if (current_tty->mode & TTY_MODE_CANONICAL) {
        buffer_full = (current_tty->char_buffer_write_idx + 1) % PAGE_SIZE ==
                      current_tty->char_buffer_read_idx;
    } else {
        // In non-canonical mode we need to keep one free char in the buffer so there's space left
        // for a newline in case the of tty being switch over to canonical mode.
        buffer_full = (current_tty->char_buffer_write_idx + 2) % PAGE_SIZE ==
                      current_tty->char_buffer_read_idx;
    }

    if (buffer_full) {
        current_tty->keys_dropped = true;
        LOG("Character 'c' dropped");
        return;
    }

    // Add char to tty buffer
    current_tty->char_buffer[current_tty->char_buffer_write_idx] = c;
    current_tty->char_buffer_write_idx = (current_tty->char_buffer_write_idx + 1) % PAGE_SIZE;

    if (!(current_tty->mode & TTY_MODE_CANONICAL)) {
        // Always commit in non-canonical mode
        current_tty->char_buffer_commit_idx = (current_tty->char_buffer_commit_idx + 1) % PAGE_SIZE;
    }
}

static size_t tty_line_len(struct tty* tty)
{
    return (tty->char_buffer_write_idx - tty->char_buffer_commit_idx + PAGE_SIZE) % PAGE_SIZE;
}

static int on_events_received(input_event_t event)
{
    char     c;
    uint16_t key_code = event.key_code;
    uint16_t status   = event.status;

    if (CHECK_IF_PRESSED(status)) {
        // Handle tty switch
        if (key_code >= KEY_F1 && key_code <= KEY_F12) {
            if (CHECK_IF_CTRL(status) && CHECK_IF_ALT(status)) {
                tty_switch(key_code - KEY_F1);
            }
            return;
        }

        if (key_code == KEY_ENTER) {
            tty_append_char('\n');

            // Canonical mode specific blocking and line handling
            if ((current_tty->mode & TTY_MODE_CANONICAL) && current_tty->opened) {
                // Commit the current line
                current_tty->char_buffer_commit_idx = current_tty->char_buffer_write_idx;

                if (current_tty->waiting_proc) {
                    scheduler_unblock_task(current_tty->waiting_proc);
                    current_tty->waiting_proc = NULL;
                }
            }
            return;
        }

        if (key_code == KEY_BACKSPACE) {
            // Only do line-editing in canonical mode
            if (current_tty->mode & TTY_MODE_CANONICAL) {
                // Only delete if we have characters to commit
                if (tty_line_len(current_tty) > 0) {
                    current_tty->char_buffer_write_idx =
                        (current_tty->char_buffer_write_idx - 1) % PAGE_SIZE;
                    text_mode_del(current_tty->text_mode_dev, 1);
                }
            } else {
                tty_append_char('\b');
            }
            return;
        }

        if (KEY_ASCII_PRINTABLE(key_code)) {
            c = key_code;
            if (KEY_LETTER(key_code) && CHECK_IF_UPPER_CASE(status)) {
                c -= ('a' - 'A');
            }
            tty_append_char(c);
        }
    }

    return 0;
}

static int tty_open(struct device* dev, struct open_file* file, int oflag)
{
    struct tty* tty = GET_STRUCT(struct tty, device, dev);

    if (oflag & O_WRONLY) {
        return 0;  // Allow multiple writers
    }

    // Only allow a single consumer
    if ((oflag & O_RDONLY || oflag & O_RDONLY) && tty->opened != NULL) {
        return -EBUSY;
    }
    tty->opened = file;

    if (tty->char_buffer == NULL) {
        tty->char_buffer = vmem_request_free_page(1);
        if (tty->char_buffer == NULL) {
            return -ENOMEM;
        }
    }

    tty->char_buffer_write_idx = 0;
    tty->char_buffer_read_idx  = 0;
    return 0;
}

static int tty_close(struct device* dev, struct open_file* file)
{
    struct tty* tty = GET_STRUCT(struct tty, device, dev);
    kassert(tty->opened == file);

    if (tty->waiting_proc) {
        // Can this ever happened?
        scheduler_unblock_task(tty->waiting_proc);
    }

    // Allow device to be opened by a new process
    tty->opened = NULL;

    // Reset ring buffer to avoid data from leaking between process
    tty->char_buffer_read_idx   = 0;
    tty->char_buffer_write_idx  = 0;
    tty->char_buffer_commit_idx = 0;
    return 0;
}

static ssize_t tty_read(struct device* dev, char* buf, size_t size, off_t offset)
{
    (void)offset;  // cdev, so ignore offset
    int         read = 0;
    struct tty* tty  = GET_STRUCT(struct tty, device, dev);

    // Only block in canonical mode
    if (tty->mode & TTY_MODE_CANONICAL) {
        if (tty->char_buffer_read_idx == tty->char_buffer_commit_idx) {
            tty->waiting_proc = scheduler_get_current_task();
            scheduler_block_task(WAITING_FOR_IO);
        }
    }

    // Read until buffer full or empty
    for (size_t i = 0; i < size; i++) {
        if (tty->char_buffer_read_idx == tty->char_buffer_commit_idx) {
            break;
        }

        buf[i]                    = tty->char_buffer[tty->char_buffer_read_idx];
        tty->char_buffer_read_idx = (tty->char_buffer_read_idx + 1) % PAGE_SIZE;
        read++;
    }

    if (read > 0) {
        // Reset dropped flag since we have freed upp space within the char buffer
        tty->keys_dropped = false;
    }

    // TODO: Handle non-canonical mode vmin/timeouts...
    return read;
}

static ssize_t tty_write(struct device* dev, const char* buf, size_t size, off_t offset)
{
    (void)offset;  // cdev, so ignore offset
    char        c;
    ssize_t     ret;
    size_t      line_len, line_idx;
    struct tty* tty = GET_STRUCT(struct tty, device, dev);

    // To ensure that the currently written line always closets to the cursor, we need to re-move
    // the echoing before wring the data, and the re-add again after the writing is done.
    line_len = tty_line_len(tty);
    text_mode_del(tty->text_mode_dev, line_len);

    ret = text_mode_write(tty->text_mode_dev, buf, size);

    for (size_t i = 0; i < line_len; i++) {
        line_idx = (tty->char_buffer_commit_idx + i) % PAGE_SIZE;
        text_mode_putc(tty->text_mode_dev, tty->char_buffer[line_idx]);
    }
    return ret;
}

static int tty_init(size_t index)
{
    int         ret;
    char        str[10];
    struct tty* tty_dev = tty_table + index;

    tty_dev->text_mode_dev = text_mode_get_display(index + 1);
    if (!tty_dev->text_mode_dev) {
        return -ENODEV;
    }

    tty_dev->subscriber.on_events_received = on_events_received;
    tty_dev->saved_kbd_state               = 0;

    ret = register_device(&tty_driver, &tty_dev->device);
    if (ret < 0) {
        return ret;
    }

    ret = create_device_file(NULL, &tty_dev->device, true);
    if (ret < 0) {
        return ret;
    }

    // Reset ringbuffer
    tty_dev->char_buffer_read_idx   = 0;
    tty_dev->char_buffer_write_idx  = 0;
    tty_dev->char_buffer_commit_idx = 0;

    // Run in canonical mode by default
    tty_dev->mode = TTY_MODE_CANONICAL;
    return 0;
}

int make_tty_devs()
{
    int ret;

    // TODO: Query for available keyboard...

    num_ttys = MIN(text_get_number_of_displays(), MAX_TTYS);
    if (!num_ttys) {
        return -ENODEV;
    }

    for (size_t i = 0; i < num_ttys; i++) {
        ret = tty_init(i);
        if (ret < 0) {
            return ret;
        }
    }

    tty_switch(0);
    return 0;
}

/* When do we call register on this object, at first access? statically? on init? */
struct driver tty_driver = {
    .name         = "tty",
    .device_read  = tty_read,
    .device_open  = tty_open,
    .device_write = tty_write,
    .device_close = tty_close,
};
