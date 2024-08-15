#ifndef KLIB_LIBC_H
#define KLIB_LIBC_H
/*
    Implements the libc methods required for a gcc freestanding environment
*/

#include <stddef.h>

/* static_assert from c23, assumes the gcc ## preprocessor extension to be available */
#define static_assert(expr, ...) _Static_assert(expr, ##__VA_ARGS__)

int    memcmp(const void *lhs, const void *rhs, size_t count);
void  *memcpy(void *__restrict dest, const void *__restrict src, size_t count);
void  *memmove(void *dest, const void *src, size_t count);
void  *memset(void *dest, int ch, size_t count);
size_t strlen(const char *str);
int    strcmp(const char *lhs, const char *rhs);
int    strncmp(const char *lhs, const char *rhs, size_t count);
char  *strdup(const char *str1);

/* We deviate from the standard by making strtok reentrant. There's no need of a non-reentrant
 * strtok in the kernel, it while only cause bugs in reentrant code. */
char *strtok(char *str, const char *delim, char **saveptr);

__attribute__((__noreturn__)) void abort();

#endif /* KLIB_LIBC_H */
