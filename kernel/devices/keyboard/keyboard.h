/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
   Inspired by Sebastian Raase's "avrterm-kbd" code (no licence, used with his permission)
*/
#ifndef DEVICES_KEYBOARD_KEYBOARD_H
#define DEVICES_KEYBOARD_KEYBOARD_H

#include <devices/device.h>
#include <devices/input_manager.h>
#include <stdint.h>

/* Object to store common data and functions needed by all keyboard drivers  */
struct keyboard {
    struct device dev;

    // Callback to change the keyboard leds, the different bits within corresponds to leds
    // defined in enum keycode_lock_keys
    void (*set_leds)(unsigned char);
};

/* Send keycodes received by the keyboard and pass then further up the input stack */
void inline keyboard_send_key(uint16_t keycode, bool released)
{
    input_event_t event;
    event.keycode = ((released & 0x01) << 15) | keycode;
    input_manager_send_event(event);
}

/* Initialise the keyboard object. Return 0 on success and -ERRNO on failure */
int keyboard_init(struct keyboard *kbd);

/* Set keyboard leds for all keyboards */
void set_keyboard_leds(unsigned char leds);

/* For driver registration */
extern struct driver keyboard_driver;

#endif /* DEVICES_KEYBOARD_KEYBOARD_H */
