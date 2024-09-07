/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef _STDIO_H
#define _STDIO_H

#include <sys/cdefs.h>

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char *__restrict, ...);
int putchar(int);
int puts(const char *);

#ifdef __cplusplus
}
#endif

#endif
