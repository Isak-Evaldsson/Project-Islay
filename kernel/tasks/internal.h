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

/* Marks the start of a critical section. All code between begin and end will run uninterrupted
 * by preemption or calls to schedule */
void critical_section_start(uint32_t *interrupt_flags);

/* Marks the end of a critical section. All code between begin and end will run uninterrupted
 * by preemption or calls to schedule */
void critical_section_end(uint32_t interrupt_flags);

// The caller is responsible for appropriately locking/unlocking the scheduler when calling it
void schedule();

#endif /*TASKS_INTERNAL_H */
