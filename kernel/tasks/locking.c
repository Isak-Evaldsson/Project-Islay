#include <arch/interrupt.h>
#include <klib/klib.h>
#include <tasks/locking.h>
#include <tasks/scheduler.h>
#include <tasks/task_queue.h>

#include "scheduler_internals.h"

#define LOG_LOCKING 1

#if LOG_LOCKING
#define LOG(...) log("[LOCKING]: " __VA_ARGS__)
#else
#define LOG(...)
#endif

/* Allocate and initialise a semaphore */
semaphore_t *semaphore_create(int max_count)
{
    semaphore_t *semaphore = kmalloc(sizeof(semaphore_t));
    if (semaphore != NULL) {
        *semaphore = (semaphore_t)SEMAPHORE_INIT(max_count);
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

    critical_section_start();

    if (semaphore->current_count < semaphore->max_count) {
        semaphore->current_count++;
        LOG("%x successfully acquired semaphore/mutex %x", current_task, semaphore);

    } else {
        LOG("%x failed to  acquired semaphore/mutex %x", current_task, semaphore);
        current_task->next = NULL;  // ensure we dont accidentally insert multiple tasks
        task_queue_enque(&semaphore->waiting_tasks, current_task);
        scheduler_block_task(WAITING_FOR_LOCK);
    }
    critical_section_end();
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

    critical_section_start();
    LOG("%x released semaphore/mutex", current_task);

    if (semaphore->waiting_tasks.start != NULL) {
        // pick the first waiting task
        task = task_queue_dequeue(&semaphore->waiting_tasks);
        scheduler_unblock_task(task);
    } else {
        // else indicate that we have a free resource
        semaphore->current_count--;
    }

    critical_section_end();
}

/* Allocate and initialise a mutex */
mutex_t *mutex_create()
{
    mutex_t *mutex = kmalloc(sizeof(mutex_t));
    if (mutex != NULL) {
        *mutex = (mutex_t)MUTEX_INIT();
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
