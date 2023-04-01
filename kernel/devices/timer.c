#include <devices/timer.h>

// TODO: Have a more granular system with fixed point time to allowing avoiding clock drift for
// clock with a decimal period
static uint32_t time_ms;  // Time since the device was booted in ms

/*
    Get the system time
*/
uint32_t timer_get_time()
{
    return time_ms;
}

/*
    Resets the system timer
*/
void timer_reset()
{
    time_ms = 0;
}

/*
    Used for drivers to report the increase in time every clock pulse
*/
void timer_report_clock_pulse(uint32_t period_ms)
{
    time_ms += period_ms;
}
