/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef TASK_TASK_QUEUE_H
#define TASK_TASK_QUEUE_H
#include <tasks/scheduler.h>

// Queue object storing the start and and end of a linked list of task
typedef struct task_queue {
    task_t* start;
    task_t* end;
} task_queue_t;

#define QUEUE_INIT() {.start = NULL, .end = NULL}

// Initialise an empty task queue
#define EMPTY_QUEUE(name) task_queue_t name = QUEUE_INIT()

// Adds a task to the end of the queue
void task_queue_enque(task_queue_t* queue, task_t* task);

// Adds task to the front of queue
void task_queue_add_first(task_queue_t* queue, task_t* task);

// Removes the first task form the queue
task_t* task_queue_dequeue(task_queue_t* queue);

#endif /* TASK_TASK_QUEUE_H */
