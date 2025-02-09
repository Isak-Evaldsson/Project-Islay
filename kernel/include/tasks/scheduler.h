/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H
#include <arch/thread.h>
#include <fs.h>
#include <stddef.h>
#include <stdint.h>
#include <utils.h>

/* How long is the thread allowed to run before pre-emption */
#define TIME_SLICE_NS 50000000 /* 50 ms */

/* If set to 1, indicates that the task is to be preempted */
#define TASK_STATUS_PREEMPT (1 << 0)

/* If set to 1, indicates that the task is currently running an ISR */
#define TASK_STATUS_INTERRUPT (1 << 1)

/* The possible state a task can be in */
typedef enum {
    READY_TO_RUN,
    RUNNING,
    BLOCKED,
    SLEEPING,
    PAUSED,
    TERMINATED,
    WAITING_FOR_LOCK,
    WAITING_FOR_IO,
} task_state_t;

/*
    The task control block which stores all relevant data for each task (thread or process). This is
    the primary data structured used by the scheduler.
*/
typedef struct task task_t;

struct task {
    struct thread_regs regs;  // Architecture depended register used for task switching, at offset 0
                              // in order to allow arch specific asm to re-use task struct pointers.
    task_t *next;             // Nest pointer allows the scheduler to have task lists

    // kernel stack allocation information
    uintptr_t kstack_bottom;
    size_t    kstack_size;

    task_state_t state;         // The state of the task
    uint64_t     sleep_expiry;  // Until when shall the task sleep
    uint64_t     time_used;     // Allows us to have time statistics
    uint8_t      status;        // Task status flags

    // File system related data
    struct task_fs_data fs_data;
};

/* Asset offset to ensure asm compatiblity */
assert_offset(struct task, regs, 0);

/*
    Scheduler API
*/

/* Initialises the scheduler by setting up the inital boot process */
void scheduler_init();

/* Gets a pointer to the currently executing task */
task_t *scheduler_get_current_task();

/* Creates a new task executing the code at the address ip and sets it's state to ready-to-run */
task_t *scheduler_create_task(void *ip);

/* Block the currently running task */
void scheduler_block_task(unsigned int reason);

/* Unblock the specified ask */
void scheduler_unblock_task(task_t *task);

/* Tells the scheduler to but current task to sleep until the timestamp when */
void scheduler_nano_sleep_until(uint64_t when);

/* Allows the clock driver to invoke the scheduler to check if any sleeping task are allowed to run.
 */
void scheduler_timer_interrupt(uint64_t time_since_boot_ns, uint64_t period_ns);

/* Allows the currently running task to voluntarily stop execution */
void scheduler_yield();

/* Terminates the currently running task */
void scheduler_terminate_task();

#endif /* TASK_SCHEDULER_H */
