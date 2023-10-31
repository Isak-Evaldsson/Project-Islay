#ifndef KLIB_INTERNAL_H
#define KLIB_INTERNAL_H
#include <stdarg.h>

/* Generic formatted writer, allowing formatted printing to any character device. */
int __fwriter(int (*putchar)(int), const char *restrict format, va_list args);

#endif /* KLIB_INTERNAL_H */
