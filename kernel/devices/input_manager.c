/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/interrupts.h>
#include <devices/input_manager.h>
#include <ring_buffer.h>
#include <uapi/errno.h>
#include <utils.h>

#include "internals.h"

#define LOG(fmt, ...) __LOG(1, "[INPUT_MANAGER]", fmt, ##__VA_ARGS__)


// Stores all currently registered input event subscribers
static struct list subscriber_queue = LIST_INIT();

void input_manager_init()
{
    static bool initiated = false;

    if (!initiated) {
        initiated = true;
        // TODO: Create input related dev/kinfo files...
    }
}

/* Allows the input driver to send an input event */
void input_manager_send_event(uint16_t key_code, unsigned char status)
{
    struct list_entry* entry;
    input_event_t      event = {.key_code = key_code, .status = status};

    if (key_code >= VALID_KEY_MAX) {
        LOG("Input manager warning: received invalid keycode: %u\n", key_code);
        return;
    }

    // TODO: fix clang format, curly bracket should be on the same line
    LIST_ITER(&subscriber_queue, entry)
    {
        struct input_subscriber* subscriber = GET_STRUCT(struct input_subscriber, list, entry);

        // How to handle errors?
        subscriber->on_events_received(event);
    }
}

int input_manger_subscribe(struct input_subscriber* subscriber)
{
    if (!subscriber->on_events_received) {
        return -EINVAL;
    }

    list_add(&subscriber_queue, &subscriber->list);
}

void input_manger_unsubscribe(struct input_subscriber* subscriber)
{
    list_remove(&subscriber_queue, &subscriber->list);
}

