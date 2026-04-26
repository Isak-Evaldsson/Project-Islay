/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
   Inspired by Sebastian Raase's "avrterm-kbd" code (no licence, used with his permission)
*/
#include <devices/input_manager.h>
#include <stdint.h>
#include <uapi/errno.h>
#include <utils.h>

#include "keyboard.h"

#define LOG(fmt, ...) __LOG(1, "[kbd]", fmt, ##__VA_ARGS__)

static DEFINE_LIST(kbd_list);
static unsigned int kbd_major;

void set_keyboard_leds(unsigned char leds)
{
    struct keyboard   *kbd;
    struct device     *dev;

    LIST_ITER_STRUCT(&kbd_list, kbd, struct keyboard, kbd_list_entry)
    {
        kbd->set_leds(kbd, leds);
    }
}
// TODO: Implment read and wirte
DEFINE_DEVICE_TYPE(kbd, NULL, NULL, NULL, NULL)

int keyboard_init(struct keyboard *kbd, struct device *dev)
{
    int ret;

    if (kbd->set_leds == NULL) {
        return -EINVAL;
    }

    ret = DEVICE_TYPE_BIND_AND_CREATE_FILE(kbd, dev, true);
    if (ret < 0) {
        return ret;
    }

    list_add_last(&kbd_list, &kbd->kbd_list_entry);
    input_manager_init();
    return 0;
}

void keyboard_remove(struct keyboard *kbd)
{
    list_entry_remove(&kbd->kbd_list_entry);
    kpanic("Handle device removal!\n");
}
