/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/

/*
    Intel 8253/8254 Programmable Interval Timer (PIT) driver
*/
#include <arch/i686/io.h>
#include <arch/interrupts.h>
#include <devices/timer.h>
#include <stdbool.h>
#include <stdint.h>
#include <utils.h>

/* Macro for rounded integer division */
#define ROUND_IDIV(a, b) (((a) + ((b) / 2)) / (b))

/*
    IO-ports
*/
#define CHANNEL_0    0x40
#define CHANNEL_1    0x41
#define CHANNEL_2    0x42
#define CMD_REGISTER 0x43

#define SELECT_CHANNEL_0  0x00
#define MODE_SQUARED_WAVE 0x06
#define HI_LO_ACCESS_MODE 0x30

#define BASE_FREQUENCY 1193182  // Is technically 1193182.66.666....., or (3579545 / 3)

#define DEFAULT_FREQUENCY 1000  // 1 kHz, i.e. 1 ms period
#define MAX_FREQUENCY     (BASE_FREQUENCY)
#define MIN_FREQUENCY     ((BASE_FREQUENCY / 0x10000) + 1)

static uint32_t pit_frequency;
static uint32_t period_ms;

bool pit_set_frequency(uint32_t freq)
{
    uint32_t reload_value;

    // bound check
    if (freq > MAX_FREQUENCY || freq < MIN_FREQUENCY) {
        return false;  // failed to set freq
    }

    pit_frequency = freq;
    period_ms     = (1000 / freq);
    reload_value  = ROUND_IDIV(BASE_FREQUENCY, freq);

    // Handle special case, 0x10000 == 0 since PIT only can receive 16 bytes
    if (reload_value == 0x10000) {
        reload_value = 0x00;
    }

    outb(CHANNEL_0, reload_value & 0xff);
    outb(CHANNEL_0, (reload_value >> 8) & 0xff);

    return true;  // success
}

void pit_set_default_frequency()
{
    bool success = pit_set_frequency(DEFAULT_FREQUENCY);
    kassert(success);
}

void pit_init()
{
    outb(CMD_REGISTER, SELECT_CHANNEL_0 | HI_LO_ACCESS_MODE | MODE_SQUARED_WAVE);
    pit_set_default_frequency();
}

void pit_interrupt_handler(struct interrupt_stack_state *state, uint32_t interrupt_number)
{
    (void)state;
    (void)interrupt_number;

    timer_report_clock_pulse(period_ms * 1000000);
}
