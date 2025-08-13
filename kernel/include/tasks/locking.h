/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef TASK_LOCKING_H
#define TASK_LOCKING_H
#include <tasks/task_queue.h>

/* Defines an empty semaphore struct */
#define SEMAPHORE_INIT(name, count) \
    {.current_count = 0, .max_count = (count), .waiting_tasks = QUEUE_INIT((name).waiting_tasks)}

#define MUTEX_INIT(name) {.sem = SEMAPHORE_INIT((name).sem, 1)}

/* Initialise a staticly allocated semaphore */
#define SEMAPHORE_DEFINE(name, count) semaphore_t name = SEMAPHORE_INIT(name, count)

/* Initialise a staticly allocated mutex */
#define MUTEX_DEFINE(name) mutex_t name = MUTEX_INIT(name)

/* Semaphore lock */
typedef struct semaphore semaphore_t;

struct semaphore {
    int          max_count;
    int          current_count;
    uint32_t     interrupt_flags;
    task_queue_t waiting_tasks;
};

/* Mutex lock, like a semaphore of size 1 but with a nicer interface */
typedef struct mutex mutex_t;

struct mutex {
    semaphore_t sem;  // internal implemented as a semaphore of count 1
};

/* Allocate and initialise a semaphore */
semaphore_t *semaphore_create(int max_count);

/* Acquire semaphore */
void semaphore_acquire(semaphore_t *semaphore);

/* Release semaphore */
void semaphore_release(semaphore_t *semaphore);

/* Allocate and initialise a mutex */
mutex_t *mutex_create();

/* Lock mutex */
void mutex_lock(mutex_t *mutex);

/* Unlock mutex */
void mutex_unlock(mutex_t *mutex);

#endif /* TASK_LOCKING_H */
