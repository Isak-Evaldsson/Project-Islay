/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef TASKS_INTERNAL_H
#define TASKS_INTERNAL_H
#include <stdbool.h>
#include <tasks/scheduler.h>

/* Pointer to the currently running task */
extern task_t *current_task;

/* Flag indicating if the scheduler has been initialised. While false the kernel can assume to run
 * on single threaded */
extern bool scheduler_initialised;

/* Frees the memory of the task object */
void free_task(task_t *task);

/* Function handling creation of the root task */
task_t *create_root_task();

/* Disables preemption within the scheduler */
void scheduler_disable_preemption();

/* Enabled preemption within the scheduler */
void scheduler_enable_preemption();

// The caller is responsible for appropriately locking/unlocking the scheduler when calling it
void schedule();

/* Called by the interrupt handler allowing notifying the scheduler that an interrupt has been
 * called */
void scheduler_start_of_interrupt();

/* Called by the interrupt handler allowing notifying the scheduler that the interrupt handler is
 * done executing allowing the scheduler to perform save preemption */
void scheduler_end_of_interrupt();

#endif /* TASKS_INTERNAL_H */
