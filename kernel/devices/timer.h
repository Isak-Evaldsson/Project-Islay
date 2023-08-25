/*
    Basic timer api to generalize timekeeping
*/
#ifndef DEVICE_TIMER_H
#define DEVICE_TIMER_H

#include <stdint.h>

/*
    Get the system time in ns
*/
uint64_t timer_get_time_since_boot();

/*
    Used for drivers to report the increase in time every clock pulse
*/
void timer_report_clock_pulse(uint64_t period_ns);

#endif /* DEVICE_TIMER_H */
