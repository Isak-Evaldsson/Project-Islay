#include <arch/boot.h>
#include <arch/tty.h>
#include <boot/multiboot.h>
#include <klib/klib.h>
#include <stdint.h>

/* The kernel main function */
extern void kernel_main();

/*
    The kernel initialisation code that is run, after the boot assembly code, but before
    entering kernel main.

    Performs 3 steps
    1. Reads and process multiboot info data from < 1 Mib
    2. Removes identity mapping
    3. Calls kernel main function
*/
void kernel_init(multiboot_info_t* mbd, uint32_t magic)
{
    /* Make sure the magic number matches for memory mapping */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        kpanic("invalid magic number!");
    }

    /* Check bit 6 to see if we have a valid memory map */
    if (!(mbd->flags >> 6 & 0x1)) {
        kpanic("invalid memory map given by GRUB bootloader");
    }

    /* Read relevant multiboot info... */

    /* Remove identity mapping, doing anything with the mbd pointer at this point will lead to
     * unrecoverable page faults */
    unmap_identity_mapping();

    /* Enter kernel main */
    kernel_main();
}