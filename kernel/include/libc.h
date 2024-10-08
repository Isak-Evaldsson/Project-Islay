/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef KLIB_LIBC_H
#define KLIB_LIBC_H
/*
    Implements the libc methods required for a gcc freestanding environment
*/

#include <stdarg.h>
#include <stddef.h>

/* static_assert from c23, assumes the gcc ## preprocessor extension to be available */
#define static_assert(expr, ...) _Static_assert(expr, ##__VA_ARGS__)

int    memcmp(const void *lhs, const void *rhs, size_t count);
void  *memcpy(void *__restrict dest, const void *__restrict src, size_t count);
void  *memmove(void *dest, const void *src, size_t count);
void  *memset(void *dest, int ch, size_t count);
size_t strlen(const char *str);
size_t strnlen(const char *str, size_t count);
int    strcmp(const char *lhs, const char *rhs);
int    strncmp(const char *lhs, const char *rhs, size_t count);
char  *strdup(const char *str1);
char  *strcpy(char *restrict dest, const char *restrict src);

/* We deviate from the standard by making strtok reentrant. There's no need of a non-reentrant
 * strtok in the kernel, it while only cause bugs in reentrant code. */
char *strtok(char *str, const char *delim, char **saveptr);

int snprintf(char *restrict buffer, size_t bufsz, const char *restrict format, ...);
int vsnprintf(char *restrict buffer, size_t bufsz, const char *restrict format, va_list vlist);

__attribute__((__noreturn__)) void abort();

#endif /* KLIB_LIBC_H */
