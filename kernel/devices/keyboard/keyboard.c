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

#define LOG(fmt, ...) __LOG(1, "[kbd]", fmt, ##__VA_ARGS__)

static unsigned char global_keylock_state = 0;

static void set_leds()
{
    struct keyboard   *kbd;
    struct list_entry *entry;

    LIST_ITER(&keyboard_driver.devices, entry)
    {
        kbd = GET_STRUCT(struct keyboard, dev, LIST_ENTRY_TO_DEV(entry));
        kbd->set_leds(global_keylock_state);
    }
}

static void set_modifier_state(struct keyboard *kbd, uint8_t modifier, bool released)
{
    if (released) {
        CLR_BIT(kbd->modifier_state, modifier);
    } else {
        SET_BIT(kbd->modifier_state, modifier);
    }
}

void keyboard_process_key(struct keyboard *kbd, uint8_t keycode, bool released)
{
    bool    keylock_state_update = false;
    uint8_t status               = (released) ? (1 << STATUS_RELEASED) : 0;

    // Drop invalid or erroneous keys codes
    if (keycode >= KEY_CODE_MAX || keycode <= ERR_UNDEF) {
        LOG("Invalid keycode received: %u", keycode);
        return;
    }

    // Adjust local keyboard state
    switch (keycode) {
        case KEY_LCTRL:
            set_modifier_state(kbd, KBD_LCTRL, released);
            break;
        case KEY_RCTRL:
            set_modifier_state(kbd, KBD_RCTRL, released);
            break;
        case KEY_LALT:
            set_modifier_state(kbd, KBD_LALT, released);
            break;
        case KEY_RALT:
            set_modifier_state(kbd, KBD_RALT, released);
            break;
        case KEY_LSHIFT:
            set_modifier_state(kbd, KBD_LSHIFT, released);
            break;
        case KEY_RSHIFT:
            set_modifier_state(kbd, KBD_RSHIFT, released);
            break;
        case KEY_LSUPER:
            set_modifier_state(kbd, KBD_LSUPER, released);
            break;
        case KEY_RSUPER:
            set_modifier_state(kbd, KBD_RSUPER, released);
            break;
    };

    // Adjust global keyboard state
    if (!released) {
        switch (keycode) {
            case KEY_CAPSLOCK:
                INV_BIT(global_keylock_state, LED_CAPS_LOCK);
                keylock_state_update = true;
                break;
            case KEY_NUMLOCK:
                INV_BIT(global_keylock_state, LED_NUM_LOCK);
                keylock_state_update = true;
                break;
            case KEY_SCROLLOCK:
                INV_BIT(global_keylock_state, LED_SCROLL_LOCK);
                keylock_state_update = true;
                break;
        }
    };

    if (keylock_state_update) {
        set_leds();
    }

    // Set status based on global and per-keyboard state
    if ((kbd->modifier_state & (1 << KBD_LSHIFT)) || (kbd->modifier_state & (1 << KBD_RSHIFT))) {
        SET_BIT(status, STATUS_MOD_SHIFT);
    }

    if ((kbd->modifier_state & (1 << KBD_LCTRL)) || (kbd->modifier_state & (1 << KBD_RCTRL))) {
        SET_BIT(status, STATUS_MOD_CTRL);
    }

    if ((kbd->modifier_state & (1 << KBD_LALT)) || (kbd->modifier_state & (1 << KBD_RALT))) {
        SET_BIT(status, STATUS_MOD_ALT);
    }

    if ((kbd->modifier_state & (1 << KBD_LSUPER)) || (kbd->modifier_state & (1 << KBD_RSUPER))) {
        SET_BIT(status, STATUS_MOD_SUPER);
    }

    if (global_keylock_state & (1 << LED_CAPS_LOCK)) {
        SET_BIT(status, STATUS_CAPSLOCK);
    }

    if (global_keylock_state & (1 << LED_NUM_LOCK)) {
        SET_BIT(status, STATUS_NUMLOCK);
    }

    if (global_keylock_state & (1 << LED_SCROLL_LOCK)) {
        SET_BIT(status, STATUS_SCROLLOCK);
    }

    // TODO: Translate from keymap to UTF8...
    input_manager_send_event((input_event_t){.key_code = ((status << 8) | keycode), .status = 0});
}

unsigned char get_keyboard_state()
{
    return global_keylock_state;
}

void set_keyboard_state(unsigned char state)
{
    struct keyboard   *kbd;
    struct list_entry *entry;

    global_keylock_state = state;
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
