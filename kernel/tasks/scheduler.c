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
#include <tasks/spinlock.h>
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

/* Protects the termination queue */
static SPINLOCK_DEFINE(termination_lock);

// Counter variables keeping track of the time consumption of each task
static uint64_t last_count    = 0;
static uint64_t current_count = 0;

// Counts how many ns the CPU has been idle
static uint64_t idle_time_ns = 0;

// Pointer to the currently running task
task_t *current_task;

// The next task to run, set by the scheduler, but the context switch itself is
// handled by interrupt logic and is architecture dependent
task_t *next_task;

/* Stores at which timestamp the currently running task shall be preempted, if set to zero indicate
 * to never preempt */
static uint64_t preemption_timestamp_ns = 0;

// Variables handling if the scheduler needs to postpone task switches. Necessary if you want to
// unblock multiple tasks within without running the risk of the first unblocked task preempting the
// task the unblocks the rest of the task.

/*
 * Disables preemption if non-zero. Note, the process can still re-schedule itself;
 * so, the next time the process runs, the counter will be reset.
 */
unsigned int preemption_counter;

/* Variable storing the earliest wakeup time (in ns since boot) for any sleeping task. Allows
 * clock drivers check if it's necessary to call the 'scheduler_check_sleep_queue' */
uint64_t scheduler_earliest_wakeup = UINT64_MAX;

/* Protects sleep related data */
static SPINLOCK_DEFINE(sleep_lock);

/* Task responsible of freeing up space for task within the termination queue */
static task_t *cleanup_task = NULL;

/* Flag indicating if the scheduler has been initialised */
bool scheduler_initialised = false;

/* Perform voluntary task switch, the caller is responsible for correct locking */
static void schedule();

void scheduler_disable_preemption()
{
    unsigned int old;

    if (!scheduler_initialised) {
        return;
    }
    old = preemption_counter++;
    kassert(preemption_counter > old);
}

void scheduler_enable_preemption()
{
    unsigned int old;

    if (!scheduler_initialised) {
        return;
    }
    old = preemption_counter--;
    kassert(preemption_counter < old);
}

/* Lock scheduler, blocks the task switching process from becoming disturbed by interrupts */
void scheduler_lock(uint32_t *interrupt_flags)
{
#ifndef SMP
    // Disable interrupts and save the previous state
    *interrupt_flags = get_register_and_disable_interrupts();
    scheduler_disable_preemption();
#endif
}

/* Unlock scheduler, allows interrupts */
void scheduler_unlock(uint32_t interrupt_flags)
{
#ifndef SMP
    // Restore interrupt flags before locking
    restore_interrupt_register(interrupt_flags);
    scheduler_enable_preemption();
#endif
}

static void mark_task_blocked(block_reason_t reason)
{
    uint32_t flags;

    scheduler_lock(&flags);
    LOG("Block task %x, reason %u", current_task, reason);
    current_task->state        = BLOCKED;
    current_task->block_reason = reason;
    scheduler_unlock(flags);
}

void scheduler_block_task(block_reason_t reason)
{
    mark_task_blocked(reason);
    schedule();
}

void scheduler_unblock_task(task_t *task)
{
    uint32_t flags;
    scheduler_lock(&flags);
    LOG("Unblock task %x", task);

    if (task->state == BLOCKED || task->state == BLOCKED_IDLING) {
        if (task == current_task) {
            LOG("Unblock current, rescheduling it");
            kassert(MARK_FOR_RESCHEDULE(task));
            task->state = RUNNING;
        } else {
            LOG("put %x in ready_queue", task);
        task->state = READY_TO_RUN;
        task_queue_enqueue(&ready_queue, task);

            // Make sure the current_task gets preempted
            if (!preemption_timestamp_ns) {
        preemption_timestamp_ns = timer_get_time_since_boot() + TIME_SLICE_NS;
            }
        }
    }
    scheduler_unlock(flags);
}

static void update_time_used()
{
    current_count    = timer_get_time_since_boot();
    uint64_t elapsed = current_count - last_count;
    last_count       = current_count;

    if (current_task->status == BLOCKED_IDLING) {
        idle_time_ns += elapsed;
    } else {
        current_task->time_used += elapsed;
    }
}

static void schedule()
{
    struct task *task = current_task;

    // Schedule should never be called within an interrupt or when preemption
    // is disabled
    kassert(!(task->status & TASK_STATUS_INTERRUPT));

    if (preemption_counter) {
        LOG("%x voluntarily re-schedules while preemption is disabled %x", task);
    }

    /*
     * Since context switches occurs within interrupts we simply set the
     * rescheduling status flags and waits for the next interrupt to do the work.
     * Since there might not be any other task to schedule, this thread might
     * need to act as the idle thread, hence the loop.
     */
    MARK_FOR_RESCHEDULE(task);
    do {
        kassert(interrupts_enabled());
        wait_for_interrupt();
    } while (WAITING_FOR_RESCHEDULE(task));
}

/*
 * The core of the scheduler, set the next_task to be executed at the next
 * context switch.
 *
 * Note, is up to the caller to ensure that function is called in thread safe manner
 */
void do_schedule()
{
    struct task *task;

    update_time_used();
    kassert(WAITING_FOR_RESCHEDULE(current_task));

    // If a process voluntary re-schedules itself, it implicitly tells the system
    // that it's no longer needs to be run in a non-preemption context
    if (preemption_counter) {
        LOG("Reseting preemption counter");
        preemption_counter = 0;
    }

    task = task_queue_dequeue(&ready_queue);
    if (!task) {
        // No need to preempt when there's no other tasks asking for cpu time
        preemption_timestamp_ns = 0;

        if (current_task->state == RUNNING) {
            LOG("No new task in queue, let the task continue");
            current_task->status &= ~TASK_STATUS_RESCHEDULE;
        } else {
            LOG("No new task in queue, but current is blocked, idle");
            current_task->state = BLOCKED_IDLING;
        }
        return;
    }
    // current_task cannot be queued to run while running, there must be a scheduler bug
    kassert(task != current_task);

    LOG("Re-schedule to %x", task);
    next_task        = task;
    next_task->state = RUNNING;
    current_task->status &= ~TASK_STATUS_RESCHEDULE;

    // Requeue current_task if possible
    if (current_task->state == RUNNING) {
        current_task->state = READY_TO_RUN;
        task_queue_enqueue(&ready_queue, current_task);
    }

    preemption_timestamp_ns =
        !TASK_QUEUE_EMPTY(&ready_queue) ? timer_get_time_since_boot() + TIME_SLICE_NS : 0;
}

/*
    Called within the timer interrupt using the timed event mechanism waking up at least one
    sleeping task.
*/
static void sleep_expiry_callback(uint64_t time_since_boot_ns, uint64_t timestamp_ns)
{
    // NOTE: No need to lock scheduler since function will be called within the timer ISR

    uint32_t           flags;
    task_t            *task;
    struct list_entry *entry;

    (void)timestamp_ns;  // silence unused warning
    spinlock_lock(&sleep_lock, &flags);

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
    spinlock_unlock(&sleep_lock, flags);
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
               Mark the currently running task ready for rescheduling, allowing the interrupt
               system to reschedule when it is safe to do so.
            */
            if (!preemption_counter) {
            MARK_FOR_RESCHEDULE(current_task);
            } else {
                LOG("Preemption disabled, skip rescheduling");
            }
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
    LOG("Put task %x to sleep until %u", current_task, when);

    // No need to sleep if when has already occurred
    if (when <= timer_get_time_since_boot()) {
        return;
    }

    spinlock_lock(&sleep_lock, &flags);
    current_task->sleep_expiry = when;

    // Add task to sleep queue
    task_queue_enqueue(&sleep_queue, current_task);

    // Adjust first wakeup if necessary
    if (when < scheduler_earliest_wakeup) {
        scheduler_earliest_wakeup = when;

        // Only register time event if this task needs to wakeup before the previous ones
        timer_register_timed_event(when, sleep_expiry_callback);
    }

    // Must be within a non-irq context to prevent the sleep expiry callback
    // from firing, potential trying to unblock the task before blocking it,
    // if we block afterwards, it will never wake up
    mark_task_blocked(BLOCK_REASON_SLEEP);
    spinlock_unlock(&sleep_lock, flags);
    schedule();
}

/* Called by the interrupt handler allowing notifying the scheduler that the interrupt handler is
 * done executing allowing the scheduler to perform save preemption */
void scheduler_end_of_interrupt()
{
    if (current_task != NULL) {
        if (WAITING_FOR_RESCHEDULE(current_task)) {
            do_schedule();
        }
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
    // Insert task in termination queue

    spinlock_lock(&termination_lock, &flags);
    list_add_last(&termination_queue, &current_task->task_queue_entry);
    spinlock_unlock(&termination_lock, flags);

    // make sure our task is not blocked
    scheduler_unblock_task(cleanup_task);

    LOG("Adding %x to termination queue", current_task);
    current_task->state = TERMINATED;
    schedule();
}

static void cleanup_thread()
{
    uint32_t           flags;
    task_t            *task;
    struct list_entry *entry;
    bool               empty;

    // TODO: Re-write to handle refcount...

    while (true) {
        LOG("wakeup");
        spinlock_lock(&termination_lock, &flags);
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
        empty = LIST_EMPTY(&termination_queue);
        spinlock_unlock(&termination_lock, flags);

        if (!empty) {
            // There tasks left to kill, hopefully they can be free'd the next time this thread
            // runs
            scheduler_yield();
        } else {
            // The work is done, but to sleep until there's more work to do
            scheduler_block_task(BLOCK_REASON_PAUSED);
        }
    }
}

void scheduler_init()
{
    /*
     * Can't take the scheduler lock since scheduler_initialised is set mid function,
     * this will break the preemption logic, since the preemption_counter will never be
     * able to reach one. Disabling interrupts should suffice.
     */
    uint32_t flags = get_register_and_disable_interrupts();

    current_task = create_root_task();
    if (current_task == NULL) {
        kpanic("Failed to allocate memory for initial task");
    }

    next_task               = current_task;
    last_count              = timer_get_time_since_boot();
    preemption_timestamp_ns = timer_get_time_since_boot() + TIME_SLICE_NS;

    // Mark the scheduler as initialised
    scheduler_initialised = true;
    timer_register_timed_event(preemption_timestamp_ns, preemption_callback);

    LOG("Initialise scheduler (root proc: %x)", current_task);

    // Start task cleaning up terminated task
    cleanup_task = get_task(create_task(cleanup_thread));

    restore_interrupt_register(flags);
}

/* Gets a pointer to the currently executing task */
task_t *scheduler_get_current_task()
{
    return current_task;
}
