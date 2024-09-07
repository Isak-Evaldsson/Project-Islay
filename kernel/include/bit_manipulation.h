/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef KLIB_BIT_MANIPULATION_H
#define KLIB_BIT_MANIPULATION_H

#define SET_BIT(b, n)  ((b) |= (1 << (n)))
#define CLR_BIT(b, n)  ((b) &= ~((1) << (n)))
#define MASK_BIT(b, n) ((b) & (1 << n))

#define GET_LOWEST_BITS(b, n)  ((b) & ((1 << (n)) - 1))
#define MASK_LOWEST_BITS(b, n) ((b) & ~((1 << (n)) - 1))

#endif /* KLIB_BIT_MANIPULATION_H */
