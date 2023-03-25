#include <klib/klib.h>
#include <klib/libc.h>

__attribute__((__noreturn__)) void kpanic(const char *restrict format, ...)
{
    va_list args;

    // Panic header
    kprintf("kernel panic: ");

    // Message
    va_start(args, format);
    kvprintf(format, args);
    va_end(args);

    // Stop the kernel
    asm volatile("hlt");

    // Don't return
    while (1)
        ;  // empty
    __builtin_unreachable();
}