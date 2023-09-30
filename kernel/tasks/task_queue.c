#include <klib/klib.h>
#include <stdint.h>
#include <tasks/task_queue.h>

#include "scheduler.h"

void task_queue_enque(task_queue_t* queue, task_t* task)
{
    // check that we're not accidentally put more than a single task into the list
    kassert(task->next == NULL);

    // Update start if queue is empty
    if (queue->start == NULL) {
        queue->start = task;
    } else {
        // Link previous ends next pointer if queue is non-empty
        queue->end->next = task;
    }

    // Add task to end of queue
    queue->end = task;
}

void task_queue_add_first(task_queue_t* queue, task_t* task)
{
    // check that we're not accidentally put more than a single task into the list
    kassert(task->next == NULL);

    // Set our new task to start of queue
    task->next   = queue->start;
    queue->start = task;

    // If queue contains a single entry set end as well
    if (queue->start->next == NULL) {
        queue->end = queue->start;
    }
}

task_t* task_queue_dequeue(task_queue_t* queue)
{
    task_t* task = queue->start;  // get first element of the queue

    // Update start pointer if queue is not empty
    if (task != NULL) {
        queue->start = queue->start->next;

        // Check if queue now have become empty
        if (queue->start == NULL) {
            queue->end = NULL;
        }

        task->next = NULL;
    }

    return task;
}
