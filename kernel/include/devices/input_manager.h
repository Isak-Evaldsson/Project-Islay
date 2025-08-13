/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef DEVICES_INPUT_H
#define DEVICES_INPUT_H

#include <devices/unicode.h>
#include <list.h>
#include <stdbool.h>
#include <stdint.h>

/*
    Generalised api for key input handling,
    independent on whatever it's a usb devices or ps2 device etc.
*/

/* Representing system input as an input event object */
typedef struct input_event_t {
    // Encodes different types of keycodes with the following bit-pattern:
    // RTTMMMMCCCCCCCCC
    // | | |  |
    // | | |  USB HID Key, see enum keys
    // | | Lock types (see enum keycode_lock_keys ) or Modifiers (see enum keycode_modifiers)
    // | Type: keycode_types
    // Released if high
    uint16_t keycode;
    ucs2_t   ucs2_char;
} input_event_t;

#define KEYCODE_CHECK_RELEASED(keycode) (((keycode) & 0x8000) >> 15)
#define KEYCODE_GET_TYPE(keycode)       (((keycode) & 0x6000) >> 13)
#define KEYCODE_GET_MODIFIER(keycode)   (((keycode) & 0x1e00) >> 9)
#define KEYCODE_GET_KEY(keycode)        (((keycode) & 0x01ff))

#define KEYCODE_CREATE(rel, type, mod, key) \
    (((rel) & 0x01) << 15) | (((type) & 0x03) << 13) | (((mod) & 0x0f) << 9) | ((key) & 0x01ff)

#define KEYCODE_MKEY(mod)  KEYCODE_CREATE(0, KEYCODE_TYPE_MOD, mod, 0)
#define KEYCODE_LKEY(lock) KEYCODE_CREATE(0, KEYCODE_TYPE_LOCK, lock, 0)
#define KEYCODE_RKEY(key)  KEYCODE_CREATE(0, KEYCODE_TYPE_REG, 0, key)

/* The different types of keycodes */
enum keycode_types {
    KEYCODE_TYPE_REG,   // Regular keys
    KEYCODE_TYPE_LOCK,  // Lock keys such as caps lock, scroll lock etc.
    KEYCODE_TYPE_MOD,   // Modifier keys such as lshift, ralt, etc.
};

/* Keycode lock keys, as defined by USB HID usage table 0x08 */
enum keycode_lock_keys {
    KEYCODE_NUM_LOCK    = 0x01,
    KEYCODE_CAPS_LOCK   = 0x02,
    KEYCODE_SCROLL_LOCK = 0x03,
};

/* Keycode modifier keys  */
enum keycode_modifiers {
    KEYCODE_MOD_LCTRL,
    KEYCODE_MOD_RCTRL,
    KEYCODE_MOD_LSHIFT,
    KEYCODE_MOD_RSHIFT,
    KEYCODE_MOD_LALT,
    KEYCODE_MOD_RALT,
    KEYCODE_MOD_LSUPER,
    KEYCODE_MOD_RSUPER,
};

/*
    Keycode keys, as defined by USB HID usage table 0x07
*/
enum keys {
    KEY_NONE        = 0x00,
    ERR_ROLLOVER    = 0x01,
    ERR_POSTFAIL    = 0x02,
    ERR_UNDEF       = 0x03,
    KEY_A           = 0x04,
    KEY_B           = 0x05,
    KEY_C           = 0x06,
    KEY_D           = 0x07,
    KEY_E           = 0x08,
    KEY_F           = 0x09,
    KEY_G           = 0x0a,
    KEY_H           = 0x0b,
    KEY_I           = 0x0c,
    KEY_J           = 0x0d,
    KEY_K           = 0x0e,
    KEY_L           = 0x0f,
    KEY_M           = 0x10,
    KEY_N           = 0x11,
    KEY_O           = 0x12,
    KEY_P           = 0x13,
    KEY_Q           = 0x14,
    KEY_R           = 0x15,
    KEY_S           = 0x16,
    KEY_T           = 0x17,
    KEY_U           = 0x18,
    KEY_V           = 0x19,
    KEY_W           = 0x1a,
    KEY_X           = 0x1b,
    KEY_Y           = 0x1c,
    KEY_Z           = 0x1d,
    KEY_1           = 0x1e,
    KEY_2           = 0x1f,
    KEY_3           = 0x20,
    KEY_4           = 0x21,
    KEY_5           = 0x22,
    KEY_6           = 0x23,
    KEY_7           = 0x24,
    KEY_8           = 0x25,
    KEY_9           = 0x26,
    KEY_0           = 0x27,
    KEY_ENTER       = 0x28,
    KEY_ESCAPE      = 0x29,
    KEY_BACKSPACE   = 0x2a,
    KEY_TAB         = 0x2b,
    KEY_SPACE       = 0x2c,
    KEY_MINUS       = 0x2d,
    KEY_EQUAL       = 0x2e,
    KEY_LBRACKET    = 0x2f,
    KEY_RBRACKET    = 0x30,
    KEY_BSLASH      = 0x31,
    KEY_TILDE       = 0x32,
    KEY_COLON       = 0x33,
    KEY_APOSTROPHE  = 0x34,
    KEY_GRAVE       = 0x35,
    KEY_COMMA       = 0x36,
    KEY_DOT         = 0x37,
    KEY_FSLASH      = 0x38,
    KEY_CAPSLOCK    = 0x39,
    KEY_F1          = 0x3a,
    KEY_F2          = 0x3b,
    KEY_F3          = 0x3c,
    KEY_F4          = 0x3d,
    KEY_F5          = 0x3e,
    KEY_F6          = 0x3f,
    KEY_F7          = 0x40,
    KEY_F8          = 0x41,
    KEY_F9          = 0x42,
    KEY_F10         = 0x43,
    KEY_F11         = 0x44,
    KEY_F12         = 0x45,
    KEY_PRTSC       = 0x46,
    KEY_SCROLLOCK   = 0x47,
    KEY_PAUSE       = 0x48,
    KEY_INSERT      = 0x49,
    KEY_HOME        = 0x4a,
    KEY_PAGEUP      = 0x4b,
    KEY_DELETE      = 0x4c,
    KEY_END         = 0x4d,
    KEY_PAGEDOWN    = 0x4e,
    KEY_RIGHT       = 0x4f,
    KEY_LEFT        = 0x50,
    KEY_DOWN        = 0x51,
    KEY_UP          = 0x52,
    KEY_NUMLOCK     = 0x53,
    KEYPAD_FSLASH   = 0x54,
    KEYPAD_ASTERISK = 0x55,
    KEYPAD_MINUS    = 0x56,
    KEYPAD_PLUS     = 0x57,
    KEYPAD_ENTER    = 0x58,
    KEYPAD_1        = 0x59,
    KEYPAD_2        = 0x5a,
    KEYPAD_3        = 0x5b,
    KEYPAD_4        = 0x5c,
    KEYPAD_5        = 0x5d,
    KEYPAD_6        = 0x5e,
    KEYPAD_7        = 0x5f,
    KEYPAD_8        = 0x60,
    KEYPAD_9        = 0x61,
    KEYPAD_0        = 0x62,
    KEYPAD_DOT      = 0x63,
    KEY_PIPE        = 0x64,
    KEY_APP         = 0x65,
    KEY_POWER       = 0x66,
    KEYPAD_EQUAL    = 0x67,
    /* F13 to F24 */
    KEY_EXE      = 0x74,
    KEY_HELP     = 0x75,
    KEY_MENU     = 0x76,
    KEY_SELECT   = 0x77,
    KEY_STOP     = 0x78,
    KEY_AGAIN    = 0x79,
    KEY_UNDO     = 0x7a,
    KEY_CUT      = 0x7b,
    KEY_COPY     = 0x7c,
    KEY_PASTE    = 0x7d,
    KEY_FIND     = 0x7e,
    KEY_MUTE     = 0x7f,
    KEY_VOL_UP   = 0x80,
    KEY_VOL_DOWN = 0x81,
    // Locking keys: 0x82 - 0x84
    KEYPAD_COMMA = 0x85,
    // Keypad equal: 0x86
    KEY_INT1      = 0x87,
    KEY_INT2      = 0x88,
    KEY_INT3      = 0x89,
    KEY_INT4      = 0x8a,
    KEY_INT5      = 0x8b,
    KEY_INT6      = 0x8c,
    KEY_INT7      = 0x8d,
    KEY_INT8      = 0x8e,
    KEY_INT9      = 0x8f,
    KEY_LANG1     = 0x90,
    KEY_LANG2     = 0x91,
    KEY_LANG3     = 0x92,
    KEY_LANG4     = 0x93,
    KEY_LANG5     = 0x94,
    KEY_LANG6     = 0x95,
    KEY_LANG7     = 0x96,
    KEY_LANG8     = 0x97,
    KEY_LANG9     = 0x98,
    KEY_ERASE     = 0x99,
    KEY_SYSREQ    = 0x9a,
    KEY_CANCEL    = 0x9b,
    KEY_CLEAR     = 0x9c,
    KEY_PRIOR     = 0x9d,
    KEY_RETURN    = 0x9e,
    KEY_SEPARATOR = 0x9f,
    // Out, Oper, Clear/Again, CrSel/Props, ExSel: 0xa0 - 0xa5
    // Miscellaneous keypad keys: 0xb0 - 0xdd
    // Reserved: 0xde - 0xdf
    KEY_LCTRL  = 0xe0,
    KEY_LSHIFT = 0xe1,
    KEY_LALT   = 0xe2,
    KEY_LSUPER = 0xe3,
    KEY_RCTRL  = 0xe4,
    KEY_RSHIFT = 0xe5,
    KEY_RALT   = 0xe6,
    KEY_RSUPER = 0xe7,

    // Extended keys, not part of the HID standard
    KEY_MEDIA_PLAY      = 0xe8,
    KEY_MEDIA_STOP      = 0xe9,
    KEY_MEDIA_NEXT      = 0xea,
    KEY_MEDIA_PREV      = 0xeb,
    KEY_MEDIA_FORWARD   = 0xec,
    KEY_MEDIA_BACK      = 0xed,
    KEY_MEDIA_CALC      = 0xee,
    KEY_MEDIA_REFRESH   = 0xef,
    KEY_MEDIA_SEARCH    = 0xf0,
    KEY_MEDIA_COMPUTER  = 0xf1,
    KEY_MEDIA_EMAIL     = 0xf2,
    KEY_MEDIA_FAVORITES = 0xf3,
    KEY_SLEEP           = 0xf4,
    KEY_WAKE            = 0xf5,
    KEY_MAX,
};

#define KEY_ASCII_PRINTABLE(key) (key >= KEY_SPACE && key <= KEY_TILDE)
#define KEY_LETTER(key)          (key >= KEY_A && key <= KEY_Z)

/* Initialises the input manager */
void input_manager_init();

/* Allows input drivers to send key events to the input driver */
void input_manager_send_event(input_event_t event);

/*
    To read events report to the input manger, an input subscriber object must be created and
    supplied to the registration functions bellow.

    When actively subscribing, the on_events_receive callback will be called once events are
    available.
*/
struct input_subscriber {
    int (*on_events_received)(input_event_t event);
    struct list_entry list;
};

/*
    Register an input subscriber object to actively subscribe on new input events, when a new event
   is available, the objects associated callback will be called.
*/
int input_manger_subscribe(struct input_subscriber* subscriber);

/* Unsubscribe from inputs events, the callback associated to the subscriber object will no longer
 * be called on new input events. */
void input_manger_unsubscribe(struct input_subscriber* subscriber);

#endif /* DEVICES_INPUT_H */
