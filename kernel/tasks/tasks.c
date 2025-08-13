/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#include <arch/paging.h>
#include <memory/vmem_manager.h>
#include <tasks/scheduler.h>

/* Global list of all tasks */
static DEFINE_LIST(task_list);

static tid_t alloc_tid()
{
    static tid_t last_tid = 0;

    tid_t prev = last_tid;
    tid_t new  = ++last_tid;

    if (new < prev) {
        kpanic("Can't create new task, out of tids");
    }

    kassert(new > 0);
    return new;
}

/* Wrapper functions for new tasks handling proper setup/cleanup */
static void new_task_wrapper(void* ip)
{
    void (*func)() = ip;

    // call actual task function
    func();

    // since were done, terminate task
    scheduler_terminate_task();

    kassert(false);  // unreachable
}

/* Creates a new task executing the code at the address ip */
tid_t create_task(void* ip)
{
    uint32_t flags;
    task_t*  task = kalloc(sizeof(task_t));
    if (task == NULL) {
        return 0;
    }

    // Allocate stack
    task->kstack_bottom = vmem_request_free_page(0);
    if (task->kstack_bottom == NULL) {
        return 0;
    }

    task->kstack_size   = PAGE_SIZE;
    uintptr_t stack_top = task->kstack_bottom + task->kstack_size;

    // Setup thread registers
    init_thread_regs_with_stack(&task->regs, (void*)stack_top, new_task_wrapper, ip);

    // Initialise fields
    task->tid       = alloc_tid();
    task->time_used = 0;
    task->state     = BLOCKED;  // initially blocked, since the scheduler doesn't know about it yet
    task->status    = 0;
    task_data_init(&task->fs_data);
    atomic_store(&task->ref_count, 0);

    // Add to global task list
    list_add_last(&task_list, &task->task_list_entry);

    // Make the scheduler aware of the new task
    scheduler_unblock_task(task);

    return task->tid;
}

/* Function handling creation of the root task */
task_t* create_root_task()
{
    // The root task is a special case compared to regular, since it's already running and it's
    // stack was allocated during boot
    task_t* task = kalloc(sizeof(task_t));
    if (task == NULL) {
        return NULL;
    }

    // Setup thread registers
    init_initial_thread_regs(&task->regs);

    // Initialise fields
    task->tid       = alloc_tid();
    task->time_used = 0;
    task->state     = RUNNING;
    task->status    = 0;
    task_data_init(&task->fs_data);
    atomic_store(&task->ref_count, 0);

    // Add to global task list
    list_add_last(&task_list, &task->task_list_entry);
    return task;
}

/* Gives task control block associated to the supplied tid. Needs to call put_task() when do to
 * allow it to be properly cleaned up on termination */
task_t* get_task(tid_t tid)
{
    task_t*            task;
    struct list_entry* entry;

    if (tid == 0) {
        return NULL;
    }

    LIST_ITER(&task_list, entry)
    {
        task = GET_STRUCT(task_t, task_list_entry, entry);
        if (task->tid == tid) {
            // Only return non-killed tasks
            if (task->state != TERMINATED) {
                atomic_add_fetch(&task->ref_count, 1);
                return task;
            }
            return NULL;
        }
    }
    return NULL;
}

/* Mark the supplied task control block as no longer used */
void put_task(task_t* task)
{
    atomic_sub_fetch(&task->ref_count, 1);
}

/* Frees the memory of the task object */
void free_task(task_t* task)
{
    // Can't free the root task since it's created differently compared to other tasks
    kassert(task->tid != 0);

    // Free'ing up a process still in used, might put the kernel in a dangerous inconsistent state
    kassert(atomic_load(&task->ref_count) == 0);

    // Free'ing a task in a non-terminated state could be very dangerous
    kassert(task->state == TERMINATED);

    list_entry_remove(&task->task_list_entry);
    vmem_free_page(task->kstack_bottom);
    kfree(task);
}
