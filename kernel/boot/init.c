#include <arch/boot.h>
#include <arch/paging.h>
#include <arch/serial.h>
#include <arch/tty.h>
#include <boot/multiboot.h>
#include <devices/ps2_keyboard.h>
#include <memory/page_frame_manager.h>
#include <stdint.h>
#include <utils.h>

/* The kernel main function */
extern void kernel_main(struct boot_data *boot_data);

#define MiB (2 ^ 20)

static struct boot_data boot_data;

void initrd_relocation(physaddr_t old, physaddr_t new, size_t size)
{
    // Check that the address ranges do not overlap by comparing the distances between them
    kassert((old > new ? old - new : new - old) > size);

    size_t page_count = ALIGN_BY_PAGE_SIZE(size) / PAGE_SIZE;
    for (size_t i = 0; i < page_count; i++) {
        map_page(new + i, P2L(new + i), PAGE_OPTION_WRITABLE);
        map_page(old + i, P2L(old + i), PAGE_OPTION_WRITABLE);
    }

    memcpy((void *)P2L(new), (void *)P2L(old), size);
    for (size_t i = 0; i < page_count; i++) {
        unmap_page(P2L(old + i));
        map_page(new + i, P2L(new + i), 0);  // Re-map page to read-only
    }
}

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
    // Init terminal + serial first to make the rest of the start process easier to debug
    term_init();
    if (serial_init() == 1) {
        kpanic("No serial\n");
    }

    /* Make sure the magic number matches for memory mapping */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        kpanic("invalid magic number!");
    }

    /* Check bit 6 to see if we have a valid memory map */
    if (!(mbd->flags >> 6 & 0x1)) {
        kpanic("invalid memory map given by GRUB bootloader");
    }

    /* Reading memory map */
    size_t mmap_size = mbd->mmap_length / sizeof(multiboot_memory_map_t);
    if (mmap_size > MEMMAP_SEGMENT_MAX) {
        kpanic("Too many memory segments, %u", mbd->mmap_length);
    }

    boot_data.mem_size  = 0;
    boot_data.mmap_size = 0;
    for (size_t i = 0; i < mmap_size; i++) {
        multiboot_memory_map_t *entry = (multiboot_memory_map_t *)mbd->mmap_addr + i;

        // Find available memory area above 1 MiB
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE && entry->addr > MiB) {
            boot_data.mem_size = entry->addr + entry->len;

            boot_data.mmap_segments[boot_data.mmap_size++] =
                (memory_segment_t){.addr = entry->addr, .length = entry->len};
        }
    }

    if (mbd->mods_count < 1) {
        kpanic("Boot failure: missing initrd");
    }

    multiboot_module_t *initrd_mod = (multiboot_module_t *)mbd->mods_addr;

    // Relocate initrd to the first available page after the kernel bss
    size_t     initrd_size  = initrd_mod->mod_end - initrd_mod->mod_start;
    physaddr_t initrd_start = ALIGN_BY_PAGE_SIZE(KERNEL_END - HIGHER_HALF_ADDR);
    initrd_relocation(initrd_mod->mod_start, initrd_start, initrd_size);

    boot_data.initrd_size  = initrd_size;
    boot_data.initrd_start = initrd_start;

    /* Remove identity mapping, doing anything with the mbd pointer at this point will lead to
     * unrecoverable page faults */
    unmap_identity_mapping();

    /* Enter kernel main */
    kernel_main(&boot_data);
}