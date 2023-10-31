#include <utils.h>
#include <limits.h>

#include "internal.h"

// Converts unsigned int to string, returns number of chars
static size_t itoa(unsigned int n, char *buffer, size_t len, char radix)
{
    size_t       i;  // buffer index, equals to strlen after first loop
    unsigned int tmp     = n;
    unsigned int divisor = 10;

    // if n == zero, place a 0 in the buffer and return
    if (n == 0 && len > 1) {
        buffer[0] = '0';
        buffer[1] = '\n';
        return 1;
    }

    if (radix == 'x')
        divisor = 16;
    else if (radix == 'o')
        divisor = 8;

    // Writing digits to buffer, first to last
    for (i = 0; tmp > 0 && i < (len - 1); i++) {
        char c    = (tmp % divisor);
        buffer[i] = (c < 10) ? ('0' + c) : (c - 10 + 'a');
        tmp       = tmp / divisor;
    }
    buffer[i] = '\0';

    // Reverse buffer
    for (size_t j = 0; j < i / 2; j++) {
        // swap(j, i - j - 1)
        char tmp          = buffer[j];
        buffer[j]         = buffer[i - j - 1];
        buffer[i - j - 1] = tmp;
    }

    return i;
}

static bool print(int (*putchar)(int), const char *data, size_t length)
{
    const unsigned char *bytes = (const unsigned char *)data;
    for (size_t i = 0; i < length; i++)
        if (putchar(bytes[i]) == EOF) return false;
    return true;
}

/* Generic printf function */
int __fwriter(int (*putchar)(int), const char *restrict format, va_list args)
{
    char radix;
    int  written = 0;
    char numstr[32 + 1];  // buffer can fit a 32bit binary number string

    while (*format != '\0') {
        size_t maxrem = INT_MAX - written;

        if (format[0] != '%' || format[1] == '%') {
            if (format[0] == '%') format++;
            size_t amount = 1;
            while (format[amount] && format[amount] != '%') amount++;
            if (maxrem < amount) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            if (!print(putchar, format, amount)) return -1;
            format += amount;
            written += amount;
            continue;
        }

        const char *format_begun_at = format++;

        if (*format == 'c') {
            format++;
            char c = (char)va_arg(args, int /* char promotes to int */);
            if (!maxrem) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            if (!print(putchar, &c, sizeof(c))) return -1;
            written++;
        } else if (*format == 's') {
            format++;
            const char *str = va_arg(args, const char *);
            size_t      len = strlen(str);
            if (maxrem < len) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            if (!print(putchar, str, len)) return -1;
            written += len;
        } else if ((radix = *format) == 'u' || radix == 'o' || radix == 'x') {
            format++;
            unsigned int num = va_arg(args, unsigned int);
            size_t       len = itoa(num, numstr, sizeof(numstr), radix);

            if (!print(putchar, numstr, len)) return -1;
            written += len;
        } else {
            format     = format_begun_at;
            size_t len = strlen(format);
            if (maxrem < len) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            if (!print(putchar, format, len)) return -1;
            written += len;
            format += len;
        }
    }
    return written;
}
