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

enum ps2_modifier_bits {
    LSHIFT,
    RSHIFT,
    LCTRL,
    RCTRL,
    LALT,
    RALT,
    LSUPER,
    RSUPER,
};

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
    unsigned int modifiers;

    // Managing commands to the keyboard
    keyboard_send_cmd_t send_cmd;
    ring_buff_struct(uint8_t, KBD_CMD_BUFF_SIZE) cmd_buffer;

    // TODO: Add proper locking and timeouts when sending commands...
};

/* maps set 1 scan codes to input event key codes */
static const uint8_t set1_to_keycode[] = {
    // clang-format off
    INVALID_KEY,            KEY_ESCAPE,             KEY_1,                  KEY_2,      
    KEY_3,                  KEY_4,                  KEY_5,                  KEY_6,                
    KEY_7,                  KEY_8,                  KEY_9,                  KEY_0,                  
    KEY_MINUS,              KEY_EQUAL,              KEY_BACKSPACE,          KEY_TAB,       
    KEY_Q,                  KEY_W,                  KEY_E,                  KEY_R,
    KEY_T,                  KEY_Y,                  KEY_U,                  KEY_I,      
    KEY_O,                  KEY_P,                  KEY_LSBRACKET,          KEY_RSBRACKET,
    KEY_ENTER,              /*LCTRL*/INVALID_KEY,   KEY_A,                  KEY_S,
    KEY_D,                  KEY_F,                  KEY_G,                  KEY_H,
    KEY_J,                  KEY_K,                  KEY_L,                  KEY_SEMI,
    KEY_APOSTROPHE,         KEY_BACKTICK,           /*LSHIFT*/INVALID_KEY,  KEY_BSLASH, 
    KEY_Z,                  KEY_X,                  KEY_C,                  KEY_V,                  
    KEY_B,                  KEY_N,                  KEY_M,                  KEY_COMMA,  
    KEY_DOT,                KEY_FSLASH,             /*RSHIFT*/INVALID_KEY,  KEY_ASTERISK,
    /*LALT*/INVALID_KEY,    KEY_SPACE,              KEY_CAPS,               KEY_F1,
    KEY_F2,                 KEY_F3,                 KEY_F4,                 KEY_F5,    
    KEY_F6,                 KEY_F7,                 KEY_F8,                 KEY_F9,                 
    KEY_F10,                KEY_NUMLOCK,            KEY_SCROLLOCK,          KEY_7,
    KEY_8,                  KEY_9,                  KEY_MINUS,              KEY_4,
    KEY_5,                  KEY_6,                  KEY_PLUS,               KEY_1,
    KEY_2,                  KEY_3,                  KEY_0,                  KEY_DOT,
    INVALID_KEY,            INVALID_KEY,            INVALID_KEY,            KEY_F11,
    KEY_F12
    //clang-format on
};

/* maps set 1 extended (E0, 0x10 + index) scan codes to input event key codes */
static const uint8_t set1_extended_to_keycode[] = {
    // clang-format off
    KEY_MM_PREV,            INVALID_KEY,            INVALID_KEY,        INVALID_KEY,
    INVALID_KEY,            INVALID_KEY,            INVALID_KEY,        INVALID_KEY,
    INVALID_KEY,            KEY_MM_NEXT,            INVALID_KEY,        INVALID_KEY,
    KEY_ENTER,              /*RCTRL*/INVALID_KEY,   INVALID_KEY,        INVALID_KEY,
    KEY_MM_MUTE,            KEY_MM_CALC,            KEY_MM_PLAY,        INVALID_KEY,
    KEY_MM_STOP,            INVALID_KEY,            INVALID_KEY,        INVALID_KEY,
    INVALID_KEY,            INVALID_KEY,            INVALID_KEY,        INVALID_KEY,
    INVALID_KEY,            INVALID_KEY,            KEY_MM_VOL_DOWN,    INVALID_KEY,
    KEY_MM_VOL_UP,          INVALID_KEY,            KEY_MM_HOME,        INVALID_KEY,
    INVALID_KEY,            KEY_FSLASH,             INVALID_KEY,        INVALID_KEY,
    /*RALT*/INVALID_KEY,    INVALID_KEY,            INVALID_KEY,        INVALID_KEY,
    INVALID_KEY,            INVALID_KEY,            INVALID_KEY,        INVALID_KEY,
    INVALID_KEY,            INVALID_KEY,            INVALID_KEY,        INVALID_KEY,
    INVALID_KEY,            INVALID_KEY,            INVALID_KEY,        KEY_HOME,
    KEY_UP,                 KEY_PAGEUP,             INVALID_KEY,        KEY_LEFT,
    INVALID_KEY,            KEY_RIGHT,              INVALID_KEY,        KEY_END,
    KEY_DOWN,               KEY_PAGEDOWN,           KEY_INSERT,         KEY_DELETE,
    INVALID_KEY,            INVALID_KEY,            INVALID_KEY,        INVALID_KEY,
    INVALID_KEY,            INVALID_KEY,            INVALID_KEY,        /*LSUPER*/INVALID_KEY,
    /*RSUPER*/INVALID_KEY,  KEY_MM_APPS,            KEY_ACPI_POWER,     KEY_ACPI_SLEEP,
    INVALID_KEY,            INVALID_KEY,            INVALID_KEY,        KEY_ACPI_WAKE,
    INVALID_KEY,            KEY_MM_SEARCH,          KEY_MM_FAVORITES,   KEY_MM_REFRESH,
    KEY_MM_STOP,            KEY_MM_FORWARD,         KEY_MM_BACK,        KEY_MM_COMPUTER,
    KEY_MM_EMAIL,           KEY_MM_SELECT,
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
    uint16_t      cmd      = 0;
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

/* Helper function, that sends key event to the input manger based on the supplied keycode, handles
   scancode independet internal state such as caps_lock etc. */
void send_event(uint16_t keycode, bool released)
{
    unsigned char status = 0;

    if (keycode == INVALID_KEY) {
        LOG("received invalid keycode");
        return;
    }

    if ((kbd.modifiers & (1 << LSHIFT)) || (kbd.modifiers & (1 << RSHIFT))) {
        SET_BIT(status, MOD_SHIFT);
    }

    if ((kbd.modifiers & (1 << LCTRL)) || (kbd.modifiers & (1 << RCTRL))) {
        SET_BIT(status, MOD_CTRL);
    }

    if ((kbd.modifiers & (1 << LALT)) || (kbd.modifiers & (1 << RALT))) {
        SET_BIT(status, MOD_ALT);
    }

    if ((kbd.modifiers & (1 << LSUPER)) || (kbd.modifiers & (1 << RSUPER))) {
        SET_BIT(status, MOD_SUPER);
    }

    // Covert scancode modifiers to os key status
    if (released) {
        SET_BIT(status, INPUT_RELEASED);
    }

    keyboard_process_event(&kbd.kbd, keycode, status);
}

void ps2_keyboard_send(unsigned char scancode)
{
    uint8_t cmd;

    // Keyboard state machine
    // clang-format off
    switch (kbd.state) {
        case 0:  // Initial state, regular keys
            switch (scancode) {
            case 0x2A: SET_BIT(kbd.modifiers, LSHIFT); kbd.state = 0; break;
            case 0x36: SET_BIT(kbd.modifiers, RSHIFT); kbd.state = 0; break;
            case 0x1D: SET_BIT(kbd.modifiers, LCTRL);  kbd.state = 0; break;
            case 0x38: SET_BIT(kbd.modifiers, LALT);   kbd.state = 0; break;
            case 0xAA: CLR_BIT(kbd.modifiers, LSHIFT); kbd.state = 0; break;
            case 0xB6: CLR_BIT(kbd.modifiers, RSHIFT); kbd.state = 0; break;
            case 0x9D: CLR_BIT(kbd.modifiers, LCTRL);  kbd.state = 0; break;
            case 0xB8: CLR_BIT(kbd.modifiers, LALT);   kbd.state = 0; break;
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
                    send_event(set1_to_keycode[scancode], false);
                } else if (scancode >= 0x80 && scancode <= 0xD7) {
                    send_event(set1_to_keycode[scancode - 0x80], true);
                }
                kbd.state = 0; break;
            }
            break;
        case 1: // Extended keys, media, apci, etc.
            switch (scancode) {
            case 0x1D: SET_BIT(kbd.modifiers, RCTRL);   kbd.state = 0; break;
            case 0x38: SET_BIT(kbd.modifiers, RALT);    kbd.state = 0; break;
            case 0x5B: SET_BIT(kbd.modifiers, LSUPER);  kbd.state = 0; break;
            case 0x5C: SET_BIT(kbd.modifiers, RSUPER);  kbd.state = 0; break;
            case 0x9D: CLR_BIT(kbd.modifiers, RCTRL);   kbd.state = 0; break;
            case 0xB8: CLR_BIT(kbd.modifiers, RALT);    kbd.state = 0; break;
            case 0xDB: CLR_BIT(kbd.modifiers, LSUPER);  kbd.state = 0; break;
            case 0xDC: CLR_BIT(kbd.modifiers, RSUPER);  kbd.state = 0; break;
            case 0x2A: kbd.state = 2; break; // Print screen pressed: 0xE0, 0x2A, 0xE0, 0x37
            case 0xB7: kbd.state = 2; break; // Print screen released: 0xE0, 0xB7, 0xE0, 0xAA
            default: 
                if (scancode >= 0x10 && scancode <= 0x6D) {
                    send_event(set1_extended_to_keycode[scancode - 0x10], false);
                } else if (scancode >= 0x90 && scancode <= 0xED) {
                    send_event(set1_extended_to_keycode[scancode - 0x90], true);
                }
                kbd.state = 0; break; 
            }
            break;
        case 2: kbd.state = (scancode == 0xE0 ? 3 : 0); break;
        case 3:
            if (scancode == 0x37 || scancode == 0xAA) {
                send_event(KEY_PRTSC, scancode == 0xAA);
            }
            kbd.state = 0; break; 
        // Pause/Break pressed (has no pressed): 0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xC5 
        case 4: kbd.state = (scancode == 0x1D ? 5 : 0); break;
        case 5: kbd.state = (scancode == 0x45 ? 6 : 0); break;
        case 6: kbd.state = (scancode == 0xE1 ? 7 : 0); break;
        case 7: kbd.state = (scancode == 0x9D ? 8 : 0); break;
        case 9:
            if (scancode == 0xC5) {
                send_event(KEY_BREAK, true);
            }
            kbd.state = 0; break;
        default:
            kpanic("PS2 Driver reaching undefined state %u\n", kbd.state);
    }
    // clang-format on
}
