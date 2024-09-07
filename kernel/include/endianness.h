/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef KLIB_ENDIANNESS_H
#define KLIB_ENDIANNESS_H
#include <arch/arch.h>

/* Swap byte orders in integer */
uint16_t swap16(uint16_t n);
uint32_t swap32(uint32_t n);

/*
    Convert from big/little endian to native byte order
*/
#if ARCH_ENDIANNESS == ENDIAN_BIG
#define be16ton(x) (x)
#define be32ton(x) (x)
#define le16ton(x) swap16(x)
#define le32ton(x) swap32(x)

#elif ARCH_ENDIANNESS == ENDIAN_LITTLE
#define be16ton(x) swap16(x)
#define be32ton(x) swap32(x)
#define le16ton(x) (x)
#define le32ton(x) (x)

#else
#error "Architecture does not specify endianess"
#endif

#endif /* ENDIANNESS */
