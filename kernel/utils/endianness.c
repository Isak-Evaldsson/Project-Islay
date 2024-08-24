#include <endianness.h>

uint16_t swap16(uint16_t n)
{
    return (n >> 8) |  // Move byte 1 to 0
           (n << 8);   // Move byte 0 to 1
}

uint32_t swap32(uint32_t n)
{
    return (n >> 24) & 0x000000ff |  // Move byte 3 to 0
           (n >> 8) & 0x0000ff00 |   // Move byte 2 to 1
           (n << 8) & 0x00ff0000 |   // Move byte 1 to 2
           (n << 24) & 0xff000000;   // Move byte 0 to byte 3
}
