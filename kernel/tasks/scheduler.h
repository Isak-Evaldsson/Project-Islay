#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H
#include <arch/thread.h>
#include <stddef.h>
#include <stdint.h>

// How long is the thread allowed to run before pre-emption
#define TIME_SLICE_NS 50000000  // 50 ms

/* The possible state a task can be in */
typedef enum {
    READY_TO_RUN,
    RUNNING,
    BLOCKED,
    SLEEPING,
} task_state_t;

/*
    The task control block which stores all relevant data for each task (thread or process). This is
    the primary data structured used by the scheduler.
*/
typedef struct task task_t;

struct task {
    thread_regs_t *regs;  // Architecture depended register used for task switching
    task_t        *next;  // Nest pointer allows the scheduler to have task lists

    // kernel stack allocation information
    uintptr_t kstack_bottom;
    size_t    kstack_size;

    task_state_t state;         // The state of the task
    uint64_t     sleep_expiry;  // Until when shall the task sleep
    uint64_t     time_used;     // Allows us to have time statistics
};

/*
    Scheduler API
*/

/* Initialises the scheduler by setting up the inital boot process */
void scheduler_init();

/* Creates a new task executing the code at the address ip and sets it's state to ready-to-run */
task_t *scheduler_create_task(void *ip);

/* Block the currently running task */
void scheduler_block_task(unsigned int reason);

/* Unblock the specified ask */
void scheduler_unblock_task(task_t *task);

/* Tells the scheduler to but current task to sleep until the timestamp when */
void scheduler_nano_sleep_until(uint64_t when);

/* Allows the clock driver to invoke the scheduler to check if any sleeping task are allowed to run.
 */
void scheduler_check_sleep_queue(uint64_t time_since_boot);

/* Stores how long time the currently runing task has left before being preempted */
extern uint64_t time_slice_remaning;

/* Varible storing the earliest wakeup time (in ns since boot) for any sleeping task. Allows
 * clock drivers check if it's necessary to call the 'scheduler_check_sleep_queue' */
extern uint64_t scheduler_earliest_wakeup;

#endif /* TASK_SCHEDULER_H */