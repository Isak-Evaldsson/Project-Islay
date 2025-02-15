/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
   Inspired by Sebastian Raase's "avrterm-kbd" code (no licence, used with his permission)
*/
#include <atomics.h>
#include <devices/input_manager.h>
#include <devices/keyboard/ps2_keyboard.h>
#include <ring_buffer.h>
#include <stdbool.h>
#include <stddef.h>
#include <utils.h>

#include "../internals.h"
#include "keyboard.h"

#define LOG(fmt, ...) __LOG(1, "[ps2_keyboard]", fmt, ##__VA_ARGS__)

#define KBD_CMD_BUFF_SIZE 32

/* PS2 commands */
#define PS2_CMD_SET_LEDS     0xED
#define PS2_CMD_NONE         0xEE
#define PS2_CMD_SCANCODE_SET 0xF0
#define PS2_CMD_IDENTIFY     0xF2
#define PS2_CMD_RATE         0xF3

#define PS2_RESPONSE_RESEND 0xFE
#define PS2_RESPONSE_ACK    0xFA

struct ps2_keyboard {
    struct keyboard kbd;
    char*           name;

    // Keyboard state, driving the internal state machine
    unsigned int state;

    // Managing commands to the keyboard
    keyboard_send_cmd_t send_cmd;
    ring_buff_struct(uint8_t, KBD_CMD_BUFF_SIZE) cmd_buffer;

    // TODO: Add proper locking and timeouts when sending commands...
};

/* maps set 1 scan codes to input event key codes */
static const uint8_t set1_to_keycode[] = {
    // clang-format off
    KEY_NONE,           KEY_ESCAPE,         KEY_1,              KEY_2,      
    KEY_3,              KEY_4,              KEY_5,              KEY_6,                
    KEY_7,              KEY_8,              KEY_9,              KEY_0,                  
    KEY_MINUS,          KEY_EQUAL,          KEY_BACKSPACE,      KEY_TAB,       
    KEY_Q,              KEY_W,              KEY_E,              KEY_R,
    KEY_T,              KEY_Y,              KEY_U,              KEY_I,      
    KEY_O,              KEY_P,              KEY_LBRACKET,       KEY_RBRACKET,
    KEY_ENTER,          KEY_LCTRL,          KEY_A,              KEY_S,
    KEY_D,              KEY_F,              KEY_G,              KEY_H,
    KEY_J,              KEY_K,              KEY_L,              KEY_COLON,
    KEY_APOSTROPHE,     KEY_GRAVE,          KEY_LSHIFT,         KEY_BSLASH, 
    KEY_Z,              KEY_X,              KEY_C,              KEY_V,                  
    KEY_B,              KEY_N,              KEY_M,              KEY_COMMA,  
    KEY_DOT,            KEY_FSLASH,         KEY_RSHIFT,         KEYPAD_ASTERISK,
    KEY_LALT,           KEY_SPACE,          KEY_CAPSLOCK,       KEY_F1,
    KEY_F2,             KEY_F3,             KEY_F4,             KEY_F5,    
    KEY_F6,             KEY_F7,             KEY_F8,             KEY_F9,                 
    KEY_F10,            KEY_NUMLOCK,        KEY_SCROLLOCK,      KEYPAD_7,
    KEYPAD_8,           KEYPAD_9,           KEYPAD_MINUS,       KEYPAD_4,
    KEYPAD_5,           KEYPAD_6,           KEYPAD_PLUS,        KEYPAD_1,
    KEYPAD_2,           KEYPAD_3,           KEYPAD_0,           KEYPAD_DOT,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_F11,
    KEY_F12
    //clang-format on
};

/* maps set 1 extended (E0, 0x10 + index) scan codes to input event key codes */
static const uint8_t set1_extended_to_keycode[] = {
    // clang-format off
KEY_MEDIA_PREV,     KEY_NONE,           KEY_NONE,               KEY_NONE,
KEY_NONE,           KEY_NONE,           KEY_NONE,               KEY_NONE,
KEY_NONE,           KEY_MEDIA_NEXT,     KEY_NONE,               KEY_NONE,
KEYPAD_ENTER,       KEY_RCTRL,          KEY_NONE,               KEY_NONE,
KEY_MUTE,           KEY_MEDIA_CALC,     KEY_MEDIA_PLAY,         KEY_NONE,
KEY_STOP,           KEY_NONE,           KEY_NONE,               KEY_NONE,
KEY_NONE,           KEY_NONE,           KEY_NONE,               KEY_NONE,
KEY_NONE,           KEY_NONE,           KEY_VOL_DOWN,           KEY_NONE,
KEY_VOL_UP,         KEY_NONE,           KEY_HOME,               KEY_NONE,
KEY_NONE,           KEYPAD_FSLASH,      KEY_NONE,               KEY_NONE,
KEY_RALT,           KEY_NONE,           KEY_NONE,               KEY_NONE,
KEY_NONE,           KEY_NONE,           KEY_NONE,               KEY_NONE,
KEY_NONE,           KEY_NONE,           KEY_NONE,               KEY_NONE,
KEY_NONE,           KEY_NONE,           KEY_NONE,               KEY_HOME,
KEY_UP,             KEY_PAGEUP,         KEY_NONE,               KEY_LEFT,
KEY_NONE,           KEY_RIGHT,          KEY_NONE,               KEY_END,
KEY_DOWN,           KEY_PAGEDOWN,       KEY_INSERT,             KEY_DELETE,
KEY_NONE,           KEY_NONE,           KEY_NONE,               KEY_NONE,
KEY_NONE,           KEY_NONE,           KEY_NONE,               KEY_LSUPER,
KEY_RSUPER,         KEY_APP,            KEY_POWER,              KEY_SLEEP,
KEY_NONE,           KEY_NONE,           KEY_NONE,               KEY_WAKE,
KEY_NONE,           KEY_MEDIA_SEARCH,   KEY_MEDIA_FAVORITES,    KEY_MEDIA_REFRESH,
KEY_MEDIA_STOP,     KEY_MEDIA_FORWARD,  KEY_MEDIA_BACK,         KEY_MEDIA_COMPUTER,
KEY_MEDIA_EMAIL,    KEY_SELECT,
    // clang-format on
};

/* TODO: Allow multiple keyboard support */
static struct ps2_keyboard kbd;

void ps2_send_command(unsigned char data)
{
    if (ring_buff_full(kbd.cmd_buffer)) {
        LOG("command buffer full, ignoring sent command (cmd: %u)", data);
        return;
    }

    // Store data in case we need to re-send
    ring_buffer_push(kbd.cmd_buffer, data);

    // If there already was data in the queue there's no need to starting sending,
    // the data will be processed on the next ACK
    if (ring_buff_size(kbd.cmd_buffer) == 1) {
        kbd.send_cmd(data);
    }
}

static void ps2_keyboard_set_leds(unsigned char leds)
{
    unsigned char led_bits = 0;

    if (leds & (1 << LED_CAPS_LOCK)) {
        SET_BIT(led_bits, 2);
    }

    if (leds & (1 << LED_NUM_LOCK)) {
        SET_BIT(led_bits, 1);
    }

    if (leds & (1 << LED_SCROLL_LOCK)) {
        SET_BIT(led_bits, 0);
    }

    ps2_send_command(PS2_CMD_SET_LEDS);
    ps2_send_command(led_bits);
}

void ps2_keyboard_register(char* device_name, keyboard_send_cmd_t fn)
{
    kassert(fn != NULL);

    // Check if trying to register multiple keyboards
    if (kbd.send_cmd != NULL) {
        kpanic(
            "PS2 driver currently not supporting multiple ps2 keyboard devices\nRegistering "
            "%s, "
            "but %s previously registered",
            device_name, kbd.name);
    }

    ring_buff_init(kbd.cmd_buffer);

    kbd.send_cmd     = fn;
    kbd.kbd.set_leds = ps2_keyboard_set_leds;
    kassert(keyboard_init(&kbd.kbd) == 0);

    kprintf("PS/2 keyboard driver: successfully registered %s\n", device_name);
}

void ps2_keyboard_send(unsigned char scancode)
{
    uint8_t cmd;

    // Keyboard state machine
    // clang-format off
    switch (kbd.state) {
        case 0:  // Initial state, regular keys
            switch (scancode) {
            case 0xE0: kbd.state = 1; break; // Extended keys and print screen
            case 0xE1: kbd.state = 4; break; // Pause
			case PS2_RESPONSE_ACK:
                ring_buffer_pop(kbd.cmd_buffer, cmd);

                // Send next command if available
                if(!ring_buff_empty(kbd.cmd_buffer)) {
                    cmd = ring_buff_first(kbd.cmd_buffer);
                    kbd.send_cmd(cmd);
                }
            break;
			case PS2_RESPONSE_RESEND: 
                if (!ring_buff_empty(kbd.cmd_buffer)) {
                    cmd = ring_buff_first(kbd.cmd_buffer);
                    kbd.send_cmd(cmd);
                }
            break;
            default: // Regular key
                if (scancode <= 0x58) {
                    keyboard_process_key(&kbd.kbd, set1_to_keycode[scancode], false);
                } else if (scancode >= 0x80 && scancode <= 0xD7) {
                    keyboard_process_key(&kbd.kbd, set1_to_keycode[scancode - 0x80], true);
                }
                kbd.state = 0; break;
            }
            break;
        case 1: // Extended keys, media, apci, etc.
            switch (scancode) {
            case 0x2A: kbd.state = 2; break; // Print screen pressed: 0xE0, 0x2A, 0xE0, 0x37
            case 0xB7: kbd.state = 2; break; // Print screen released: 0xE0, 0xB7, 0xE0, 0xAA
            default: 
                if (scancode >= 0x10 && scancode <= 0x6D) {
                    keyboard_process_key(&kbd.kbd, set1_extended_to_keycode[scancode - 0x10], false);
                } else if (scancode >= 0x90 && scancode <= 0xED) {
                    keyboard_process_key(&kbd.kbd, set1_extended_to_keycode[scancode - 0x90], true);
                }
                kbd.state = 0; break; 
            }
            break;
        case 2: kbd.state = (scancode == 0xE0 ? 3 : 0); break;
        case 3:
            if (scancode == 0x37 || scancode == 0xAA) {
                keyboard_process_key(&kbd.kbd, KEY_PRTSC, scancode == 0xAA);
            }
            kbd.state = 0; break; 
        // Pause/Break pressed (has no pressed): 0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xC5 
        case 4: kbd.state = (scancode == 0x1D ? 5 : 0); break;
        case 5: kbd.state = (scancode == 0x45 ? 6 : 0); break;
        case 6: kbd.state = (scancode == 0xE1 ? 7 : 0); break;
        case 7: kbd.state = (scancode == 0x9D ? 8 : 0); break;
        case 9:
            if (scancode == 0xC5) {
                keyboard_process_key(&kbd.kbd, KEY_PAUSE, true);
            }
            kbd.state = 0; break;
        default:
            kpanic("PS2 Driver reaching undefined state %u\n", kbd.state);
    }
    // clang-format on
}
