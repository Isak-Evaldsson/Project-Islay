#include <devices/timer.h>
#include <tasks/scheduler.h>

void nano_sleep(uint64_t nanoseconds)
{
    scheduler_nano_sleep_until(timer_get_time_since_boot() + nanoseconds);
}

void sleep(uint64_t seconds)
{
    nano_sleep(seconds * 1000000000);
}
