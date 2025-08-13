/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#ifndef TASK_TASK_H
#define TASK_TASK_H
#include <arch/thread.h>
#include <atomics.h>
#include <fs.h>
#include <list.h>
#include <stddef.h>
#include <stdint.h>
#include <utils.h>

/* If set to 1, indicates that the task is to be preempted */
#define TASK_STATUS_PREEMPT (1 << 0)

/* If set to 1, indicates that the task is currently running an ISR */
#define TASK_STATUS_INTERRUPT (1 << 1)

/* The possible state a task can be in */
typedef enum {
    READY_TO_RUN,
    RUNNING,
    BLOCKED,
    TERMINATED,
} task_state_t;

/* Reason for a task to be in a blocked state */
typedef enum {
    BLOCK_REASON_SLEEP,
    BLOCK_REASON_PAUSED,
    BLOCK_REASON_LOCK_WAIT,
    BLOCK_REASON_IO_WAIT,
    BLOCK_REASON_MAX,
} block_reason_t;

// Temporary solution? I see two options 1: Merge headers, 2: Forward reference
typedef struct task_queue task_queue_t;

/* Number identify an existing task */
typedef unsigned int tid_t;

/*
    The task control block which stores all relevant data for each task (thread or process). This is
    the primary data structured used by the scheduler.
*/
typedef struct task task_t;

struct task {
    struct thread_regs regs;  // Architecture depended register used for task switching, at offset 0
                              // in order to allow arch specific asm to re-use task struct pointers.
    tid_t             tid;    // Unique identifier for each task
    struct list_entry task_queue_entry;    // Allows the task to be in queue
    task_queue_t*     current_task_queue;  // What task_queue does the task currently belongs to

    atomic_uint_t     ref_count;  // To ensure that the task isn't killed when the object is in use
    struct list_entry task_list_entry;  // Global list of all tasks

    // kernel stack allocation information
    uintptr_t kstack_bottom;
    size_t    kstack_size;

    task_state_t   state;         // The state of the task
    block_reason_t block_reason;  // Why the task is in a blocked state
    uint64_t       sleep_expiry;  // Until when shall the task sleep
    uint64_t       time_used;     // Allows us to have time statistics
    uint8_t        status;        // Task status flags

    // File system related data
    struct task_fs_data fs_data;
};

/* Asset offset to ensure asm compatiblity */
assert_offset(struct task, regs, 0);

/* Creates a new task executing the code at the address ip */
tid_t create_task(void* ip);

/* Gives task control block associated to the supplied tid. Needs to call put_task() when do to
 * allow it to be properly cleaned up on termination */
task_t* get_task(tid_t tid);

/* Mark the supplied task control block as no longer used */
void put_task(task_t* task);

#endif /* TASK_TASK_H */
