/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/

#include <arch/paging.h>
#include <devices/display/text_mode_display.h>
#include <devices/input_manager.h>
#include <devices/tty.h>
#include <memory/vmem_manager.h>
#include <ring_buffer.h>
#include <tasks/scheduler.h>

#include "internals.h"
#include "keyboard/keyboard.h"
#include "keyboard/keymaps.h"

#define TTY_MODE_CANONICAL 0x01

#define LOG(fmt, ...) __LOG(1, "[TTY]", fmt, ##__VA_ARGS__)

/*
    TTY Re-design:

    Have two input queues (of uc2-events);
    - raw queue - inserted to on input events in interrupt context, wakeup all procss waiting for
      the queue to be empty
    - canonical queue - when reading, move chars from raw queue to canonical queue (and line edit on
      they way if needed), then copy as many you need to the reader. How do we deal with waits?
      Two scenarios:
        kthread - just do a wait-loop
        userspace syscall bottom half handler - but proc to sleep, then

    - if overflow, simply flush the queues (canonical as well?, unix v6 does)

    Have an output queue:
    - When echoing char, add it to queue (and start time event if it isn't started).
    - When writing to tty, add it to queue and but process to sleep - What if the there's not
      enough space in the queue? for kthreads a simple wait loop is fine, but what if it's a syscall
      running in bottom half isr? Iterate over all wait process, re-submit chars, wait again

    - How to handle overflows?
    - Who to to actually output? Start a timed event (if there's not one already running), the
      callback does the following:
        - Dumps a fixed number of chars (or until the queue is empty)
        - If empty, wakeup processes waiting for the queue to be empty, stop the timed events
        - else continue the timed events

    How to do escape code handling?

    Except for the tty itself, what other infrastructure is needed?
    - A mechansm for a process to wait on a particular address, and a wakeup


    Buffer wait API:

    buffer_wait(const char* wait_buff, char* dest_buff, size_t n):
        1. Block running task
        2. Note buffers in process record
        3. Add task to wait queue

    buffer_wakeup_waiters(const char* buff, size_t n):
        1. Find all waiting tasks (either a global io wait queue or one per buffer)
        2. For each waiting task:
            1. Copy n bytes (or as many dest_buff holds)
            2. Unblock task...
            3. Regarding unblocking, I guess we could have two modes:
                - Require buffer to be filled
                - Always unblock
            4. Do we need a distinction between kernel and userspace threads?

    How would that API be used in the tty?
    Well, in tty_input, when an enter is received, call buffer_wakeup_waiter(raw_buff, )

    On tty_read - if the current raw buffer is empty, call buffer_wait()
*/

#define TTY_BUF_SIZE  PAGE_SIZE
#define TTY_BUF_ELEMS (TTY_BUF_SIZE / sizeof(ucs2_t))

ring_buff_type(tty_buff, ucs2_t, /* DYNAMICLY ALLOCATED */);

#define TTY_BUFF_INIT(buff, size) ring_buff_init_with_cap(buff, size - sizeof(struct tty_buff))

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
    size_t       char_buffer_read_idx;
    size_t       char_buffer_write_idx;
    size_t       char_buffer_commit_idx;
    char*        char_buffer;

<<<<<<< Updated upstream
    bool         keys_dropped;     // Has the keys been dropped due to the char buffer being full
    unsigned int saved_kbd_state;  // Keyboard state for that tty
=======
    bool keys_dropped;  // Has the keys been dropped due to the char buffer being full

    uint8_t kbd_modifier_state;
    uint8_t kbd_led_state;

    // TODO: Dynamically allocate...
    struct tty_buff* raw_input_buff;
    struct tty_buff* canonical_input_buff;
    struct tty_buff* output_buff;
>>>>>>> Stashed changes
};

#define MAX_TTYS 8
static struct tty  tty_table[MAX_TTYS];
static size_t      num_ttys;
static struct tty* current_tty;

static void tty_switch(size_t index);
static void tty_send_char(ucs2_t c);

static int tty_on_input(input_event_t event)
{
    ucs2_t         c;
    const uint16_t keycode = event.keycode;
    const uint8_t  key     = KEYCODE_GET_KEY(keycode);

    switch (KEYCODE_GET_TYPE(keycode)) {
        case KEYCODE_TYPE_LOCK:
            if (!KEYCODE_CHECK_RELEASED(keycode)) {
                INV_BIT(current_tty->kbd_led_state, KEYCODE_GET_MODIFIER(keycode));
                set_keyboard_leds(current_tty->kbd_led_state);
            }
            break;

        case KEYCODE_TYPE_MOD:
            if (KEYCODE_CHECK_RELEASED(keycode)) {
                CLR_BIT(current_tty->kbd_modifier_state, KEYCODE_GET_MODIFIER(keycode));
            } else {
                SET_BIT(current_tty->kbd_modifier_state, KEYCODE_GET_MODIFIER(keycode));
            }
            break;

        case KEYCODE_TYPE_REG:
            if (!KEYCODE_CHECK_RELEASED(keycode)) {
                // Handle tty switch
                if (key >= KEY_F1 && key <= KEY_F12) {
                    if ((current_tty->kbd_modifier_state &
                         ((1 << KEYCODE_MOD_LCTRL) | (1 << KEYCODE_MOD_RCTRL))) &&
                        (current_tty->kbd_modifier_state &
                         ((1 << KEYCODE_MOD_LALT) | (1 << KEYCODE_MOD_RALT)))) {
                        tty_switch(key - KEY_F1);
                    }
                    return 0;
                }

                if (key == KEY_ENTER) {
                    // TODO: Handle wakeup reader magic...
                }

                c = keymap_get_key(keycode, current_tty->kbd_modifier_state,
                                   current_tty->kbd_led_state);
                if (c != UCS2_NOCHAR) {
                    // TODO: Handle echoing...

                    if (!ring_buff_full(*current_tty->raw_input_buff)) {
                        ring_buffer_push(*current_tty->raw_input_buff, c);
                    } else {
                        // TODO: Dropped keys flags + flushing?
                    }
                }
            }
    }
    return 0;
}

static void tty_canonicalize_input()
{
    ucs2_t c;

    while (!ring_buff_empty(*current_tty->raw_input_buff)) {
        c = ring_buffer_pop(*current_tty->raw_input_buff);

        // TODO: Should we assume the canonical buffer to always have space?,
        // Then we need to flush both on overflow in tty_on_input

        if (/* Line editing... */ 1) {
            if (c == 0x007F /*TODO DELETE MACRO*/) {
                if (!ring_buff_empty(*current_tty->canonical_input_buff)) {
                    ring_buffer_pop(*current_tty->canonical_input_buff);
                }
                continue;
            }
        }

        ring_buffer_push(*current_tty->canonical_input_buff, c);
    }
}

static void tty_send_char(ucs2_t c)
{
    // TODO: How to handle the overflow case?
    // We have two scenarios:
    // 1. on write - just put the writer to sleep
    // 2. on echo - unclear?
    ring_buffer_push(*current_tty->output_buff, c);
    // TODO: Start the dump timer if not already created...
}

#define DUMP_LIMIT (100)

static void dump_char_timed_event()
{
    ucs2_t c;

    if (ring_buff_empty(*current_tty->output_buff)) {
        return;  // Nothing to do
    }

    for (size_t i = 0; i < DUMP_LIMIT; i++) {
        c = ring_buffer_pop(*current_tty->output_buff);
        // TODO: Write to vga display

        if (ring_buff_empty(*current_tty->output_buff)) {
            break;
        }
    }

    // TODO: Wakeup waiters...
    if (!ring_buff_empty(*current_tty->output_buff)) {
        // TODO: Register new timed event...
    }
}

static void tty_switch(size_t index)
{
    struct tty* old = current_tty;
    if (index >= num_ttys) {
        return;
    }

    LOG("Switch from %u to %u", old ? old : 0, index);
    if (old) {
        input_manger_unsubscribe(&old->subscriber);
    }

    current_tty = tty_table + index;
    input_manger_subscribe(&current_tty->subscriber);  // TODO: Error check...
    set_keyboard_leds(current_tty->kbd_led_state);
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
    uint16_t keycode = event.keycode;
    uint8_t  key     = KEYCODE_GET_KEY(keycode);

    switch (KEYCODE_GET_TYPE(keycode)) {
        case KEYCODE_TYPE_LOCK:
            if (!KEYCODE_CHECK_RELEASED(keycode)) {
                INV_BIT(current_tty->kbd_led_state, KEYCODE_GET_MODIFIER(keycode));
                set_keyboard_leds(current_tty->kbd_led_state);
            }
            return 0;

        case KEYCODE_TYPE_MOD:
            if (KEYCODE_CHECK_RELEASED(keycode)) {
                CLR_BIT(current_tty->kbd_modifier_state, KEYCODE_GET_MODIFIER(keycode));
            } else {
                SET_BIT(current_tty->kbd_modifier_state, KEYCODE_GET_MODIFIER(keycode));
            }
            return 0;

        case KEYCODE_TYPE_REG:
            if (!KEYCODE_CHECK_RELEASED(keycode)) {
                // Handle tty switch
                if (key >= KEY_F1 && key <= KEY_F12) {
                    if ((current_tty->kbd_modifier_state &
                         ((1 << KEYCODE_MOD_LCTRL) | (1 << KEYCODE_MOD_RCTRL))) &&
                        (current_tty->kbd_modifier_state &
                         ((1 << KEYCODE_MOD_LALT) | (1 << KEYCODE_MOD_RALT)))) {
                        tty_switch(key - KEY_F1);
                    }
                    return 0;
                }

                if (key == KEY_ENTER) {
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
                    return 0;
                }

                if (key == KEY_BACKSPACE) {
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
                    return 0;
                }

                ucs2_t ucs2_char = keymap_get_key(keycode, current_tty->kbd_modifier_state,
                                                  current_tty->kbd_led_state);
                if (ucs2_char != UCS2_NOCHAR) {
                    // Temporary UC2->ASCII until the tty re-written to properly handle ucs2
                    char c = 0x1A;  // use ascii sub as default char
                    if (!(ucs2_char >> 7)) {
                        c = ucs2_char & 0x7f;
                    }
                    tty_append_char(c);
                }
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

    ret = register_device(&tty_driver, &tty_dev->device);
    if (ret < 0) {
        return ret;
    }

    ret = create_device_file(NULL, &tty_dev->device, true);
    if (ret < 0) {
        return ret;
    }

    // Allocate buffers... (Can we move them to open?),
    tty_dev->raw_input_buff = vmem_request_free_page(1);
    if (tty_dev->raw_input_buff == NULL) {
        return -ENOMEM;
    }

    tty_dev->canonical_input_buff = vmem_request_free_page(1);
    if (tty_dev->canonical_input_buff == NULL) {
        return -ENOMEM;
    }

    tty_dev->output_buff = vmem_request_free_page(1);
    if (tty_dev->output_buff == NULL) {
        return -ENOMEM;
    }

    TTY_BUFF_INIT(*tty_dev->raw_input_buff, PAGE_SIZE);
    TTY_BUFF_INIT(*tty_dev->canonical_input_buff, PAGE_SIZE);
    TTY_BUFF_INIT(*tty_dev->output_buff, PAGE_SIZE);

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
