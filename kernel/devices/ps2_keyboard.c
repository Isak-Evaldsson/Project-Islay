/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <devices/input_manager.h>
#include <devices/ps2_keyboard.h>
#include <stdbool.h>
#include <stddef.h>
#include <utils.h>

#define KBD_CMD_BUFF_SIZE 16

#define BREAK_CODE 0x80

/* TODO: Allow multiple keyboard support */
static struct {
    char*                   name;
    keyboard_receive_data_t callback;
} device;

/* Keyboard state, driving the internal state machine */
static enum kbd_state {
    KBD_INITIAL_STATE,
    KBD_KEY_PRESSED_STATE,
    KBD_EXTENDED_KEY_STATE,
} state = KBD_INITIAL_STATE;

/* Checks if CAPS lock is pressed */
static bool caps_pressed = false;

/* maps set 1 scan codes to input event key codes */
static uint16_t set1_to_keycode[] = {
    // TODO: Add all keys
    INVALID_KEY,   KEY_ESCAPE, KEY_1,     KEY_2,      KEY_3,        KEY_4,          KEY_5,
    KEY_6,         KEY_7,      KEY_8,     KEY_9,      KEY_0,        KEY_MINUS,      KEY_EQUAL,
    KEY_BACKSPACE, KEY_TAB,    KEY_Q,     KEY_W,      KEY_E,        KEY_R,          KEY_T,
    KEY_Y,         KEY_U,      KEY_I,     KEY_O,      KEY_P,        KEY_LBRACKET,   KEY_RBRACKET,
    KEY_ENTER,     KEY_CTRL,   KEY_A,     KEY_S,      KEY_D,        KEY_F,          KEY_G,
    KEY_H,         KEY_J,      KEY_K,     KEY_L,      KEY_SEMI,     KEY_APPOSTRPHE, KEY_BACKTICKS,
    KEY_LSHIFT,    KEY_LSLASH, KEY_Z,     KEY_X,      KEY_C,        KEY_V,          KEY_B,
    KEY_N,         KEY_M,      KEY_COMMA, KEY_DOT,    KEY_RSLASH,   KEY_RSHIFT,     KEY_PRTSC,
    KEY_ALT,       KEY_SPACE,  KEY_CAPS,  KEY_F1,     KEY_F2,       KEY_F3,         KEY_F4,
    KEY_F5,        KEY_F6,     KEY_F7,    KEY_F8,     KEY_F9,       KEY_F10,        KEY_NUMLOCK,
    KEY_SCROLLOCK, KEY_HOME,   KEY_UP,    KEY_PAGEUP, KEY_MINUS,    KEY_LEFT,       KEY_CENTER,
    KEY_RIGHT,     KEY_PLUS,   KEY_END,   KEY_DOWN,   KEY_PAGEDOWN, KEY_INSERT,     KEY_DELETE,
};

void ps2_keyboard_register(char* device_name, keyboard_receive_data_t fn)
{
    kassert(fn != NULL);

    // Check if trying to register multiple keyboards
    if (device.callback != NULL) {
        kpanic(
            "PS2 driver currently not supporting multiple ps2 keyboard devices\nRegistering "
            "%s, "
            "but %s previously registered",
            device_name, device.name);
    }

    // Registering our device
    device.name     = device_name;
    device.callback = fn;

    input_manager_init();
    kprintf("PS/2 keyboard driver: successfully registered %s\n", device_name);
}

/* Helper function, that sends key event to the input manger based on the supplied keycode, handles
   scancode independet internal state such as caps_lock etc. */
void send_event(uint16_t keycode, bool released)
{
    unsigned char status = (released) ? INPUT_RELEASE : INPUT_PRESSED;

    // switch caps mode when key is released
    if (keycode == KEY_CAPS && released) {
        // TODO: Send back light code to keyboard
        caps_pressed = !caps_pressed;
    }

    if (caps_pressed) {
        status |= UPPER_CASE;
    }

    input_manager_send_event(keycode, status);
}

void ps2_keyboard_send(unsigned char scancode)
{
    // kprintf("State: %u, Scancode: %u\n", state, scancode);

    // Keyboard mealy state machine
    switch (state) {
        case KBD_INITIAL_STATE:
            if (scancode != 0xE0) {
                state = KBD_KEY_PRESSED_STATE;
                send_event(set1_to_keycode[scancode], false);
            } else {
                state = KBD_EXTENDED_KEY_STATE;
                kprintf("PS2 Driver currently ignores extended keys \n");
            }
            break;
        case KBD_KEY_PRESSED_STATE:
            if (scancode >= 0x80) {
                state = KBD_INITIAL_STATE;
                send_event(set1_to_keycode[scancode - 0x80], true);
            } else {
                send_event(set1_to_keycode[scancode], false);
            }
            break;

        case KBD_EXTENDED_KEY_STATE:
            state = KBD_INITIAL_STATE;
            break;

        default:
            kpanic("PS2 Driver reaching undefined state %u\n", state);
    }
}
