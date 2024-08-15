#include <arch/tty.h>
#include <libc.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <utils.h>

#include "internal.h"

static int term_putchar(int ic)
{
    unsigned char c = (char)ic;
    term_write(&c, sizeof(c));
    return ic;
}

int kvprintf(const char *restrict format, va_list args)
{
    struct fwriter_ops ops = {
        .type = FWRITER_CHARDEV,
        .args = {.putchar = term_putchar},
    };
    return __fwriter(&ops, format, args);
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
