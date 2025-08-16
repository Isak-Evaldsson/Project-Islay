/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef TASK_TASK_QUEUE_H
#define TASK_TASK_QUEUE_H
#include <list.h>
#include <tasks/spinlock.h>
#include <tasks/task.h>

/* A generic task queue, with an API to ensure that task are safely queued. To simply scheduling, a
 * task can only be in one queue at the time. */
typedef struct task_queue {
    struct spinlock lock;
    struct list     list;
} task_queue_t;

#define QUEUE_INIT(name) {.lock = SPINLOCK_INIT(), .list = LIST_INIT((name).list)}

/* Initialise an empty task queue */
#define EMPTY_QUEUE(name) task_queue_t name = QUEUE_INIT(name)

/* Check if the task queue is empty */
#define TASK_QUEUE_EMPTY(queue_ptr) LIST_EMPTY(&(queue_ptr)->list)

/* Adds a task to the end of the queue */
void task_queue_enqueue(task_queue_t* queue, task_t* task);

/* Adds task to the front of queue */
void task_queue_add_first(task_queue_t* queue, task_t* task);

/* Removes the first task form the queue */
task_t* task_queue_dequeue(task_queue_t* queue);

/* Removes a task for it's current task queue*/
void task_remove_from_current_task_queue(task_t* task);

#endif /* TASK_TASK_QUEUE_H */
