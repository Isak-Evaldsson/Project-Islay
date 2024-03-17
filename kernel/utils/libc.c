/*
    Implements the libc methods required for a gcc freestanding environment
*/
#include <utils.h>

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

int strncmp(const char *lhs, const char *rhs, size_t count)
{
    while (count && *lhs && (*lhs == *rhs)) {
        lhs++;
        rhs++;
        count--;
    }
    return *(const unsigned char *)lhs - *(const unsigned char *)rhs;
}

char *strdup(const char *str1)
{
    size_t len = strlen(str1) + 1;  // Include null termination
    char  *str = kmalloc(len);
    if (str)
        memcpy(str, str1, len);

    return str;
}

char *strtok(char *str, const char *delim, char **saveptr)
{
    const char *d;
    char       *s, *start = str;

    if (str) {
        // First parse: Handle string starting with token
        d = delim;
        while (*start && *start == *d) {
            start++;
            d++;
        }
    } else {
        start = *saveptr;
    }
    s = start;

    // Find start of delimiter, i.e. end of token
    while (*s && *s != *delim) s++;

    // If end of string no need to check the delimiter
    if (!s[0]) {
        goto end;
    }

    *s = '\0';  // mark end of token
    do {
        // Skip comparing the first char since we have inserted a null termination,
        // the first check is done by the previous while anyway.
        s++;
        delim++;
    } while (*s && *s == *delim);

end:
    *saveptr = s;
    return start;
}

__attribute__((__noreturn__)) void abort()
{
    kpanic("abort()");
}