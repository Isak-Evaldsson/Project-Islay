#ifndef KLIB_KLIB_H
#define KLIB_KLIB_H
#include <klib/libc.h>
#include <stdarg.h>
#include <stdint.h>

#define EOF (-1)  // End of file

// Kernel equivalent of libc macro
#define kassert(expr)                                                                 \
    if (!(expr)) {                                                                    \
        kpanic("kernel assertion '%s' failed at %s:%u\n", #expr, __FILE__, __LINE__); \
    }

/* Kernel equivalent to libc printf */
int kprintf(const char *restrict, ...);

/* Equivalent to kprintf except it allows passthrough of varargs */
int kvprintf(const char *restrict, va_list);

/* Kernel panic, displays error message and halts the kernel */
__attribute__((__noreturn__)) void kpanic(const char *restrict, ...);

#endif /* KLIB_KLIB_H */