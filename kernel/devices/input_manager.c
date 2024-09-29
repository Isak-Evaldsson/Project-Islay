/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/interrupts.h>
#include <devices/input_manager.h>
#include <ring_buffer.h>
#include <utils.h>

#define EVENT_QUEUE_SIZE 200

#define INPUT_MANGER_ERR(msg) kprintf("Input manager error: %s\n", msg)

/*
    Debug macro verify initiation
*/
#define VERIFY_QUEUE_INIT() kassert(event_queue.cap == EVENT_QUEUE_SIZE)

// TODO: more dynamic event queue that a fixed size ring buffer
static ring_buff_struct(input_event_t, EVENT_QUEUE_SIZE) event_queue;

void input_manager_init()
{
    static bool initiated = false;

    if (!initiated) {
        initiated = true;
        ring_buff_init(event_queue);
    }
}

/* Allows the input driver to send an input event */
void input_manager_send_event(uint16_t key_code, unsigned char status)
{
    VERIFY_QUEUE_INIT();
    input_event_t event = {.key_code = key_code, .status = status};

    if (key_code > KEY_DELETE) {
        kprintf("Input manager warning: received invalid keycode: %u\n", key_code);
        return;
    }

    // TODO: error handling
    if (!ring_buff_full(event_queue)) {
        ring_buffer_push(event_queue, event);
    } else {
        INPUT_MANGER_ERR("input event queue filled up");
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
