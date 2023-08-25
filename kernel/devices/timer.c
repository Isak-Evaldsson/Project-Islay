#include <devices/timer.h>
#include <tasks/scheduler.h>

// TODO: Have a more granular system with fixed point time to allowing avoiding clock drift for
// clock with a decimal period
static uint64_t time_since_boot_ns;  // Time since the device was booted in ns allows about 584
                                     // years of uptime before overflow, should proably be enough :)

/*
    Get the system time
*/
uint64_t timer_get_time_since_boot()
{
    return time_since_boot_ns;
}

/*
    Used for drivers to report the increase in time every clock pulse
*/
void timer_report_clock_pulse(uint64_t period_ns)
{
    time_since_boot_ns += period_ns;
}
