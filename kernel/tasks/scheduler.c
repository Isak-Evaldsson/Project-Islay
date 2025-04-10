/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/interrupts.h>
#include <arch/paging.h>
#include <arch/thread.h>
#include <devices/timer.h>
#include <memory/vmem_manager.h>
#include <tasks/locking.h>
#include <tasks/scheduler.h>
#include <tasks/task_queue.h>
#include <utils.h>

#include "internal.h"

#define LOG_SCHEDULER 1

#define LOG(fmt, ...) __LOG(LOG_SCHEDULER, "[SCHEDULER]", fmt, ##__VA_ARGS__)

/*
    Scheduler state
*/

/*
   TODO:
   1. Double-check so that the time counting works correctly
   2. Better algorithm than round robin
 */

// The ready-to-run task queue
static EMPTY_QUEUE(ready_queue);

// Sleep queue
static EMPTY_QUEUE(sleep_queue);

/*
 * Task waiting to be terminated. The regular task queue wraps the list strucure and add some extra
 * checks and a refcount. The purpose of the refcount is to make sure that a task object can't be
 * free'd while in a task queue, if that was used for the termination queue, the refcount would
 * never be 0, preventing the cleanup thread from doing its job.
 */
static DEFINE_LIST(termination_queue);

// Counter variables keeping track of the time consumption of each task
static uint64_t last_count    = 0;
static uint64_t current_count = 0;

// Counts how many ns the CPU has been idle
static uint64_t idle_time_ns = 0;

// Pointer to the currently running task
task_t *current_task;

/* Stores at which timestamp the currently running task shall be preempted, if set to zero indicate
 * to never preempt */
static uint64_t preemption_timestamp_ns = 0;

// Variables handling if the scheduler needs to postpone task switches. Necessary if you want to
// unblock multiple tasks within without running the risk of the first unblocked task preempting the
// task the unblocks the rest of the task.
unsigned int postpone_task_switch_counter = 0;  // If above 0 we're in a postpone state
bool         task_switch_postponed = false;     // Any tries to task switch during postpone state?

/* Variable storing the earliest wakeup time (in ns since boot) for any sleeping task. Allows
 * clock drivers check if it's necessary to call the 'scheduler_check_sleep_queue' */
uint64_t scheduler_earliest_wakeup = UINT64_MAX;

/* Task responsible of freeing up space for task within the termination queue */
static task_t *cleanup_task = NULL;

/* Flag indicating if the scheduler has been initialised */
bool scheduler_initialised = false;

// Performs a context switch by switching the current running one to a new task
static void switch_task(task_t *new_task)
{
    task_t *old_task;

    // Done we need to postpone the task switch?
    if (postpone_task_switch_counter > 0) {
        task_switch_postponed = true;  // tell scheduler that we have postponed a task switch

        // Add our task first in queue, so it get's switched to once we're ready to call schedule
        // again
        LOG("Postponing switch to %x, added first in ready queue", new_task);
        task_queue_add_first(&ready_queue, new_task);
        return;
    }

    // TODO: Handle calls to switch task when in sleep mode, i.e. current_task == NULL
    kassert(current_task != NULL);
    // Re-add current task to list if it's still in running state
    if (current_task->state == RUNNING) {
        current_task->state = READY_TO_RUN;
        task_queue_enqueue(&ready_queue, current_task);
    }

    if (current_task == NULL) {
        // Task unblocked and stopped us being idle, so only one task can be running
        preemption_timestamp_ns = 0;
    } else if (TASK_QUEUE_EMPTY(&ready_queue) && (current_task->state != RUNNING)) {
        // Currently running task blocked and the task we're switching to is the only task left
        preemption_timestamp_ns = 0;
    } else {
        // More than one task wants the CPU, so set a time slice length
        preemption_timestamp_ns = timer_get_time_since_boot() + TIME_SLICE_NS;
    }

    // Switch state for our new task
    new_task->state = RUNNING;

    // Switch current task
    old_task     = current_task;
    current_task = new_task;

    LOG("Swich task from %x to %x, preemption timestamp %u", old_task, new_task,
        preemption_timestamp_ns);
    kernel_thread_switch(&new_task->regs, &old_task->regs);
}

/* Lock scheduler, blocks the task switching process from becoming disturbed by interrupts */
void scheduler_lock(uint32_t *interrupt_flags)
{
#ifndef SMP
    // Disable interrupts and save the previous state
    *interrupt_flags = get_register_and_disable_interrupts();
#endif
}

/* Unlock scheduler, allows interrupts */
void scheduler_unlock(uint32_t interrupt_flags)
{
#ifndef SMP
    // Restore interrupt flags before locking
    restore_interrupt_register(interrupt_flags);
#endif
}

// Marks the start of a critical region, during it's execution we can't perform task switches nor
// interrupts
void critical_section_start(uint32_t *interrupt_flags)
{
#ifndef SMP
    scheduler_lock(interrupt_flags);
    postpone_task_switch_counter++;
#endif
}

// Marks the end of a critical region, during it's execution we can't perform task switches nor
// interrupts
void critical_section_end(uint32_t interrupt_flags)
{
#ifndef SMP
    postpone_task_switch_counter--;

    // Are there any more task that require task switching to be postponed
    if (postpone_task_switch_counter == 0) {
        // No, do have we have task switches waiting?
        if (task_switch_postponed) {
            task_switch_postponed = false;
            schedule();
        }
    }

    scheduler_unlock(interrupt_flags);
#endif
}

void scheduler_block_task(unsigned int reason)
{
    uint32_t flags;
    scheduler_lock(&flags);

    LOG("Block task %x, reason %u", current_task, reason);
    current_task->state = reason;
    schedule();

    scheduler_unlock(flags);
}

void scheduler_unblock_task(task_t *task)
{
    uint32_t flags;
    scheduler_lock(&flags);
    LOG("Unblock task %x", task);

    // Only enqueue if the task isn't already in the queue
    if (task->state != READY_TO_RUN) {
        task->state = READY_TO_RUN;

        // Never preempt, always append to end of queue
        LOG("Unblock: put %x in ready_queue", task);
        task_queue_enqueue(&ready_queue, task);
    }

    // Ensure that the currently running thread is preempted, maybe do something smarter like having
    // a better time remaining system

    // If the current task is running and not idle, and it's never preempted (timestamp == 0) make
    // sure it's preempted so the new process is allowed to run
    if (current_task != NULL && preemption_timestamp_ns == 0) {
        preemption_timestamp_ns = timer_get_time_since_boot() + TIME_SLICE_NS;
    }

    scheduler_unlock(flags);
}

static void update_time_used()
{
    current_count    = timer_get_time_since_boot();
    uint64_t elapsed = current_count - last_count;
    last_count       = current_count;

    if (current_task == NULL) {
        idle_time_ns += elapsed;
    } else {
        current_task->time_used += elapsed;
    }
}

// Note, is the caller to schedule that is responsible for correct locking of the scheduler
void schedule()
{
    task_t *task;
    update_time_used();

    // Done we need to postpone the task switch?
    if (postpone_task_switch_counter > 0) {
        task_switch_postponed = true;  // tell scheduler that we have postponed a call to schedule()
        return;
    }

    // Are there any new tasks to schedule
    task = task_queue_dequeue(&ready_queue);
    if (task != NULL) {
        switch_task(task);  // Perform the context switch
    } else if (current_task->state == RUNNING) {
        // Otherwise, if the task is in running state, let it continue
    } else {
        // The current running task is block and there's no other task to be run
        task                    = current_task;
        current_task            = NULL;  // mark that CPU currently sleeps
        preemption_timestamp_ns = 0;     // disable preemption checking

        LOG("Enter sleep state");

        kassert(TASK_QUEUE_EMPTY(&ready_queue));

        // Do nothing while waiting for a task to unblock and become "ready to run".  The only
        // thing that is going to update the ready queue is going to be from a timer IRQ
        // (with a single processor anyway), but interrupts are disabled. The timer must be
        // allowed to fire, but do not allow any task changes to take place.  The
        // task_switches_postponed_flag must remain set to force the timer to return to this
        // loop.

        do {
            enable_interrupts();   // enable interrupts to allow the timer to fire
            wait_for_interrupt();  // halt and wait for the timer to fire
            disable_interrupts();  // disable interrupts again to see if there is something to
        } while (TASK_QUEUE_EMPTY(&ready_queue));

        // Set the blocked task to running again
        current_task = task;
        LOG("Exit sleep state");

        // Switch to the task that unblocked (unless the task that unblocked happens to be the
        // task we borrowed)
        task = task_queue_dequeue(&ready_queue);
        if (task != current_task) {
            switch_task(task);
        } else {
            // If no task switch is needed, set current task to a running state
            current_task->state = RUNNING;
        }
    }

    kassert(current_task != NULL);
}

/*
    Called within the timer interrupt using the timed event mechanism waking up at least one
    sleeping task.
*/
static void sleep_expiry_callback(uint64_t time_since_boot_ns, uint64_t timestamp_ns)
{
    // NOTE: No need to lock scheduler since function will be called within the timer ISR
    task_t            *task;
    struct list_entry *entry;

    (void)timestamp_ns;  // silence unused warning

    // Reset first wakeup flag
    scheduler_earliest_wakeup = UINT64_MAX;

    LIST_ITER_SAFE_REMOVAL(&sleep_queue.list, entry)
    {
        task = GET_STRUCT(task_t, task_queue_entry, entry);
        if (task->sleep_expiry <= time_since_boot_ns) {
            LOG("Wake-up task %x from sleep at %u", task, time_since_boot_ns);
            task_remove_from_current_task_queue(task);
            scheduler_unblock_task(task);
        } else {
            // Decrement first wakeup flag if necessary
            if (task->sleep_expiry < scheduler_earliest_wakeup) {
                scheduler_earliest_wakeup = task->sleep_expiry;
            }
        }
    }

    // Register new timed event if there are task left to be woken up
    if (scheduler_earliest_wakeup < UINT64_MAX) {
        timer_register_timed_event(scheduler_earliest_wakeup, sleep_expiry_callback);
    }
}

/*
    Called within the timer interrupt using the timed event mechanism check if it's time to preempt
    the currently running task
*/
static void preemption_callback(uint64_t time_since_boot_ns, uint64_t timestamp_ns)
{
    // NOTE: No need to lock scheduler since function will be called within the timer ISR
    uint64_t next_preemption_timestamp = time_since_boot_ns + TIME_SLICE_NS;

    (void)timestamp_ns;  // silence unused warning

    if (preemption_timestamp_ns != 0) {
        /* if preemption_timestamp is less than our time since boot than he callback has fired to
         * late, indicating something is wrong with the scheduler */
        kassert(preemption_timestamp_ns >= time_since_boot_ns);

        /* Should currently running task be preempted?  */
        if (preemption_timestamp_ns == time_since_boot_ns) {
            /*
                Mark the currently running task ready for preemption, allowing the interrupt system
                to perform preemption when it is safe to do so. This avoids bugs such as interrupt
                controllers not being properly ACK:ed due to a schedule call causing a task switch
            */
            current_task->status |= TASK_STATUS_PREEMPT;
        } else {
            /* No, adjust next_preemption timestamp so that the next callback fire when it's time */
            LOG("No need to preempt %x at %u", current_task, time_since_boot_ns);
            next_preemption_timestamp = preemption_timestamp_ns;
        }
    }

    /* Register a new callback at the appropriate time */
    timer_register_timed_event(next_preemption_timestamp, preemption_callback);
}

/* Tells the scheduler to but current task to sleep until the timestamp when */
void scheduler_nano_sleep_until(uint64_t when)
{
    uint32_t flags;
    critical_section_start(&flags);
    LOG("Put task %x to sleep until %u", current_task, when);

    // No need to sleep if when has already occurred
    if (when <= timer_get_time_since_boot()) {
        critical_section_end(flags);
        return;
    }

    current_task->sleep_expiry = when;

    // Add task to sleep queue
    task_queue_enqueue(&sleep_queue, current_task);

    // Adjust first wakeup if necessary
    if (when < scheduler_earliest_wakeup) {
        scheduler_earliest_wakeup = when;

        // Only register time event if this task needs to wakeup before the previous ones
        timer_register_timed_event(when, sleep_expiry_callback);
    }

    critical_section_end(flags);
    scheduler_block_task(SLEEPING);
}

/* Called by the interrupt handler allowing notifying the scheduler that the interrupt handler is
 * done executing allowing the scheduler to perform save preemption */
void scheduler_end_of_interrupt()
{
    // Preempt current task if necessary
    if (current_task != NULL && current_task->status & TASK_STATUS_PREEMPT) {
        // clear preemption flag
        current_task->status &= ~TASK_STATUS_PREEMPT;

        // If preempting an non-running task schedule might start sleeping which leads to unexpected
        // interrupt behaviors
        kassert(current_task->state == RUNNING);

        /* perform task switch */
        schedule();
    }

    if (current_task != NULL) {
        // clear interrupt flag, done after possible schedule() calls since once it gets running
        // again it will still not have called iret
        current_task->status &= ~TASK_STATUS_INTERRUPT;
    }
}

/* Called by the interrupt handler allowing notifying the scheduler that an interrupt has been
 * called */
void scheduler_start_of_interrupt()
{
    if (current_task != NULL) {
        current_task->status |= TASK_STATUS_INTERRUPT;
    }
}

/* Allows the currently running task to voluntarily stop execution */
void scheduler_yield()
{
    uint32_t flags;

    scheduler_lock(&flags);
    schedule();
    scheduler_unlock(flags);
}

/* Terminates the currently running task */
void scheduler_terminate_task()
{
    // Note: Can do any harmless stuff here (close files, free memory in user-space, ...) but
    // there's none of that yet

    uint32_t flags;
    critical_section_start(&flags);

    // Insert task in termination queue
    list_add_last(&termination_queue, &current_task->task_queue_entry);

    // block task, so that a task switch occur once the lock is released
    scheduler_block_task(TERMINATED);

    // make sure our task is not blocked
    scheduler_unblock_task(cleanup_task);

    LOG("Adding %x to termination queue", current_task);

    critical_section_end(flags);
}

static void cleanup_thread()
{
    uint32_t           flags;
    task_t            *task;
    struct list_entry *entry;

    // TODO: Re-write to handle refcount...

    while (true) {
        LOG("wakeup");
        critical_section_start(&flags);

        LIST_ITER_SAFE_REMOVAL(&termination_queue, entry)
        {
            task = GET_STRUCT(task_t, task_queue_entry, entry);

            kassert(task->state == TERMINATED && !task->current_task_queue);
            if (atomic_load(&task->ref_count) == 0) {
                LOG("Cleanup terminated task %x", task);

                list_entry_remove(&task->task_queue_entry);
                free_task(task);
            } else {
                LOG("Terminated thread still in use %x", task);
            }
        }

        if (!LIST_EMPTY(&termination_queue)) {
            // There tasks left to kill, hopefully they can be free'd the next time this thread
            // runs
            schedule();
        } else {
            // The work is done, but to sleep until there's more work to do
        scheduler_block_task(PAUSED);
        }

        critical_section_end(flags);
    }
}

void scheduler_init()
{
    current_task = create_root_task();
    if (current_task == NULL) {
        kpanic("Failed to allocate memory for initial task");
    }

    last_count              = timer_get_time_since_boot();
    preemption_timestamp_ns = timer_get_time_since_boot() + TIME_SLICE_NS;

    // Mark the scheduler as initialised
    scheduler_initialised = true;
    timer_register_timed_event(preemption_timestamp_ns, preemption_callback);

    LOG("Initialise scheduler (root proc: %x)", current_task);

    // Start task cleaning up terminated task
    cleanup_task = get_task(create_task(cleanup_thread));
}

/* Gets a pointer to the currently executing task */
task_t *scheduler_get_current_task()
{
    return current_task;
}
