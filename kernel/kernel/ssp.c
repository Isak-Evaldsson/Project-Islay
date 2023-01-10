/*
    Stack Smashing Protector, SSP

    Currently a minimal possible implementation, currently more used for debugging than security
*/

#include <stdint.h>
#include <stdio.h>

#if UINT32_MAX == UINTPTR_MAX
#define STACK_CHK_GUARD 0xe2dee396
#else
#define STACK_CHK_GUARD 0x595e9fbd94fda766
#endif

// Should be a randomized that is decided at program load
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

// Handler called we stack smashing is detected
__attribute__((noreturn)) void __stack_chk_fail(void)
{
    printf("\nStack smashing detected\n");
    abort();
}
