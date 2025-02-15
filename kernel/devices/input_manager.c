/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/interrupts.h>
#include <devices/input_manager.h>
#include <ring_buffer.h>
#include <uapi/errno.h>
#include <utils.h>

#include "internals.h"

#define LOG(fmt, ...) __LOG(1, "[INPUT_MANAGER]", fmt, ##__VA_ARGS__)

// Stores all currently registered input event subscribers
static struct list subscriber_queue = LIST_INIT();

void input_manager_init()
{
    static bool initiated = false;

    if (!initiated) {
        initiated = true;
        // TODO: Create input related dev/kinfo files...
    }
}

/* Allows the input driver to send an input event */
void input_manager_send_event(input_event_t event)
{
    struct list_entry* entry;

    if ((event.key_code & 0xff) >= KEY_CODE_MAX) {
        LOG("Input manager warning: received invalid keycode: %u\n", event.key_code);
        return;
    }

    // TODO: fix clang format, curly bracket should be on the same line
    LIST_ITER(&subscriber_queue, entry)
    {
        struct input_subscriber* subscriber = GET_STRUCT(struct input_subscriber, list, entry);

        // How to handle errors?
        subscriber->on_events_received(event);
    }
}

int input_manger_subscribe(struct input_subscriber* subscriber)
{
    if (!subscriber->on_events_received) {
        return -EINVAL;
    }

    list_add(&subscriber_queue, &subscriber->list);
}

void input_manger_unsubscribe(struct input_subscriber* subscriber)
{
    list_remove(&subscriber_queue, &subscriber->list);
}

// Temporary solution until we have proper keymaps
char keycode_to_ascii(uint16_t keycode)
{
    char    c      = '\0';
    uint8_t status = keycode >> 8;
    uint8_t key    = keycode & 0xff;

    if (key >= KEY_A && key <= KEY_Z) {
        bool upper_case = status & (1 << STATUS_CAPSLOCK);
        if (CHECK_IF_SHIFT(status)) {
            upper_case = !upper_case;
        }
        c = ((upper_case) ? 'A' : 'a') + (key - KEY_A);
    } else if (key >= KEY_1 && key <= KEY_9) {
        c = '1' + (key - KEY_1);
    } else {
        switch (key) {
            case KEY_0:
                c = '0';
                break;
            case KEY_SPACE:
                c = ' ';
                break;
            case KEY_ENTER:
                c = '\n';
                break;
            default:
                break;
        }
    }
    return c;
}

/*
    keymaps are implemented as arrays of size 4 * KEY_CODE_SIZE mapping


                NONE, SHIFT, CTRL,
    KEY_CODE:
*/

/* Translate input event to ASCII */

/*
    How do we define keymaps?

    1. Big static table

    HID_KEYCODE, REG, SHIFT, ALT,
    KEY_A,       'a',  'A',  NONE
    KEY_1,        1,   '!',  NONE
    KEY_2         2,  '"',   '@'

    Since we have 231 USB HID codes, 3 modes and 4 bytes since UTF8 we get ~2800 bytes which are
    mostly empty space, We could do just the first 67 to save some space.

    2. Dynamic keymaps:

    Files with syntax:
    KEY_CODE: MODNUM_<UTF8-char> ...

    WHICH GETS TRANSLATED TO:
    struct row {
        uint16_t    keycode;
        uint8_t     n;
        struct col {
            uint8_t mod,
            uint8_t utf8_char[4],
        } cols[];
    }

    In kernel data structure:
    hashtable mapping keycode (could be an array since only integer) ->

    How do we load that structure statically?
*/
