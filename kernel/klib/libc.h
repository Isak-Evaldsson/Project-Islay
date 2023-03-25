#ifndef KLIB_LIBC_H
#define KLIB_LIBC_H
/*
    Implements the libc methods required for a gcc freestanding environment
*/

#include <stddef.h>

int    memcmp(const void *lhs, const void *rhs, size_t count);
void  *memcpy(void *__restrict dest, const void *__restrict src, size_t count);
void  *memmove(void *dest, const void *src, size_t count);
void  *memset(void *dest, int ch, size_t count);
size_t strlen(const char *str);
__attribute__((__noreturn__)) void abort();

#endif /* KLIB_LIBC_H */