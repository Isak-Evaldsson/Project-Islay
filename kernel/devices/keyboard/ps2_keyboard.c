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
static const uint16_t set1_to_keycode[] = {
    // clang-format off
    KEYCODE_RKEY(KEY_NONE),         KEYCODE_RKEY(KEY_ESCAPE),           KEYCODE_RKEY(KEY_1),                KEYCODE_RKEY(KEY_2),      
    KEYCODE_RKEY(KEY_3),            KEYCODE_RKEY(KEY_4),                KEYCODE_RKEY(KEY_5),                KEYCODE_RKEY(KEY_6),                
    KEYCODE_RKEY(KEY_7),            KEYCODE_RKEY(KEY_8),                KEYCODE_RKEY(KEY_9),                KEYCODE_RKEY(KEY_0),                  
    KEYCODE_RKEY(KEY_MINUS),        KEYCODE_RKEY(KEY_EQUAL),            KEYCODE_RKEY(KEY_BACKSPACE),        KEYCODE_RKEY(KEY_TAB),       
    KEYCODE_RKEY(KEY_Q),            KEYCODE_RKEY(KEY_W),                KEYCODE_RKEY(KEY_E),                KEYCODE_RKEY(KEY_R),
    KEYCODE_RKEY(KEY_T),            KEYCODE_RKEY(KEY_Y),                KEYCODE_RKEY(KEY_U),                KEYCODE_RKEY(KEY_I),      
    KEYCODE_RKEY(KEY_O),            KEYCODE_RKEY(KEY_P),                KEYCODE_RKEY(KEY_LBRACKET),         KEYCODE_RKEY(KEY_RBRACKET),
    KEYCODE_RKEY(KEY_ENTER),        KEYCODE_MKEY(KEYCODE_MOD_LCTRL),    KEYCODE_RKEY(KEY_A),                KEYCODE_RKEY(KEY_S),
    KEYCODE_RKEY(KEY_D),            KEYCODE_RKEY(KEY_F),                KEYCODE_RKEY(KEY_G),                KEYCODE_RKEY(KEY_H),
    KEYCODE_RKEY(KEY_J),            KEYCODE_RKEY(KEY_K),                KEYCODE_RKEY(KEY_L),                KEYCODE_RKEY(KEY_COLON),
    KEYCODE_RKEY(KEY_APOSTROPHE),   KEYCODE_RKEY(KEY_GRAVE),            KEYCODE_MKEY(KEYCODE_MOD_LSHIFT),   KEYCODE_RKEY(KEY_BSLASH), 
    KEYCODE_RKEY(KEY_Z),            KEYCODE_RKEY(KEY_X),                KEYCODE_RKEY(KEY_C),                KEYCODE_RKEY(KEY_V),                  
    KEYCODE_RKEY(KEY_B),            KEYCODE_RKEY(KEY_N),                KEYCODE_RKEY(KEY_M),                KEYCODE_RKEY(KEY_COMMA),  
    KEYCODE_RKEY(KEY_DOT),          KEYCODE_RKEY(KEY_FSLASH),           KEYCODE_MKEY(KEYCODE_MOD_RSHIFT),   KEYCODE_RKEY(KEYPAD_ASTERISK),
    KEYCODE_MKEY(KEYCODE_MOD_LALT), KEYCODE_RKEY(KEY_SPACE),            KEYCODE_LKEY(KEYCODE_CAPS_LOCK),    KEYCODE_RKEY(KEY_F1),
    KEYCODE_RKEY(KEY_F2),           KEYCODE_RKEY(KEY_F3),               KEYCODE_RKEY(KEY_F4),               KEYCODE_RKEY(KEY_F5),    
    KEYCODE_RKEY(KEY_F6),           KEYCODE_RKEY(KEY_F7),               KEYCODE_RKEY(KEY_F8),               KEYCODE_RKEY(KEY_F9),                 
    KEYCODE_RKEY(KEY_F10),          KEYCODE_LKEY(KEYCODE_NUM_LOCK),     KEYCODE_LKEY(KEYCODE_SCROLL_LOCK),  KEYCODE_RKEY(KEYPAD_7),
    KEYCODE_RKEY(KEYPAD_8),         KEYCODE_RKEY(KEYPAD_9),             KEYCODE_RKEY(KEYPAD_MINUS),         KEYCODE_RKEY(KEYPAD_4),
    KEYCODE_RKEY(KEYPAD_5),         KEYCODE_RKEY(KEYPAD_6),             KEYCODE_RKEY(KEYPAD_PLUS),          KEYCODE_RKEY(KEYPAD_1),
    KEYCODE_RKEY(KEYPAD_2),         KEYCODE_RKEY(KEYPAD_3),             KEYCODE_RKEY(KEYPAD_0),             KEYCODE_RKEY(KEYPAD_DOT),
    KEYCODE_RKEY(KEY_NONE),         KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_INT1),             KEYCODE_RKEY(KEY_F11),
    KEYCODE_RKEY(KEY_F12)
    //clang-format on
};

/* maps set 1 extended (E0, 0x10 + index) scan codes to input event key codes */
static const uint16_t set1_extended_to_keycode[] = {
    // clang-format off
    KEYCODE_RKEY(KEY_MEDIA_PREV),       KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),
    KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),
    KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_MEDIA_NEXT),       KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),
    KEYCODE_RKEY(KEYPAD_ENTER),         KEYCODE_MKEY(KEYCODE_MOD_RCTRL),    KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),
    KEYCODE_RKEY(KEY_MUTE),             KEYCODE_RKEY(KEY_MEDIA_CALC),       KEYCODE_RKEY(KEY_MEDIA_PLAY),       KEYCODE_RKEY(KEY_NONE),
    KEYCODE_RKEY(KEY_STOP),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),
    KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),
    KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_VOL_DOWN),         KEYCODE_RKEY(KEY_NONE),
    KEYCODE_RKEY(KEY_VOL_UP),           KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_HOME),             KEYCODE_RKEY(KEY_NONE),
    KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEYPAD_FSLASH),        KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),
    KEYCODE_MKEY(KEYCODE_MOD_RALT),     KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),
    KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),
    KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),
    KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_HOME),
    KEYCODE_RKEY(KEY_UP),               KEYCODE_RKEY(KEY_PAGEUP),           KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_LEFT),
    KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_RIGHT),            KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_END),
    KEYCODE_RKEY(KEY_DOWN),             KEYCODE_RKEY(KEY_PAGEDOWN),         KEYCODE_RKEY(KEY_INSERT),           KEYCODE_RKEY(KEY_DELETE),
    KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),
    KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_MKEY(KEYCODE_MOD_LSUPER),
    KEYCODE_MKEY(KEYCODE_MOD_RSUPER),   KEYCODE_RKEY(KEY_APP),              KEYCODE_RKEY(KEY_POWER),            KEYCODE_RKEY(KEY_SLEEP),
    KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_WAKE),
    KEYCODE_RKEY(KEY_NONE),             KEYCODE_RKEY(KEY_MEDIA_SEARCH),     KEYCODE_RKEY(KEY_MEDIA_FAVORITES),  KEYCODE_RKEY(KEY_MEDIA_REFRESH),
    KEYCODE_RKEY(KEY_MEDIA_STOP),       KEYCODE_RKEY(KEY_MEDIA_FORWARD),    KEYCODE_RKEY(KEY_MEDIA_BACK),       KEYCODE_RKEY(KEY_MEDIA_COMPUTER),
    KEYCODE_RKEY(KEY_MEDIA_EMAIL),      KEYCODE_RKEY(KEY_SELECT),
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

    if (leds & (1 << KEYCODE_CAPS_LOCK)) {
        SET_BIT(led_bits, 2);
    }

    if (leds & (1 << KEYCODE_NUM_LOCK)) {
        SET_BIT(led_bits, 1);
    }

    if (leds & (1 << KEYCODE_SCROLL_LOCK)) {
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
                cmd = ring_buffer_pop(kbd.cmd_buffer);

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
                    keyboard_send_key(set1_to_keycode[scancode], false);
                } else if (scancode >= 0x80 && scancode <= 0xD7) {
                    keyboard_send_key(set1_to_keycode[scancode - 0x80], true);
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
                    keyboard_send_key(set1_extended_to_keycode[scancode - 0x10], false);
                } else if (scancode >= 0x90 && scancode <= 0xED) {
                    keyboard_send_key(set1_extended_to_keycode[scancode - 0x90], true);
                }
                kbd.state = 0; break; 
            }
            break;
        case 2: kbd.state = (scancode == 0xE0 ? 3 : 0); break;
        case 3:
            if (scancode == 0x37 || scancode == 0xAA) {
                keyboard_send_key(KEYCODE_RKEY(KEY_PRTSC), scancode == 0xAA);
            }
            kbd.state = 0; break; 
        // Pause/Break pressed (has no pressed): 0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xC5 
        case 4: kbd.state = (scancode == 0x1D ? 5 : 0); break;
        case 5: kbd.state = (scancode == 0x45 ? 6 : 0); break;
        case 6: kbd.state = (scancode == 0xE1 ? 7 : 0); break;
        case 7: kbd.state = (scancode == 0x9D ? 8 : 0); break;
        case 9:
            if (scancode == 0xC5) {
                keyboard_send_key(KEYCODE_RKEY(KEY_PAUSE), true);
            }
            kbd.state = 0; break;
        default:
            kpanic("PS2 Driver reaching undefined state %u\n", kbd.state);
    }
    // clang-format on
}
