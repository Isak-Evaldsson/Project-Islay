/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <string.h>

int memcmp(const void *lhs, const void *rhs, size_t count)
{
    const unsigned char *ul = (const unsigned char *)lhs;
    const unsigned char *ur = (const unsigned char *)rhs;

    for (size_t i = 0; i < count; i++) {
        if (ul[i] < ur[i])
            return -1;
        else if (ul[i] > ur[i])
            return 1;
    }
    return 0;
}
