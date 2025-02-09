/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef KLIB_KLIB_H
#define KLIB_KLIB_H
#include <bit_manipulation.h>
#include <endianness.h>
#include <libc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Rounds up a number to a multiple of n, assuming n is a factor of 2 */
#define ALIGN_BY_MULTIPLE(num, n)           \
    ({                                      \
        static_assert(n % 2 == 0);          \
        (((num) + ((n) - 1)) & ~((n) - 1)); \
    })

/* Maximum value between two ints */
#define MAX(a, b)           \
    ({                      \
        typeof(a) _a = (a); \
        typeof(b) _b = (b); \
        _a > _b ? _a : _b;  \
    })

/* Minimum value between two ints */
#define MIN(a, b)           \
    ({                      \
        typeof(a) _a = (a); \
        typeof(b) _b = (b); \
        _a < _b ? _a : _b;  \
    })

/*
    If one has a pointer to struct member m within struct s, the pointer to the struct can be
    computed with the following macro:

    struct_ptr = GET_STRUCT(s, m, field_ptr);
 */
#define GET_STRUCT(type, member, ptr)                       \
    ({                                                      \
        typeof(((type *)0)->member) *_ptr = (ptr);          \
        (type *)((uintptr_t)_ptr - offsetof(type, member)); \
    })

/* Count the number of elements in a statically allocated array */
#define COUNT_ARRAY_ELEMS(array) (sizeof(array) / sizeof(array[0]))

/*
    Returns a pointer to the end of a statically allocated array, useful as a stop condition when
    iterating over an array.
*/
#define END_OF_ARRAY(array) (array + COUNT_ARRAY_ELEMS(array))

#define EOF (-1) /* End of file */

/* Statically assert the offset for struct/union member, useful to ensure compatibly when struct
 * fields are used by both generic c code and architecture specific asm. */
#define assert_offset(type, field, offset) static_assert(offsetof(type, field) == offset)

/* Kernel equivalent of libc macro, can be disabled using NDEBUG */
#ifdef NDEBUG
#define kassert() ()
#else
#define kassert(expr)                                                                 \
    if (!(expr)) {                                                                    \
        kpanic("kernel assertion '%s' failed at %s:%u\n", #expr, __FILE__, __LINE__); \
    }
#endif

/*
    Generic function to allow per subsystem log macros, e.g:

    #define LOG(fmt, ...) __LOG(DEBUG_SUBSYS, "[SUBSYS_NAME]", fmt, ##__VA_ARGS__)
*/
#define __LOG(var, subsys, fmt, ...)                          \
    do {                                                      \
        if (var)                                              \
            log(subsys " %s: " fmt, __func__, ##__VA_ARGS__); \
    } while (0)

/*
    Kernel equivalent to libc printf
    TODO: add formater gcc macro
    TODO: handle signed integers
*/
int kprintf(const char *restrict, ...);

/* Equivalent to kprintf except it allows passthrough of varargs */
int kvprintf(const char *restrict, va_list);

/* Logs formatted message to serial output */
int log(const char *restrict format, ...);

/* kernel heap functions */

/* Does always clear the memory before returning the pointer */
void *kalloc(size_t size);
void  kfree(void *ptr);
void *krealloc(void *ptr, size_t new_size);

/* sleep functions */
void sleep(uint64_t seconds);
void nano_sleep(uint64_t nanoseconds);

/* Kernel panic, displays error message and halts the kernel */
__attribute__((__noreturn__)) void kpanic(const char *restrict, ...);

#endif /* KLIB_KLIB_H */
