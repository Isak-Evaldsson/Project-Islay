/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#include <devices/input_manager.h>

#include "keymaps.h"

/*
    TODO/Improvements:
     1. Use hashmap instead of static table to save space
     2. Support Alt+Shift
     3. Distinguish between alt + alt gr
     4. Add some kind of keymap switching mechanism, within kinfo?
*/

// clang-format off
static struct keymap us_keymap = {
    .name = "iso-us",
    .regular_keys = {
        // None         Shift           Alt             Ctrl
        {0x0061,        0x0041,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_A
        {0x0062,        0x0042,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_B
        {0x0063,        0x0043,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_C
        {0x0064,        0x0044,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_D
        {0x0065,        0x0045,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_E
        {0x0066,        0x0046,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_F
        {0x0067,        0x0047,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_G
        {0x0068,        0x0048,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_H
        {0x0069,        0x0049,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_I
        {0x006A,        0x004A,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_J
        {0x006B,        0x004B,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_K
        {0x006C,        0x004C,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_L
        {0x006D,        0x004D,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_M
        {0x006E,        0x004E,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_N
        {0x006F,        0x004F,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_O
        {0x0070,        0x0050,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_P
        {0x0071,        0x0051,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_Q
        {0x0072,        0x0052,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_R
        {0x0073,        0x0053,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_S
        {0x0074,        0x0054,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_T
        {0x0075,        0x0055,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_U
        {0x0076,        0x0056,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_V
        {0x0077,        0x0057,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_W
        {0x0078,        0x0058,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_X
        {0x0079,        0x0059,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_Y
        {0x007A,        0x005A,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_Y
        {0x0031,        0x0021,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_1
        {0x0032,        0x0040,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_2
        {0x0033,        0x0023,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_3
        {0x0034,        0x0024,         0x0080,         UCS2_NOCHAR}, // KEY_4
        {0x0035,        0x0025,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_5
        {0x0036,        0x005e,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_6
        {0x0037,        0x0026,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_7
        {0x0038,        0x002A,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_8
        {0x0039,        0x0028,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_9
        {0x0030,        0x0029,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_0
        {0x000A,        UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_ENTER
        {0x000B,        UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_ESCAPE
        {0x0008,        UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_BACKSPACE
        {0x0009,        UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_TAB
        {0x0020,        UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_SPACE
        {0x002D,        0x005F,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_MINUS
        {0x003D,        0x002B,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_EQUAL
        {0x005B,        0x007B,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_LBRACKET
        {0x005D,        0x007D,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_RBRACKET
        {0x005C,        0X007C,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_BSLASH
        {UCS2_NOCHAR,   UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_TILDE
        {0x003B,        0x003A,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_COLON
        {0x0027,        0x0022,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_APOSTROPHE
        {0x0060,        0x007E,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_GRAVE
        {0x002C,        0x003C,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_COMMA
        {0x002E,        0x003E,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_DOT
        {0x002F,        0x003F,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_FSLASH
    },  
    .int_keys = {
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT1  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT2
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT3  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT4  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT5  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT6  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT7
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT8  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT9  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG1
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG2
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG3  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG4  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG5  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG6  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG7
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG8  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG9 
    },
};  // clang-format on

// clang-format off
static struct keymap swe_keymap = { 
    .name = "iso-swe",
    .regular_keys = {
        // None         Shift           Alt             Ctrl
        {0x0061,        0x0041,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_A
        {0x0062,        0x0042,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_B
        {0x0063,        0x0043,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_C
        {0x0064,        0x0044,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_D
        {0x0065,        0x0045,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_E
        {0x0066,        0x0046,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_F
        {0x0067,        0x0047,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_G
        {0x0068,        0x0048,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_H
        {0x0069,        0x0049,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_I
        {0x006A,        0x004A,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_J
        {0x006B,        0x004B,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_K
        {0x006C,        0x004C,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_L
        {0x006D,        0x004D,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_M
        {0x006E,        0x004E,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_N
        {0x006F,        0x004F,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_O
        {0x0070,        0x0050,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_P
        {0x0071,        0x0051,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_Q
        {0x0072,        0x0052,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_R
        {0x0073,        0x0053,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_S
        {0x0074,        0x0054,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_T
        {0x0075,        0x0055,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_U
        {0x0076,        0x0056,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_V
        {0x0077,        0x0057,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_W
        {0x0078,        0x0058,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_X
        {0x0079,        0x0059,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_Y
        {0x007A,        0x005A,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_Y
        {0x0031,        0x0021,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_1
        {0x0032,        0x0022,         0x0040,         UCS2_NOCHAR}, // KEY_2
        {0x0033,        0x0023,         0x00A3,         UCS2_NOCHAR}, // KEY_3
        {0x0034,        0x00A4,         0x0024,         UCS2_NOCHAR}, // KEY_4
        {0x0035,        0x0025,         0x0080,         UCS2_NOCHAR}, // KEY_5
        {0x0036,        0x0026,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_6
        {0x0037,        0x002F,         0x007B,         UCS2_NOCHAR}, // KEY_7
        {0x0038,        0x0028,         0x005B,         UCS2_NOCHAR}, // KEY_8
        {0x0039,        0x0029,         0x005D,         UCS2_NOCHAR}, // KEY_9
        {0x0030,        0x003D,         0x007D,         UCS2_NOCHAR}, // KEY_0
        {0x000A,        UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_ENTER
        {0x000B,        UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_ESCAPE
        {0x0008,        UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_BACKSPACE
        {0x0009,        UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_TAB
        {0x0020,        UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_SPACE
        {0x002B,        0x003F,         0x005C,         UCS2_NOCHAR}, // KEY_MINUS
        {0x00B4,        0x0060,         0x00B1,         UCS2_NOCHAR}, // KEY_EQUAL
        {0x00E5,        0x00C5,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_LBRACKET
        {0x00A8,        0x005E,         0x007E,         UCS2_NOCHAR}, // KEY_RBRACKET
        {0x0027,        0X002A,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_BSLASH
        {UCS2_NOCHAR,   UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_TILDE
        {0x00F6,        0x00D6,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_COLON
        {0x00E4,        0x00C4,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_APOSTROPHE
        {0x0060,        0x007E,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_GRAVE
        {0x002C,        0x003B,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_COMMA
        {0x002E,        0x003A,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_DOT
        {0x002D,        0x005F,         UCS2_NOCHAR,    UCS2_NOCHAR}, // KEY_FSLASH
    },
    .int_keys = {
        {0x003C     ,  0x003E,         0x007C,         UCS2_NOCHAR},    // KEY_INT1  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT2
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT3  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT4  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT5  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT6  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT7
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT8  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_INT9  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG1
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG2
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG3  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG4  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG5  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG6  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG7
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG8  
        {UCS2_NOCHAR,  UCS2_NOCHAR,    UCS2_NOCHAR,    UCS2_NOCHAR},    // KEY_LANG9 
    },
};  // clang-format on

static ucs2_t keypad_map[16][2] = {
    // num-lock on, num-lock off
    {0x002F, 0x002F     }, // KEYPAD_FSLASH
    {0x002A, 0x002A     }, // KEYPAD_ASTERISK
    {0x002D, 0x002D     }, // KEYPAD_MINUS
    {0x002B, 0x002B     }, // KEYPAD_PLUS
    {0x000A, 0x000A     }, // KEYPAD_ENTER
    {0x0031, UCS2_NOCHAR}, // KEYPAD_1
    {0x0032, UCS2_NOCHAR}, // KEYPAD_2
    {0x0033, UCS2_NOCHAR}, // KEYPAD_3
    {0x0034, UCS2_NOCHAR}, // KEYPAD_4
    {0x0035, UCS2_NOCHAR}, // KEYPAD_5
    {0x0036, UCS2_NOCHAR}, // KEYPAD_6
    {0x0037, UCS2_NOCHAR}, // KEYPAD_7
    {0x0038, UCS2_NOCHAR}, // KEYPAD_8
    {0x0039, UCS2_NOCHAR}, // KEYPAD_9
    {0x0030, UCS2_NOCHAR}, // KEYPAD_0
    {0x002E, UCS2_NOCHAR}, // KEYPAD_DOT
};

struct keymap *default_keymap = &us_keymap;

ucs2_t keymap_get_key(struct keymap *keymap, uint16_t keycode)
{
    uint8_t status = KEYCODE_GET_STATUS(keycode);
    uint8_t key    = KEYCODE_GET_KEY(keycode);

    if (key >= KEYPAD_FSLASH && key <= KEYPAD_DOT) {
        return keypad_map[key - KEYPAD_FSLASH][(status & (1 << STATUS_NUMLOCK)) ? 0 : 1];
    }

    if ((key >= KEY_A && key <= KEY_FSLASH) || (key >= KEY_INT1 && key <= KEY_LANG9)) {
        int index = 0;

        if (key <= KEY_Z || key >= KEY_INT1) {
            if (status & (1 << STATUS_CAPSLOCK)) {
                index = 1;
            }
        }

        if (status & (1 << STATUS_MOD_SHIFT)) {
            index = !index;
        }
        if (status & (1 << STATUS_MOD_ALT)) {
            index = 2;
        }
        if (status & (1 << STATUS_MOD_CTRL)) {
            index = 3;
        }

        if (key >= KEY_INT1) {
            return keymap->int_keys[key - KEY_INT1][index];
        }
        return keymap->regular_keys[key - KEY_A][index];
    }

    return UCS2_NOCHAR;
}
