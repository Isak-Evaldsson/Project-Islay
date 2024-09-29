/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_i386_PIT_H
#define ARCH_i386_PIT_H

#include <stdbool.h>

#include "../interrupts/pic.h"

#define PIT_INTERRUPT_NUM 0  // relative to pic

bool pit_set_frequency(uint32_t freq);
void pit_set_default_frequency();
void pit_init();
void pit_interrupt_handler();

#endif /* ARCH_i386_PIT_H */
