#ifndef KLIB_KLIB_H
#define KLIB_KLIB_H
#include <bit_manipulation.h>
#include <libc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define EOF (-1)  // End of file

// Kernel equivalent of libc macro, can be disabled using NDEBUG
#ifdef NDEBUG
#define kassert() ()
#else
#define kassert(expr)                                                                 \
    if (!(expr)) {                                                                    \
        kpanic("kernel assertion '%s' failed at %s:%u\n", #expr, __FILE__, __LINE__); \
    }
#endif

/* Kernel equivalent to libc printf */
int kprintf(const char *restrict, ...);

/* Equivalent to kprintf except it allows passthrough of varargs */
int kvprintf(const char *restrict, va_list);

/* Logs formated message to serial output */
int log(const char *restrict format, ...);

/* Read line from keyboard into string of a give size, allowing basic kernel shell input */
void kreadline(size_t size, char *str);

/* kernel heap functions */
void *kmalloc(size_t size);
void  kfree(void *ptr);
void *kcalloc(size_t num, size_t size);
void *krealloc(void *ptr, size_t new_size);

/* sleep functions */
void sleep(uint64_t seconds);
void nano_sleep(uint64_t nanoseconds);

/* Kernel panic, displays error message and halts the kernel */
__attribute__((__noreturn__)) void kpanic(const char *restrict, ...);

#endif /* KLIB_KLIB_H */