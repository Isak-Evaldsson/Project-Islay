/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef DEVICES_PS2_KEYBOARD_H
#define DEVICES_PS2_KEYBOARD_H
/*
    Interface for a generic ps2 driver, allowing different device/arch specific implementations
*/

// Callback for sending back data to the keyboard
typedef void (*keyboard_receive_data_t)(unsigned char);

/*
    Register a ps2 device, must provide a device and callback allowing the driver to send data back
    to the keyboard.
*/
void ps2_keyboard_register(char *device_name, keyboard_receive_data_t fn);

/*
    Send data from ps2 device, assumes the sender to use scan code set 1
*/
void ps2_keyboard_send(unsigned char scancode);

#endif /* DEVICES_PS2_KEYBOARD_H */
