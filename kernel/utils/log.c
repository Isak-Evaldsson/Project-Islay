#include <arch/serial.h>
#include <utils.h>

#include "internal.h"

static int serial_putchar(int ic)
{
    unsigned char c = (char)ic;
    serial_put(c);
    return ic;
}

/* Logs formated message to serial output */
int log(const char *restrict format, ...)

{
    int     ret;
    va_list args;

    va_start(args, format);
    ret = __fwriter(&serial_putchar, format, args);
    va_end(args);

    // Add proper line endings
    serial_put('\r');
    serial_put('\n');

    return ret;
}
