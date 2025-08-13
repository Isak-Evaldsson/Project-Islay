/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_i686_PROCESSOR_H
#define ARCH_i686_PROCESSOR_H
#include <stdint.h>

/*
    Functions to read registers
*/
uint32_t get_esp();
uint32_t get_cr2();
uint32_t get_cr3();

#endif /* ARCH_i686_PROCESSOR_H */
