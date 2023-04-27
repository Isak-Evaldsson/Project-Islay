/*
    Implements the libc methods required for a gcc freestanding environment
*/
#include <klib/klib.h>

int memcmp(const void *lhs, const void *rhs, size_t count)
{
    const unsigned char *ul = (const unsigned char *)lhs;
    const unsigned char *ur = (const unsigned char *)rhs;

    for (size_t i = 0; i < count; i++) {
        if (ul[i] < ur[i])
            return -1;
        else if (ul[i] > ur[i])
            return 1;
    }
    return 0;
}

void *memcpy(void *__restrict dest, const void *__restrict src, size_t count)
{
    const unsigned char *usrc  = (const unsigned char *)src;
    unsigned char       *udest = (unsigned char *)dest;

    for (size_t i = 0; i < count; i++) udest[i] = usrc[i];

    return dest;
}

void *memmove(void *dest, const void *src, size_t count)
{
    const unsigned char *usrc  = (const unsigned char *)src;
    unsigned char       *udest = (unsigned char *)dest;

    if (udest < usrc) {
        for (size_t i = 0; i < count; i++) udest[i] = usrc[i];
    } else {
        for (size_t i = count; i != 0; i--) {
            udest[i - 1] = usrc[i - 1];
        }
    }
    return dest;
}

void *memset(void *dest, int ch, size_t count)
{
    unsigned char *udest = (unsigned char *)dest;
    for (size_t i = 0; i < count; i++) udest[i] = (unsigned char)ch;

    return dest;
}

size_t strlen(const char *str)
{
    size_t len = 0;

    while (str[len] != '\0') len++;

    return len;
}

int strcmp(const char *lhs, const char *rhs)
{
    while (*lhs && (*lhs == *rhs)) {
        lhs++;
        rhs++;
    }
    return *(const unsigned char *)lhs - *(const unsigned char *)rhs;
}

__attribute__((__noreturn__)) void abort()
{
    kpanic("abort()");
}