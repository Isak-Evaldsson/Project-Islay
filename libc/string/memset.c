/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <string.h>

void *memset(void *dest, int ch, size_t count)
{
    unsigned char *udest = (unsigned char *)dest;
    for (size_t i = 0; i < count; i++)
        udest[i] = (unsigned char)ch;

    return dest;
}
