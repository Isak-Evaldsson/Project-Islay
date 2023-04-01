/*
    Basic timer api to generalize timekeeping
*/
#ifndef DEVICE_TIMER_H
#define DEVICE_TIMER_H

#include <stdint.h>

/*
    Get the system time
*/
uint32_t timer_get_time();

/*
    Resets the system timer
*/
void timer_reset();

/*
    Used for drivers to report the increase in time every clock pulse
*/
void timer_report_clock_pulse(uint32_t period_ms);

#endif /* DEVICE_TIMER_H */
