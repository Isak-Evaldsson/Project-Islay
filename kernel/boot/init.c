#include <arch/boot.h>
#include <arch/tty.h>
#include <boot/multiboot.h>
#include <devices/ps2_keyboard.h>
#include <memory/page_frame_manager.h>
#include <stdint.h>
#include <utils.h>

/* The kernel main function */
extern void kernel_main();

#define MiB (2 ^ 20)

/*
    The kernel initialisation code that is run, after the boot assembly code, but before
    entering kernel main.

    Performs 3 steps
    1. Reads and process multiboot info data from < 1 Mib
    2. Removes identity mapping
    3. Calls kernel main function
*/
void kernel_init(multiboot_info_t *mbd, uint32_t magic)
{
    /* Make sure the magic number matches for memory mapping */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        kpanic("invalid magic number!");
    }

    /* Check bit 6 to see if we have a valid memory map */
    if (!(mbd->flags >> 6 & 0x1)) {
        kpanic("invalid memory map given by GRUB bootloader");
    }

    /* Reading memory map */
    size_t           n_entries = mbd->mmap_length / sizeof(multiboot_memory_map_t);
    memory_segment_t available_segments[n_entries];
    memory_map_t     map = {.memory_amount = 0, .n_segments = 0, .segments = available_segments};
    multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mbd->mmap_addr;

    for (size_t i = 0; i < n_entries; i++) {
        multiboot_memory_map_t *entry = mmap + i;

        // Find available memory area above 1 MiB
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE && entry->addr > MiB) {
            map.memory_amount = entry->addr + entry->len;

            available_segments[map.n_segments++] =
                (memory_segment_t){.addr = entry->addr, .length = entry->len};
        }
    }

    page_frame_manager_init(&map);

    /* Remove identity mapping, doing anything with the mbd pointer at this point will lead to
     * unrecoverable page faults */
    unmap_identity_mapping();

    /* Enter kernel main */
    kernel_main();
}