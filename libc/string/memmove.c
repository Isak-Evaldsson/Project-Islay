#include <string.h>

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
