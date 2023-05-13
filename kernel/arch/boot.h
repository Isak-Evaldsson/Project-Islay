#ifndef ARCH_BOOT_H
#define ARCH_BOOT_H
#include <stdint.h>

/* Each architecture is required to implement an assembly routine unmapping the identity mapping
 * that was setup as a part of the higher-half booting procedure */
void unmap_identity_mapping();

/*
    Kernel linker script is required to expose a start and end address for the kernel
*/
extern uint32_t _kernel_start[];      // start symbol, assumed to be physical address
extern uint32_t _kernel_end[];        // end symbol, virtual address since higher half kernel
extern uint32_t _higher_half_addr[];  // indicating the start of higher half area

/*
    Macros correctly typecasting the symbols to integer, allowing
*/
#define KERNEL_START     ((uint32_t)_kernel_start)
#define KERNEL_END       ((uint32_t)_kernel_end)
#define HIGHER_HALF_ADDR ((uint32_t)_higher_half_addr)

#endif /* ARCH_BOOT_H */
