/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <stdint.h>
#include <tasks/scheduler.h>
#include <tasks/task_queue.h>
#include <utils.h>

/* The common operations whenever inserting a task into a task_queue */
static void prepare_insert(task_queue_t* queue, task_t* task)
{
    kassert(task->state != TERMINATED);         // We should never try to enqueue dead tasks
    kassert(task->current_task_queue == NULL);  // A task can only belong to a single task queue

    // By incrementing the refcount, the cleanup thread is prevented from free'ing the task object
    // while it's in a queue
    atomic_add_fetch(&task->ref_count, 1);
    task->current_task_queue = queue;
}

void task_queue_enqueue(task_queue_t* queue, task_t* task)
{
    uint32_t flags;
    spinlock_lock(&queue->lock, &flags);

    prepare_insert(queue, task);
    list_add_last(&queue->list, &task->task_queue_entry);
    spinlock_unlock(&queue->lock, flags);
}

void task_queue_add_first(task_queue_t* queue, task_t* task)
{
    uint32_t flags;
    spinlock_lock(&queue->lock, &flags);

    prepare_insert(queue, task);
    list_add_first(&queue->list, &task->task_queue_entry);
    spinlock_unlock(&queue->lock, flags);
}

task_t* task_queue_dequeue(task_queue_t* queue)
{
    uint32_t flags;
    spinlock_lock(&queue->lock, &flags);
    struct list_entry* entry = list_remove_first(&queue->list);
    if (!entry) {
        spinlock_unlock(&queue->lock, flags);
        return NULL;
    }
    task_t* task             = GET_STRUCT(task_t, task_queue_entry, entry);
    task->current_task_queue = NULL;
    spinlock_unlock(&queue->lock, flags);
    put_task(task);  // Allow task to be free'ed
    return task;
}

void task_remove_from_current_task_queue(task_t* task)
{
    uint32_t      flags;
    task_queue_t* queue = task->current_task_queue;
    kassert(queue != NULL);

    spinlock_lock(&queue->lock, &flags);
    task->current_task_queue = NULL;
    list_entry_remove(&task->task_queue_entry);
    spinlock_unlock(&queue->lock, flags);
    put_task(task);
}
