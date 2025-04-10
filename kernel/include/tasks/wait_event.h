#include <list.h>
#include <tasks/scheduler.h>

/*
    Give more of a wait_queue name

    Do we every need more than one wait event?

    Context mutex: No, a single task can hold multiple locks, but only wait at one at the time
    Context sleep: No, a single task can only sleep once, since every time you call sleep it stops
                   execution, so there can't be a buildup of new events

    Context block cache: If reading from two pages, and both are not present, multiple read request
                         will be sent to the disk.
                         could be solve by having the wait_queue_wakeup() evaluate every time the
                         new block is found, but then check for all block before starting anything.

    Context tty: No, a task can only read from one tty at once, hence only a single tty needs to be
                 waited at.
*/
enum wait_action {
    WAIT_ACTION_NONE,
    WAIT_ACTION_CB,
}

/* Function determining if the task should wakeup, true if that's the case */
typedef bool (*wait_event_wakeup_cb)(task_t*, void*);

struct wait_event {
    // TODO: spinlock
    uint8_t              action;
    wait_event_wakeup_cb wakeup_cb;
    struct task_queue    waiters;
};

void wait_event_init(struct wait_event* event, uint8_t action, wait_event_wakeup_cb cb);
void wait_event_wait(struct wait_event* event);
void wait_event_wakeup(struct wait_event* event, void* arg);
void wait_event_wakeup_all(struct wait_event* event, void* arg);
