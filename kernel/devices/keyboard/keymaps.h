/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#ifndef DEVICES_KEYBOARD_KEYMAPS_H
#define DEVICES_KEYBOARD_KEYMAPS_H

#include <devices/unicode.h>

/* Translate a keycode to a usc2 character using the supplie state */
ucs2_t keymap_get_key(uint16_t keycode, uint8_t modifier_state, uint8_t lock_state);

/* Set global keymap based on the supplied name. returns 0 if it exists, otherwise -ERRNO */
int set_keymap(const char *name);

#endif /* DEVICES_KEYBOARD_KEYMAPS_H */
