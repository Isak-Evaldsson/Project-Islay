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

#define EVENT_QUEUE_SIZE 200

#define INPUT_MANGER_ERR(msg) kprintf("Input manager error: %s\n", msg)

/*
    Debug macro verify initiation
*/
#define VERIFY_QUEUE_INIT() kassert(event_queue.cap == EVENT_QUEUE_SIZE)

// TODO: more dynamic event queue that a fixed size ring buffer
static ring_buff_struct(input_event_t, EVENT_QUEUE_SIZE) event_queue;

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

    // TODO: error handling
    if (!ring_buff_full(event_queue)) {
        ring_buffer_push(event_queue, event);
    } else {
        INPUT_MANGER_ERR("input event queue filled up");
    }

    // TODO: fix clang format, curly bracket should be on the same line
    LIST_ITER(&subscriber_queue, entry)
    {
        struct input_subscriber* subscriber = GET_STRUCT(struct input_subscriber, list, entry);

        // How to handle errors?
        subscriber->on_events_received(event);
    }
}

/* Read event queue, used by the kernel to receive key events */
bool input_manager_get_event(input_event_t* event)
{
    VERIFY_QUEUE_INIT();

    input_event_t e;

    // Check if there's an event in the queue
    if (ring_buff_empty(event_queue))
        return false;

    ring_buffer_pop(event_queue, e);
    *event = e;
    return true;
}

void input_manager_wait_for_event(input_event_t* event)
{
    VERIFY_QUEUE_INIT();

    // Can an event be fetched form the queue
    if (input_manager_get_event(event))
        return;

    // else wait
    do {
        wait_for_interrupt();  // TODO: Implement/replace for a better wait mechanism (once we have
                               // a scheduler with a proper wait queue)
    } while (!input_manager_get_event(event));
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

