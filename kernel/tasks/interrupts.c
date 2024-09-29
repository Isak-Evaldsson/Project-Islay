/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/interrupt.h>
#include <uapi/errno.h>
#include <utils.h>
/*
    High level interrupt management (the arch agnostic part)
*/

/* Enables logging of interrupts */
#define LOG_INTERRUPTS 0

#define LOG(fmt, ...) __LOG(LOG_INTERRUPTS, "[INTERRUPTS]", fmt, ##__VA_ARGS__)

/* State and configuration flags for the interrupt entries */
typedef enum {
    INTERRUPT_ENABLED = 0,  // Is this entry enabled?
    INTERRUPT_CONCURRENT,   // Should multiple bottom halfs on the same interrupt number be able
                            // to run at the same time?
    INTERRUPT_RUNNING,      // Are the bottom half handler currently running?
} interrupt_entry_flags_t;

/* Per interrupt configuration data */
struct interrupt_entry {
    // First part of interrupt, runs in an atomic with interrupts disabled
    top_half_handler_t top_half;

    // Second part of interrupt, runs in a regular reentrant context, and therefor may be put to
    // sleep or interrupted (including by itself).
    bottom_half_handler_t bottom_half;

    interrupt_entry_flags_t flags;
};

static_assert(ARCH_N_INTERRUPTS > 0);
static struct interrupt_entry interrupt_table[ARCH_N_INTERRUPTS];

/*
    Registers an interrupt at the specified interrupt number. The caller needs to provide at least a
    top or a bottom half handler.

    The concurrent decides if there can be multiple bottom halfs handlers belong to the same
    interrupt number in-flight at the same time. If true, every interrupt will lead to a bottom half
    call even if another is already running. If false, an interrupt firing during a bottom half call
   will skip its call the the bottom half.
*/
int register_interrupt_handler(uint32_t interrupt_number, top_half_handler_t top_half,
                               bottom_half_handler_t bottom_half, bool concurrent)
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

    entry = &interrupt_table[interrupt_number];
    if (MASK_BIT(entry->flags, INTERRUPT_ENABLED)) {
        LOG("Trying to overwrite existing interrupt");
        return -EALREADY;
    }

    interrupt_entry_flags_t flags = 1 << INTERRUPT_ENABLED;
    if (concurrent) {
        SET_BIT(flags, INTERRUPT_CONCURRENT);
    }

    entry->bottom_half = bottom_half;
    entry->top_half    = top_half;
    entry->flags = flags;  // Marks the interrupt as enabled, assuming write_once and barriers this
                           // op will be atomic?
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

    kassert(interrupt_number <= ARCH_N_INTERRUPTS);
    scheduler_start_of_interrupt();  // Replace, with some kind atomic section macro

    if (!MASK_BIT(entry->flags, INTERRUPT_ENABLED)) {
        LOG("Unregistered interrupt %u fired, bug?", interrupt_number);
        goto end;
    }

    if (entry->top_half) {
        entry->top_half(state, interrupt_number);
    }

    if (entry->bottom_half) {
        // If concurrent bottom halfs are disable, can it be skipped?
        if (!MASK_BIT(entry->flags, INTERRUPT_CONCURRENT) &&
            MASK_BIT(entry->flags, INTERRUPT_RUNNING)) {
            goto end;
        }

        // Launch bottom half in a reentrant state
        SET_BIT(entry->flags, INTERRUPT_RUNNING);
        disable_interrupts();
        entry->bottom_half(interrupt_number);

        // Re-enable interrupts to make sure that end of interrupt can be done safely
        enable_interrupts();
        CLR_BIT(entry->flags, INTERRUPT_RUNNING);
    }

end:
    // Performs preemption caused by interrupt handlers
    scheduler_end_of_interrupt();
}
