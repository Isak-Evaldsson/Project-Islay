/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
   Inspired by Sebastian Raase's "avrterm-kbd" code (no licence, used with his permission)
*/
#include <devices/input_manager.h>
#include <stdint.h>
#include <uapi/errno.h>
#include <utils.h>

#include "../internals.h"
#include "keyboard.h"

static unsigned char keyboard_state = 0;

static void set_leds()
{
    struct keyboard   *kbd;
    struct list_entry *entry;

    LIST_ITER(&keyboard_driver.devices, entry)
    {
        kbd = GET_STRUCT(struct keyboard, dev, LIST_ENTRY_TO_DEV(entry));
        kbd->set_leds(keyboard_state);
    }
}

static void keyboard_update_state(unsigned char led)
{
    if (keyboard_state & (1 << led)) {
        CLR_BIT(keyboard_state, led);
    } else {
        SET_BIT(keyboard_state, led);
    }
    set_leds();
}

// Do we want to have separate functions for press and release?
void keyboard_process_event(uint16_t key_code, unsigned char status)
{
    if (KEY_LETTER(key_code)) {
        // TODO: Keymap translation of letter...

        if (status & (1 << MOD_SHIFT)) {
            SET_BIT(status, UPPER_CASE);
        };

        if (keyboard_state & (1 << LED_CAPS_LOCK)) {
            INV_BIT(status, UPPER_CASE);
        }
    }

    switch (key_code) {
        case KEY_CAPS:
            if (status & (1 << INPUT_RELEASED)) {
                keyboard_update_state(LED_CAPS_LOCK);
            }
            break;

        case KEY_NUMLOCK:
            if (status & (1 << INPUT_RELEASED)) {
                keyboard_update_state(LED_NUM_LOCK);
            }
            break;

        case KEY_SCROLLOCK:
            if (status & (1 << INPUT_RELEASED)) {
                keyboard_update_state(LED_CAPS_LOCK);
            }
            break;

        default:
            // TODO: Keymap translation of non-letter...
    }

    // TODO: Implement separate keycode for keypad and regular keys in order to get numlock
    // working...
    input_manager_send_event(key_code, status);
}

unsigned char get_keyboard_state()
{
    return keyboard_state;
}

void set_keyboard_state(unsigned char state)
{
    struct keyboard   *kbd;
    struct list_entry *entry;

    keyboard_state = state;
    set_leds();
}

int keyboard_init(struct keyboard *kbd)
{
    int ret;

    if (kbd->set_leds == NULL) {
        return -EINVAL;
    }

    ret = register_device(&keyboard_driver, &kbd->dev);
    if (ret < 0) {
        return ret;
    }

    // TODO: Assign keymap once implemented...
    input_manager_init();
    return 0;
}

struct driver keyboard_driver = {
    .name = "keyboard",
};
