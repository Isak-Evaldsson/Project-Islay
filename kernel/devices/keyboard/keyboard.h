/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
   Inspired by Sebastian Raase's "avrterm-kbd" code (no licence, used with his permission)
*/
#ifndef DEVICES_KEYBOARD_KEYBOARD_H
#define DEVICES_KEYBOARD_KEYBOARD_H

#include <stdint.h>

enum keyboard_leds {
    LED_CAPS_LOCK,
    LED_SCROLL_LOCK,
    LED_NUM_LOCK,
};

/* Object to store common data and functions needed by all keyboard drivers  */
struct keyboard {
    struct device dev;  // Or should it simply be a list?

    // Callback to change the keyboard leds, the different bits within corresponds to leds defined
    // in enum keyboard_leds
    void (*set_leds)(unsigned char);
};

/* Process events received by the keyboard and pass then further up the input stack */
void keyboard_process_event(uint16_t key_code, unsigned char status);

/* Initialise the keyboard object. Return 0 on success and -ERRNO on failure */
int keyboard_init(struct keyboard *kbd);

/* Get the global keyboard state */
unsigned char get_keyboard_state();

/* Set the global keyboard state */
void set_keyboard_state(unsigned char state);

/* For driver registration */
extern struct driver keyboard_driver;

#endif /* DEVICES_KEYBOARD_KEYBOARD_H */
