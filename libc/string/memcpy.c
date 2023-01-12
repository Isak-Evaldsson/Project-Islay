#include <string.h>

void *memcpy(void *__restrict dest, const void *__restrict src, size_t count)
{
    const unsigned char *usrc  = (const unsigned char *)src;
    unsigned char       *udest = (unsigned char *)dest;

    for (size_t i = 0; i < count; i++) udest[i] = usrc[i];

    return dest;
}
