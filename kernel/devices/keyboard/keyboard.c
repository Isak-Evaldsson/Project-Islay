/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
   Inspired by Sebastian Raase's "avrterm-kbd" code (no licence, used with his permission)
*/
#include <devices/input_manager.h>
#include <stdint.h>
#include <uapi/errno.h>
#include <utils.h>

#include "../internals.h"
#include "keyboard.h"

#define LOG(fmt, ...) __LOG(1, "[kbd]", fmt, ##__VA_ARGS__)

void set_keyboard_leds(unsigned char leds)
{
    struct keyboard   *kbd;
    struct list_entry *entry;

    LIST_ITER(&keyboard_driver.devices, entry)
    {
        kbd = GET_STRUCT(struct keyboard, dev, LIST_ENTRY_TO_DEV(entry));
        kbd->set_leds(leds);
    }
}

int keyboard_init(struct keyboard *kbd)
{
    int ret;

    if (kbd->set_leds == NULL) {
        return -EINVAL;
    }

    ret = register_device(&keyboard_driver, &kbd->dev);
    if (ret < 0) {
        return ret;
    }

    input_manager_init();
    return 0;
}

struct driver keyboard_driver = {
    .name = "keyboard",
};
