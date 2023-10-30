#include <klib/klib.h>
#include <tasks/locking.h>
#include <tasks/scheduler.h>

static mutex_t *m;

void sleeper()
{
    int i = 0;

    for (;;) {
        log("Sleeper %u", ++i);
        sleep(1);
    }
}

void f1()
{
    mutex_lock(m);
    kprintf("f1 acquired lock\n");
    mutex_unlock(m);
}

void f2()
{
    mutex_lock(m);
    kprintf("f2 acquired lock\n");
    mutex_unlock(m);
}

void scheduler_test()
{
    //
    // Sleep tests
    //
    kprintf("Spawn new task\n");
    m = mutex_create();
    scheduler_create_task(&sleeper);
    scheduler_create_task(&f1);
    scheduler_create_task(&f2);
    mutex_lock(m);

    kprintf("Back to main.");
    for (int i = 0; i < 5; i++) {
        sleep(1);
        kprintf(".");
    }
    kprintf("\n");
    mutex_unlock(m);
}
