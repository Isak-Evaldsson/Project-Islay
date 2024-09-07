/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef KLIB_INTERNAL_H
#define KLIB_INTERNAL_H
#include <stdarg.h>

typedef enum {
    FWRITER_CHARDEV,
    FWRITER_BUFFER,
} fwriter_type;

/* Argument to __fwritter specifying what to write to and required arguments */
struct fwriter_ops {
    fwriter_type type;
    union {
        int (*putchar)(int);  // used by type cdev
        struct {
            char  *buff;
            size_t len;
        } buff_ops;
    } args;
};

/* Generic formatted writer, allowing formatted printing to any character device. */
int __fwriter(struct fwriter_ops *ops, const char *restrict format, va_list args);

#endif /* KLIB_INTERNAL_H */
