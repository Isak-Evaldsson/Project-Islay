/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/interrupts.h>
#include <tasks/locking.h>
#include <tasks/scheduler.h>
#include <tasks/task_queue.h>
#include <utils.h>

#include "internal.h"

#define LOG_LOCKING 1

#define LOG(fmt, ...) __LOG(LOG_LOCKING, "[LOCKING]", fmt, ##__VA_ARGS__)

/* Allocate and initialise a semaphore */
semaphore_t *semaphore_create(int count)
{
    semaphore_t *semaphore = kalloc(sizeof(semaphore_t));
    if (semaphore != NULL) {
        *semaphore = (semaphore_t)SEMAPHORE_INIT(*semaphore, count);
    }

    return semaphore;
}

static void check_non_interrupt(void *ptr, const char *name)
{
    if (current_task->status & TASK_STATUS_INTERRUPT) {
        kpanic("Thread %x is trying to acquire %s %x within an interrupt", current_task, name, ptr);
    }
}

static void __semaphore_signal(semaphore_t *semaphore)
{
    task_t *task;
    atomic_add_fetch(&semaphore->count, 1);

    // pick the first waiting task if available
    scheduler_disable_preemption();
    task = task_queue_dequeue(&semaphore->waiting_tasks);
    if (task) {
        scheduler_unblock_task(task);
    }
    scheduler_enable_preemption();
}

static void __semaphore_wait(semaphore_t *semaphore)
{
    int current;

    current = atomic_load(&semaphore->count);
    do {
        while (!current) {
            LOG("%x failed to acquire semaphore/mutex %x", current_task, semaphore);
            scheduler_disable_preemption();
            task_queue_enqueue(&semaphore->waiting_tasks, current_task);
            scheduler_block_task(BLOCK_REASON_LOCK_WAIT);  // Implicitly enables preemption
            current = atomic_load(&semaphore->count);
        }
    } while (!atomic_compare_exchange(&semaphore->count, &current, current - 1));
    LOG("%x successfully acquired semaphore/mutex %x", current_task, semaphore);
}

/* Increments the semaphore and wakes up potential waiters */
void semaphore_signal(semaphore_t *semaphore)
{
    check_non_interrupt(semaphore, "semaphore");
    __semaphore_signal(semaphore);
}

/* Decrements the semaphore if possible, otherwise wait */
void semaphore_wait(semaphore_t *semaphore)
{
    check_non_interrupt(semaphore, "semaphore");
    __semaphore_wait(semaphore);
}

/* Allocate and initialise a mutex */
mutex_t *mutex_create()
{
    mutex_t *mutex = kalloc(sizeof(mutex_t));
    if (mutex != NULL) {
        *mutex = (mutex_t)MUTEX_INIT(*mutex);
    }

    return mutex;
}

/* Lock mutex */
void mutex_lock(mutex_t *mutex)
{
    kassert(scheduler_initialised);
    check_non_interrupt(mutex, "mutex");
    __semaphore_wait(&mutex->sem);
}

/* Unlock mutex */
void mutex_unlock(mutex_t *mutex)
{
    kassert(scheduler_initialised);
    check_non_interrupt(mutex, "mutex");
    __semaphore_signal(&mutex->sem);
}

/* Lock spinlock */
void spinlock_lock(struct spinlock *spinlock, uint32_t *irqflags)
{
#ifndef SMP
    LOG("lock %x", spinlock);
    *irqflags = get_register_and_disable_interrupts();
    scheduler_disable_preemption();

    /*
        In UMP, there's not parallel threads that can try to take the flag,
        so if it's already taken, there's bug where a thread tries to acquire
        the same lock twice.
    */
    kassert(!spinlock->flag);
    spinlock->flag++;
#else
#error Spinlock not defined for SMP
#endif
}
/* Unlock spinlock */
void spinlock_unlock(struct spinlock *spinlock, uint32_t irqflags)
{
#ifndef SMP
    LOG("unlock %x", spinlock);
    spinlock->flag--;
    restore_interrupt_register(irqflags);
    scheduler_enable_preemption();
#else
#error Spinlock not defined for SMP
#endif
}
