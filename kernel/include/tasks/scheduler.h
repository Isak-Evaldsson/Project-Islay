/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H
#include <tasks/task.h>

/* How long is the thread allowed to run before pre-emption */
#define TIME_SLICE_NS 50000000 /* 50 ms */

/* Initialises the scheduler by setting up the inital boot process */
void scheduler_init();

/* Gets a pointer to the currently executing task */
task_t *scheduler_get_current_task();

/* Block the currently running task */
void scheduler_block_task(block_reason_t reason);

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
