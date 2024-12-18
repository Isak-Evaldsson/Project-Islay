/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/interrupts.h>
#include <atomics.h>
#include <uapi/errno.h>
#include <utils.h>

#include "internal.h"

/*
    High level interrupt management (the arch agnostic part)
*/

/* Enables logging of interrupts */
#define LOG_INTERRUPTS 1

#define LOG(fmt, ...) __LOG(LOG_INTERRUPTS, "[INTERRUPTS]", fmt, ##__VA_ARGS__)

/* State and configuration flags for the interrupt entries */
typedef enum {
    INTERRUPT_ENABLED = 0,  // Is this entry enabled?
    INTERRUPT_QUEUED,       // Are the bottom half already queued
} interrupt_entry_flags_t;

/* Per interrupt configuration data */
struct interrupt_entry {
    // First part of interrupt, runs in an atomic with interrupts disabled
    top_half_handler_t top_half;

    // Second part of interrupt, runs in a regular reentrant context, and therefor may be put to
    // sleep or interrupted (including by itself).
    bottom_half_handler_t bottom_half;

    atomic_uint_t           flags;
    struct interrupt_entry *next;
};

static_assert(ARCH_N_INTERRUPTS > 0);
static struct interrupt_entry interrupt_table[ARCH_N_INTERRUPTS];

/* The level of interrupt nesting */
static unsigned int interrupt_level;

/*
    Linked list for serializing second halfs
*/
static struct interrupt_entry *bottom_half_queue_head = NULL;
static struct interrupt_entry *bottom_half_queue_tail = NULL;

/*
    Registers an interrupt at the specified interrupt number. The caller needs to provide at least a
    top or a bottom half handler.
*/
int register_interrupt_handler(uint32_t interrupt_number, top_half_handler_t top_half,
                               bottom_half_handler_t bottom_half)
{
    struct interrupt_entry *entry;

    if (interrupt_number >= ARCH_N_INTERRUPTS) {
        LOG("Invalid interrupt number %u", interrupt_number);
        return -EINVAL;
    }

    if (!top_half && !bottom_half) {
        LOG("Both top and bottom handlers are NULL");
        return -EINVAL;
    }

    if (verify_valid_interrupt(interrupt_number) < 0) {
        LOG("Invalid interrupt number %u", interrupt_number);
        return -EINVAL;
    }

    entry = &interrupt_table[interrupt_number];
    if (atomic_load(&entry->flags) & (1 << INTERRUPT_ENABLED)) {
        LOG("Trying to overwrite existing interrupt");
        return -EALREADY;
    }

    interrupt_entry_flags_t flags = 1 << INTERRUPT_ENABLED;

    entry->bottom_half = bottom_half;
    entry->top_half    = top_half;
    entry->next        = NULL;

    // Could probably be relaxed to a release operation
    atomic_store(&entry->flags, flags);
    return 0;
}

/*
    Provides an architecture independet interrupt mechanism proving a atomic top half and a
    reentrant bottom half. Is supposed to be called by the arch specific low level interrupt code.

    How this is called is up to the arch specific low level interrupt code, however it assumes it
    runs in an atomic context (i.e. INTERRUPTS MUST BE DISABLED).
 */
void generic_interrupt_handler()
{
    struct interrupt_stack_state *state            = ARCH_GET_INTERRUPT_STACK_STATE();
    unsigned int                  interrupt_number = ARCH_GET_INTERRUPT_NUMBER(state);
    struct interrupt_entry       *entry            = &interrupt_table[interrupt_number];

    /*
        Three levels of interrupts are allowed:
        0:  No interrupt is currently running, the cpu currently a executes with a task context .
        1:  A interrupt fires while a task is running, can run both top and bottom half.
        2:  A interrupt fires while another interrupt runs a bottom half, only allowed to execute a
            top half.
    */
    interrupt_level++;
    kassert(interrupt_level <= 2);

    kassert(interrupt_number <= ARCH_N_INTERRUPTS);
    scheduler_start_of_interrupt();  // Replace, with some kind atomic section macro

    if (!(atomic_load(&entry->flags) & (1 << INTERRUPT_ENABLED))) {
        LOG("Unregistered interrupt %u fired, bug?", interrupt_number);
        goto end;
    }

    LOG("N: %u, L: %u", interrupt_number, interrupt_level);

    if (entry->top_half) {
        entry->top_half(state, interrupt_number);
    }

    if (interrupt_level == 2) {
        if (entry->bottom_half && !(atomic_load(&entry->flags) & (1 << INTERRUPT_QUEUED))) {
            /*
                Bottom half interrupts are not allowed to run on top of each others, instead they
               are queued and executed on level 1.
            */
            if (bottom_half_queue_head == NULL) {
                bottom_half_queue_head = entry;
            } else {
                bottom_half_queue_tail->next = entry;
            }

            bottom_half_queue_tail = entry;
            entry->next            = NULL;

            atomic_or_fetch(&entry->flags, 1 << INTERRUPT_QUEUED);
        }
        goto level2_end;
    }

    if (entry->bottom_half) {
        enable_interrupts();  // TODO: Should this have barriers?
        entry->bottom_half(interrupt_number);
    }

    while (true) {
        /*
            This disabling and re-enabling of interrupts is needed to avoid race conditions with
            parallel level 2 upper halfs. However, it's not very efficient. A lock free queue would
            be a better approach. Also, this loop could theoretically spin forever, some kind of
            limit to the number of halfs could potentially be needed.
        */
        disable_interrupts();
        entry = bottom_half_queue_head;
        if (entry == NULL) {
            break;  // Will always exit the loop with interrupts disabled
        }

        bottom_half_queue_head = entry->next;
        if (bottom_half_queue_head == NULL) {
            bottom_half_queue_tail = NULL;
        }
        entry->next = NULL;

        // The bottom half may not be executed by its original interrupt, so it's interrupt number
        // needs to be computed.
        uint32_t bottom_half_number =
            ((uintptr_t)entry - (uintptr_t)interrupt_table) / sizeof(struct interrupt_entry);

        enable_interrupts();
        entry->bottom_half(bottom_half_number);

        // Clear queued flag
        atomic_and_fetch(&entry->flags, ~(1u << INTERRUPT_QUEUED));
    }

end:
    if (interrupt_level == 2) {
        /*
            Since level 2 interrupts runs on top of level 1 interrupts, some checks can skipped
            since they will be done at the end of the level 1 interrupt anyways
        */
        goto level2_end;
    }

    // TODO: Rename to something better, indicating that it checks for preemption, or maybe inline
    // it since have access to the extern task pointer
    scheduler_end_of_interrupt();

level2_end:
    interrupt_level--;
}
