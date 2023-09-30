#include <arch/interrupt.h>
#include <arch/paging.h>
#include <arch/thread.h>
#include <devices/timer.h>
#include <klib/klib.h>
#include <kshell.h>
#include <memory/vmem_manager.h>
#include <tasks/scheduler.h>
#include <tasks/task_queue.h>

#define LOG_SCHEDULER 1

#if LOG_SCHEDULER
#define LOG(...) log("[SCHEDULER]: " __VA_ARGS__)
#else
#define LOG(...)
#endif

/*
    Scheduler state
*/

/*
   TODO:
   1. Go through we interrupts are enabled/disabled, replace the counter with restoring interrupt
     flags instead. See discussion in https://forum.osdev.org/viewtopic.php?f=1&t=56206
   2. Wrapper for all threads, ensuring proper locking before running actual function
   3. Double-check so that the time counting works correctly
   4. Task termination and mutexes
   5. Better algorithm than round robin
 */

// The ready-to-run task queue
static EMPTY_QUEUE(ready_queue);

// Sleep queue
static EMPTY_QUEUE(sleep_queue);

// Counter variables keeping track of the time consumption of each task
static uint64_t last_count    = 0;
static uint64_t current_count = 0;

// Counts how many ns the CPU has been idle
static uint64_t idle_time_ns = 0;

// Pointer to the currently running task
static task_t *current_task;

/* Stores at which timestamp the currently running task shall be preempted, if set to zero indicate
 * to never preempt */
static uint64_t preemption_timestamp_ns = 0;

// Counter ensuring that no thread what to disable interrupt when re-enabling them. Useful since the
// primary locking mechanism for a single core cpu is simply to disable interrupts.
static int IRQ_disable_counter = 0;

// Variables handling if the scheduler needs to postpone task switches. Necessary if you want to
// unblock multiple tasks within without running the risk of the first unblocked task preempting the
// task the unblocks the rest of the task.
unsigned int postpone_task_switch_counter = 0;  // If above 0 we're in a postpone state
bool         task_switch_postponed = false;     // Any tries to task switch during postpone state?

/* Variable storing the earliest wakeup time (in ns since boot) for any sleeping task. Allows
 * clock drivers check if it's necessary to call the 'scheduler_check_sleep_queue' */
uint64_t scheduler_earliest_wakeup = UINT64_MAX;

// The caller is responsible for appropriately locking/unlocking the scheduler when calling it
void schedule();

// Performs a context switch by switching the current running one to a new task
static void switch_task(task_t *new_task)
{
    task_t *old_task;

    // Done we need to postpone the task switch?
    if (postpone_task_switch_counter > 0) {
        task_switch_postponed = true;  // tell scheduler that we have postponed a task switch

        // Add our task first in queue, so it get's switched to once we're ready to call schedule
        // again
        LOG("Postponing switch to %x, added first in ready queue", new_task);
        task_queue_add_first(&ready_queue, new_task);
        return;
    }

    // TODO: Handle calls to switch task when in sleep mode, i.e. current_task == NULL
    kassert(current_task != NULL);
    // Re-add current task to list if it's still in running state
    if (current_task->state == RUNNING) {
        current_task->state = READY_TO_RUN;
        task_queue_enque(&ready_queue, current_task);
    }

    if (current_task == NULL) {
        // Task unblocked and stopped us being idle, so only one task can be running
        preemption_timestamp_ns = 0;
    } else if ((ready_queue.start == NULL) && (current_task->state != RUNNING)) {
        // Currently running task blocked and the task we're switching to is the only task left
        preemption_timestamp_ns = 0;
    } else {
        // More than one task wants the CPU, so set a time slice length
        preemption_timestamp_ns = timer_get_time_since_boot() + TIME_SLICE_NS;
    }

    // Switch state for our new task
    new_task->state = RUNNING;

    // Switch current task
    old_task     = current_task;
    current_task = new_task;

    LOG("Swich task from %x to %x, preemption timestamp %u", old_task, new_task,
        preemption_timestamp_ns);
    kernel_thread_switch(new_task->regs, old_task->regs);
}

/* Lock scheduler, blocks the task switching process from becoming disturbed by interrupts */
void scheduler_lock()
{
#ifndef SMP
    disable_interrupts();
    IRQ_disable_counter++;
#endif
}

/* Unlock scheduler, allows interrupts */
void scheduler_unlock()
{
#ifndef SMP
    IRQ_disable_counter--;

    // Only re-enable interrupts if all calls to disable them have called to re-enable
    if (IRQ_disable_counter == 0) {
        enable_interrupts();
    }
#endif
}

// Marks the start of a critical region, during it's execution we can't perform task switches nor
// interrupts
void lock_stuff()
{
#ifndef SMP
    scheduler_lock();
    postpone_task_switch_counter++;
#endif
}

// Marks the end of a critical region, during it's execution we can't perform task switches nor
// interrupts
void unlock_stuff()
{
#ifndef SMP
    postpone_task_switch_counter--;

    // Are there any more task that require task switching to be postponed
    if (postpone_task_switch_counter == 0) {
        // No, do have we have task switches waiting?
        if (task_switch_postponed) {
            task_switch_postponed = false;
            schedule();
        }
    }

    scheduler_unlock();
#endif
}

void scheduler_block_task(unsigned int reason)
{
    // TODO: replace with proper interrupt state/restore
    scheduler_lock();

    LOG("Block task %x, reason %u", current_task, reason);
    current_task->state = reason;
    schedule();

    scheduler_unlock();
}

void scheduler_unblock_task(task_t *task)
{
    // TODO: replace with proper interrupt state/restore
    scheduler_lock();

    // Never preempt, always append to end of queue
    LOG("Unblock: put %x in ready_queue", task);
    task_queue_enque(&ready_queue, task);

    // Ensure that the currently running thread is preempted, maybe do something smarter like having
    // a better time remaining system
    if (preemption_timestamp_ns == 0) {
        preemption_timestamp_ns = timer_get_time_since_boot() + TIME_SLICE_NS;
    }

    scheduler_unlock();
}

static void update_time_used()
{
    current_count    = timer_get_time_since_boot();
    uint64_t elapsed = current_count - last_count;
    last_count       = current_count;

    if (current_task == NULL) {
        idle_time_ns += elapsed;
    } else {
        current_task->time_used += elapsed;
    }
}

// Note, is the caller to schedule that is responsible for correct locking of the scheduler
void schedule()
{
    task_t *task;
    update_time_used();

    // Done we need to postpone the task switch?
    if (postpone_task_switch_counter > 0) {
        task_switch_postponed = true;  // tell scheduler that we have postponed a call to schedule()
        return;
    }

    // Are there any new tasks to schedule
    task = task_queue_dequeue(&ready_queue);
    if (task != NULL) {
        switch_task(task);  // Perform the context switch
    } else if (current_task->state == RUNNING) {
        // Otherwise, if the task is in running state, let it continue
    } else {
        // The current running task is block and there's no other task to be run
        task         = current_task;
        current_task = NULL;  // mark that CPU currently sleeps
        LOG("Enter sleep state");

        kassert(ready_queue.start == NULL);

        // Do nothing while waiting for a task to unblock and become "ready to run".  The only
        // thing that is going to update the ready queue is going to be from a timer IRQ
        // (with a single processor anyway), but interrupts are disabled. The timer must be
        // allowed to fire, but do not allow any task changes to take place.  The
        // task_switches_postponed_flag must remain set to force the timer to return to this
        // loop.

        do {
            enable_interrupts();   // enable interrupts to allow the timer to fire
            wait_for_interrupt();  // halt and wait for the timer to fire
            disable_interrupts();  // disable interrupts again to see if there is something to
        } while (ready_queue.start == NULL);

        // Set the blocked task to running again
        current_task = task;
        LOG("Exit sleep state");

        // Switch to the task that unblocked (unless the task that unblocked happens to be the
        // task we borrowed)
        task = task_queue_dequeue(&ready_queue);
        if (task != current_task) {
            switch_task(task);
        }
    }

    kassert(current_task != NULL);
}

/*
    Called within the timer interrupt using the timed event mechanism waking up at least one
    sleeping task.
*/
static void sleep_expiry_callback(uint64_t time_since_boot_ns, uint64_t timestamp_ns)
{
    // NOTE: No need to lock scheduler since function will be called within the timer ISR
    task_t *next;
    task_t *task;

    // Remove all task from sleep queue
    next              = sleep_queue.start;
    sleep_queue.start = NULL;
    sleep_queue.end   = NULL;

    // Reset first wakeup flag
    scheduler_earliest_wakeup = UINT64_MAX;

    while (next != NULL) {
        task       = next;
        next       = task->next;
        task->next = NULL;  // ensure that we don't accidentally insert multiple tasks in lists

        // Unblock task if succeficcent time has passed
        if (task->sleep_expiry <= time_since_boot_ns) {
            LOG("Wake-up task %x from sleep at %u", task, time_since_boot_ns);
            scheduler_unblock_task(task);
        } else {
            // Re-add into queue
            LOG("CHECK SLEEP QUEUE: put %x in sleep queue", task);
            task_queue_enque(&sleep_queue, task);

            // Decrement first wakeup flag if necessary
            if (task->sleep_expiry < scheduler_earliest_wakeup) {
                scheduler_earliest_wakeup = task->sleep_expiry;
            }
        }
    }

    // Register new timed event if there are task left to be woken up
    if (scheduler_earliest_wakeup < UINT64_MAX) {
        timer_register_timed_event(scheduler_earliest_wakeup, sleep_expiry_callback);
    }
}

/*
    Called within the timer interrupt using the timed event mechanism check if it's time to preempt
    the currently running task
*/
static void preemption_callback(uint64_t time_since_boot_ns, uint64_t timestamp_ns)
{
    // NOTE: No need to lock scheduler since function will be called within the timer ISR
    uint64_t next_preemption_timestamp = time_since_boot_ns + TIME_SLICE_NS;

    if (preemption_timestamp_ns != 0) {
        /* if preemption_timestamp is less than our time since boot than he callback has fired to
         * late, indicating something is wrong with the scheduler */
        kassert(!(preemption_timestamp_ns < time_since_boot_ns));

        /* Should currently running task be preempted?  */
        if (preemption_timestamp_ns == time_since_boot_ns) {
            /*
                Mark the currently running task ready for preemption, allowing the interrupt system
                to perform preemption when it is safe to do so. This avoids bugs such as interrupt
                controllers not being properly ACK:ed due to a schedule call causing a task switch
            */
            current_task->status |= TASK_STATUS_PREEMPT;
        } else {
            /* No, adjust next_preemption timestamp so that the next callback fire when it's time */
            LOG("No need to preempt %x at %u", current_task, time_since_boot_ns);
            next_preemption_timestamp = preemption_timestamp_ns;
        }
    }

    /* Register a new callback at the appropriate time */
    timer_register_timed_event(next_preemption_timestamp, preemption_callback);
}

/* Tells the scheduler to but current task to sleep until the timestamp when */
void scheduler_nano_sleep_until(uint64_t when)
{
    // TODO: replace with proper interrupt state/restore
    lock_stuff();
    LOG("Put task %x to sleep until %u", current_task, when);

    // No need to sleep if when has already occurred
    if (when <= timer_get_time_since_boot()) {
        scheduler_unlock();  // shouldn't it be unlock_stuff()?
        return;
    }

    current_task->sleep_expiry = when;

    // Add task to sleep queue
    task_queue_enque(&sleep_queue, current_task);

    // Adjust first wakeup if necessary
    if (when < scheduler_earliest_wakeup) {
        scheduler_earliest_wakeup = when;

        // Only register time event if this task needs to wakeup before the previous ones
        timer_register_timed_event(when, sleep_expiry_callback);
    }

    unlock_stuff();
    scheduler_block_task(SLEEPING);
}

/* Checks if the current task should be preempted and performs a task switch if necessary. Allows
 * the interrupt system to preempt tasks when it's safe to do so.  */
void scheduler_preempt_current_task()
{
    if (current_task != NULL && current_task->status & TASK_STATUS_PREEMPT) {
        // clear preemption flag
        current_task->status &= ~TASK_STATUS_PREEMPT;

        /*
            Enable interrupts since the switched task might not be within an interrupt handler soon
            to be calling iret.

            TODO: move to somewhere after the actual task switch to eliminate the risk of the task
            switch being terminated by another interrupt
        */
        enable_interrupts();

        /* before task switch */
        schedule();
    }
}

/* Creates a new task executing the code at the address ip and sets it's state to ready-to-run
 */
task_t *scheduler_create_task(void *ip)
{
    task_t *task = kmalloc(sizeof(task_t));
    if (task == NULL) {
        return NULL;
    }

    // Initialise fields
    task->next      = NULL;
    task->time_used = 0;
    task->state     = READY_TO_RUN;
    task->status    = 0;

    // Allocate stack
    task->kstack_bottom = vmem_request_free_page(0);
    task->kstack_size   = PAGE_SIZE;
    uintptr_t stack_top = task->kstack_bottom + task->kstack_size;

    // Setup thread registers
    task->regs = create_thread_regs_with_stack(stack_top, ip);

    // If thread registers could not be allocated, cleanup and abort task creation
    if (task->regs == NULL) {
        vmem_free_page(task->kstack_bottom);
        kfree(task);
        return NULL;
    }

    LOG("Create task: %x", task);

    task_queue_enque(&ready_queue, task);
    return task;
}

void sleeper()
{
    scheduler_unlock();
    int i = 0;

    for (;;) {
        log("Sleeper %u", ++i);
        sleep(1);
    }
}
void scheduler_init()
{
    current_task = kmalloc(sizeof(task_t));
    if (current_task == NULL) {
        kpanic("Failed to allocate memory for initial task");
    }

    current_task->regs = create_initial_thread_regs();
    if (current_task->regs == NULL) {
        kpanic("Failed to allocate memory for initial task thread registers");
    }

    current_task->next      = NULL;
    current_task->state     = RUNNING;
    current_task->status    = 0;
    last_count              = timer_get_time_since_boot();
    preemption_timestamp_ns = timer_get_time_since_boot() + TIME_SLICE_NS;

    timer_register_timed_event(preemption_timestamp_ns, preemption_callback);

    kprintf("Current %x\n", current_task);

    LOG("Initialise scheduler (root proc: %x)", current_task);

    //
    // Sleep tests
    //
    kprintf("Spawn new task\n");
    scheduler_create_task(&sleeper);

    kprintf("Back to main\n");
    for (int i = 0; i < 20; i++) {
        sleep(1);
    }
}