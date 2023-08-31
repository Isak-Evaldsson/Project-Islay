#ifndef KLIB_KLIB_H
#define KLIB_KLIB_H
#include <stdarg.h>

/* Generic formated writer, allowing formated printing to any character device. */
int __fwriter(int (*putchar)(int), const char *restrict format, va_list args)

#endif /* KLIB_KLIB_H */
