#include <arch/tty.h>
#include <klib/klib.h>
#include <klib/klib_internal.h>
#include <klib/libc.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>

static int term_putchar(int ic)
{
    unsigned char c = (char)ic;
    term_write(&c, sizeof(c));
    return ic;
}

int kvprintf(const char *restrict format, va_list args)
{
    return __fwriter(&term_putchar, format, args);
}

int kprintf(const char *restrict format, ...)
{
    int     ret;
    va_list args;

    va_start(args, format);
    ret = kvprintf(format, args);
    va_end(args);

    return ret;
}
