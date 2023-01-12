#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static bool print(const char *data, size_t length)
{
    const unsigned char *bytes = (const unsigned char *)data;
    for (size_t i = 0; i < length; i++)
        if (putchar(bytes[i]) == EOF) return false;
    return true;
}

// Converts unsigned int to string, returns number of chars
static size_t unsigned_to_str(unsigned int n, char *buffer, size_t len)
{
    size_t       i;  // buffer index, equals to strlen after first loop
    unsigned int tmp = n;

    // Writing digits to buffer, first to last
    for (i = 0; tmp > 0 && i < (len - 1); i++) {
        char c    = '0' + (tmp % 10);
        buffer[i] = c;  //'0' + (tmp % 10);
        tmp       = tmp / 10;
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

int printf(const char *restrict format, ...)
{
    va_list params;
    va_start(params, format);

    int written = 0;

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
            if (!print(format, amount)) return -1;
            format += amount;
            written += amount;
            continue;
        }

        const char *format_begun_at = format++;

        if (*format == 'c') {
            format++;
            char c = (char)va_arg(params, int /* char promotes to int */);
            if (!maxrem) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            if (!print(&c, sizeof(c))) return -1;
            written++;
        } else if (*format == 's') {
            format++;
            const char *str = va_arg(params, const char *);
            size_t      len = strlen(str);
            if (maxrem < len) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            if (!print(str, len)) return -1;
            written += len;
        } else if (*format == 'u') {
            format++;
            unsigned int num = va_arg(params, unsigned int);
            char         numstr[100];  // TODO: replace with not hardcoded number (heap alloacted?)
            size_t       len = unsigned_to_str(num, numstr, sizeof(numstr));

            if (!print(numstr, len)) return -1;
            written += len;
        } else {
            format     = format_begun_at;
            size_t len = strlen(format);
            if (maxrem < len) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            if (!print(format, len)) return -1;
            written += len;
            format += len;
        }
    }

    va_end(params);
    return written;
}
