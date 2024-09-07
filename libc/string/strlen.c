/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <string.h>

size_t strlen(const char *str)
{
    size_t len = 0;

    while (str[len] != '\0')
        len++;

    return len;
}
