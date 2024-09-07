/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/

/*
    Basic timer api to generalize timekeeping
*/
#ifndef DEVICE_TIMER_H
#define DEVICE_TIMER_H

#include <stdbool.h>
#include <stdint.h>

/* TODO: Use time_t for time api */

/*
    Helper macro converting seconds to nanoseconds, ensuring no integer constant overflow
*/
#define SECONDS_TO_NS(second) ((second) * 1000000000ull)

/*
    Type definition for time event callbacks returns both the time since boot and the registered
    timestamp allowing the callback to compensate if they differ.
 */
typedef void (*timed_event_callback)(uint64_t time_since_boot_ns, uint64_t timestamp_ns);

/*
    Allows the registration of timed events, one the supplied timestamps is reached the callback
    will be executed. The time system does not grantee the callback to be invoked at exactly the
    specified time, however it grantees to not invoke it earlier than the specified timestamp.
 */
bool timer_register_timed_event(uint64_t timestamp_ns, timed_event_callback callback);

/*
    Get the system time in ns
*/
uint64_t timer_get_time_since_boot();

/*
    Used for drivers to report the increase in time every clock pulse
*/
void timer_report_clock_pulse(uint64_t period_ns);

#endif /* DEVICE_TIMER_H */
