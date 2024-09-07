/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <stdio.h>

int puts(const char *str)
{
    return printf("%s\n", str);
}
