/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>
#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

int    memcmp(const void *lhs, const void *rhs, size_t count);
void  *memcpy(void *__restrict dest, const void *__restrict src, size_t count);
void  *memmove(void *dest, const void *src, size_t count);
void  *memset(void *dest, int ch, size_t count);
size_t strlen(const char *str);

#ifdef __cplusplus
}
#endif
#endif
