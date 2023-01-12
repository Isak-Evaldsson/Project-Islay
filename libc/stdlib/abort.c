#include <stdio.h>
#include <stdlib.h>

__attribute__((__noreturn__)) void abort()
{
#ifdef __is_libk
    printf("kernel: panic: abort()\n");
    asm volatile("hlt");
#else
    // TODO: Abnormally terminate the process as if by SIGABRT.
    printf("abort()\n");
#endif
    while (1)
        ;  // empty
    __builtin_unreachable();
}
