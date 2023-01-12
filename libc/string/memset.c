#include <string.h>

void *memset(void *dest, int ch, size_t count)
{
    unsigned char *udest = (unsigned char *)dest;
    for (size_t i = 0; i < count; i++) udest[i] = (unsigned char)ch;

    return dest;
}
