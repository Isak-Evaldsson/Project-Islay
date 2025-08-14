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
semaphore_t *semaphore_create(int max_count)
{
    semaphore_t *semaphore = kalloc(sizeof(semaphore_t));
    if (semaphore != NULL) {
        *semaphore = (semaphore_t)SEMAPHORE_INIT(*semaphore, max_count);
    }

    return semaphore;
}

/* Acquire semaphore */
void semaphore_acquire(semaphore_t *semaphore)
{
    // Before scheduler is initialised we can grantee mutual exclusion by disable interrupts since
    // we're running in a single threaded context
    if (!scheduler_initialised) {
        disable_interrupts();
        return;
    }

    if (current_task->status & TASK_STATUS_INTERRUPT) {
        kpanic("Thread %x is trying to acquire semaphore/mutex %x within an interrupt",
               current_task, semaphore);
    }

    critical_section_start(&semaphore->interrupt_flags);

    if (semaphore->current_count < semaphore->max_count) {
        semaphore->current_count++;
        LOG("%x successfully acquired semaphore/mutex %x", current_task, semaphore);

    } else {
        LOG("%x failed to  acquired semaphore/mutex %x", current_task, semaphore);
        task_queue_enqueue(&semaphore->waiting_tasks, current_task);
        scheduler_block_task(WAITING_FOR_LOCK);
    }
    critical_section_end(semaphore->interrupt_flags);
}

/* Release semaphore */
void semaphore_release(semaphore_t *semaphore)
{
    task_t *task;

    // Before scheduler is initialised we can grantee mutual exclusion by disable interrupts since
    // we're running in a single threaded context
    if (!scheduler_initialised) {
        enable_interrupts();
        return;
    }

    if (current_task->status & TASK_STATUS_INTERRUPT) {
        kpanic("Thread %x is trying to release semaphore/mutex %x within an interrupt",
               current_task, semaphore);
    }

    critical_section_start(&semaphore->interrupt_flags);
    LOG("%x released semaphore/mutex %x", current_task, semaphore);

    if (!TASK_QUEUE_EMPTY(&semaphore->waiting_tasks)) {
        // pick the first waiting task
        task = task_queue_dequeue(&semaphore->waiting_tasks);
        scheduler_unblock_task(task);
    } else {
        // else indicate that we have a free resource
        semaphore->current_count--;
    }

    critical_section_end(semaphore->interrupt_flags);
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
    semaphore_acquire(&mutex->sem);
}

/* Unlock mutex */
void mutex_unlock(mutex_t *mutex)
{
    semaphore_release(&mutex->sem);
}

/* Lock spinlock */
void spinlock_lock(struct spinlock *spinlock, uint32_t *irqflags)
{
#ifndef SMP
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
    spinlock->flag--;
    restore_interrupt_register(irqflags);
    scheduler_enable_preemption();
#else
#error Spinlock not defined for SMP
#endif
}
