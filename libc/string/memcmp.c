#include <string.h>

int memcmp(const void *lhs, const void *rhs, size_t count)
{
    const unsigned char *ul = (const unsigned char *)lhs;
    const unsigned char *ur = (const unsigned char *)rhs;

    for (size_t i = 0; i < count; i++)
    {
        if (ul[i] < ur[i])
            return -1;
        else if (ul[i] > ur[i])
            return 1;
    }
    return 0;
}