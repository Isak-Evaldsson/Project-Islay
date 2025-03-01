/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#ifndef DEVICES_KEYBOARD_KEYMAPS_H
#define DEVICES_KEYBOARD_KEYMAPS_H

#include <devices/unicode.h>

/* Data structure for the keymap related tables */
struct keymap {
    char *name;

    // Tables with 4 columns representing  Regular, Shift, Alt, Ctrl
    ucs2_t regular_keys[53][4];  // The key present on an us keyboard
    ucs2_t int_keys[18][4];      // For international/lang keycodes
};

/* Pointer to the built-in standard keymap */
extern struct keymap *default_keymap;

/* Translate a keycode to a usc2 character using the supplied keymap */
ucs2_t keymap_get_key(struct keymap *keymap, uint16_t keycode);

#endif /* DEVICES_KEYBOARD_KEYMAPS_H */
