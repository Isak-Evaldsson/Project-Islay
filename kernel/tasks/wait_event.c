#include <tasks/wait_event.h>
#include <uapi/errno.h>

#include "internal.h"

void wait_event_wakeup_tasks(struct wait_event* event, void* arg)
{
    /*
        For each waiter in event->waiters:
            1. Get task struct
            2. Perform action:
                NONE - Do nothing
                CB - if true wakeup, otherwise we want to continue waiting (skip step 3 and 4)
            3. Remove waiter from list in TCB
            4. If last in list:
                unblock task...
    */
}

int wait_event_task_wait(struct wait_event* event)
{
    struct wait_event waiter = {};  // Doesn't work, can't be stack allocated

    return __wait_event_task_wait(event, &waiter);
}

int wait_event_task_wait_cb(struct wait_event* event, bool (*cb)(void*))
{
    struct wait_event waiter = {};

    return __wait_event_task_wait(event, &waiter);
}

static int __wait_event_task_wait(struct wait_event* event, struct wait_event* waiter)
{
    // Do some waiter struct validation...
    uint32_t                  flags;
    struct wait_event_waiter* waiter;  // TODO: How to allocate?

    // Input validation
    switch (action) {
        case WAIT_ACTION_NONE:
            if (!cb) {
                return -EINVAL;
            }
            waiter->cb = cb;
            break;

        case WAIT_ACTION_CB:
            waiter->cb = NULL;
            break;

        default:
            return -EINVAL;
    }

    waiter->task   = current_task->tid;
    waiter->action = action;

    critical_section_start(&flags);
    list_add(&event->waiters, &waiter->entry);

    // TODO: Mark the waiting in our TCB...

    // TODO: Custom reason?
    scheduler_block_task(BLOCKED);

    critical_section_end(flags);
}