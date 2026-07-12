/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/boot.h>
#include <arch/paging.h>
#include <arch/sections.h>
#include <arch/serial.h>
#include <arch/tty.h>
#include <boot/multiboot.h>
#include <devices/keyboard/ps2_keyboard.h>
#include <initobj.h>
#include <memory/memory_map.h>
#include <memory/page_frame_manager.h>
#include <stdint.h>
#include <uapi/errno.h>
#include <utils.h>

#define INIT_SECTION_START  GET_LINKER_SYMBOL(_initobjs_start)
#define INIT_SECTION_END    GET_LINKER_SYMBOL(_initobjs_end)

/* The kernel main function */
extern void kernel_main(struct boot_data *boot_data);

#define MiB (2 ^ 20)

static struct boot_data boot_data;

static void initrd_relocation(physaddr_t old, physaddr_t new, size_t size)
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

static unsigned int map_range_unaligned(physaddr_t paddr, virtaddr_t vaddr, size_t size,
        uint16_t flags, physaddr_t *start_page)
{
    physaddr_t start;
    unsigned int page_count = ALIGN_BY_PAGE_SIZE(size) / PAGE_SIZE;

    // For unaligned addresses we need to add the previous page as well
    if (paddr % PAGE_SIZE) {
        start = ALIGN_BY_PAGE_SIZE(paddr) - PAGE_SIZE;
        page_count++;
    } else {
        start = paddr;
    }

    for (size_t i = 0; i < page_count; i++)
        map_page(start + i * PAGE_SIZE, vaddr + i * PAGE_SIZE, flags);

    if (start_page)
        *start_page = start;

    return page_count;
}

static int parse_multiboot_header(physaddr_t multiboot_addr, virtaddr_t vaddr)
{
    int ret = 0;
    virtaddr_t offset;
    physaddr_t start;
    unsigned int page_count;
    struct multiboot_mmap_entry *mmap_start, *entry;
    struct multiboot_mod_list *initrd_mod;
    struct multiboot_info *mbd;
    size_t mmap_size;

    page_count = map_range_unaligned(multiboot_addr, vaddr, sizeof(*mbd), 0, &start);
    mbd = (multiboot_info_t *)(vaddr + (multiboot_addr - start));
    if (!(mbd->flags & MULTIBOOT_INFO_MEM_MAP)) {
        ret = -EINVAL;
        goto unmap;
    }

    mmap_size = mbd->mmap_length / sizeof(multiboot_memory_map_t);

    offset = vaddr + (page_count * PAGE_SIZE);
    page_count += map_range_unaligned(mbd->mmap_addr, offset, mbd->mmap_length, 0, &start);
    mmap_start = (struct multiboot_mmap_entry *)(offset + (mbd->mmap_addr - start));
    for (size_t i = 0; i < mmap_size; i++) {
        entry = mmap_start + i;

        memory_map_add_entry((physaddr_t)entry->addr, (physaddr_t)entry->len,
                    entry->type == MULTIBOOT_MEMORY_AVAILABLE);
    }

    if (mbd->mods_count < 1) {
        ret = -ENODATA;
        goto unmap;
    }

    offset = vaddr + (page_count * PAGE_SIZE);
    page_count += map_range_unaligned(mbd->mods_addr, offset, sizeof(*initrd_mod), 0, &start);
    initrd_mod = (multiboot_module_t *)(offset + (mbd->mods_addr - start));

    boot_data.initrd_start = initrd_mod->mod_start;
    boot_data.initrd_size = initrd_mod->mod_end - initrd_mod->mod_start;

unmap:
    for (size_t i = 0; i < page_count; i++)
        unmap_page(vaddr + i * PAGE_SIZE);

    return ret;
}

/*
    The kernel initialisation code that is run, after the boot assembly code, but before
    entering kernel main.
*/
void kernel_init(physaddr_t multiboot_addr, uint32_t magic)
{
    int ret;

    // Init terminal + serial first to make the rest of the start process easier to debug
    term_init();
    if (serial_init() == 1) {
        kpanic("No serial\n");
    }

    // Make sure the magic number matches for memory mapping
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        kpanic("invalid magic number!");
    }

    ret = parse_multiboot_header(multiboot_addr, ALIGN_BY_PAGE_SIZE(KERNEL_END + 1));
    if (ret)
        kpanic("Multiboot header parsing failed: %i\n", ret);

    kassert(KERNEL_END == INIT_SECTION_END);
    parse_init_section((struct init_object **)INIT_SECTION_START,
            (struct init_object **)INIT_SECTION_END);
    /*
     * Unmap init section since it's no longer needed, will be overwritten by initrd allocation.
     * If initrd size < init section size, there will be pages from the init section that is still
     * mapped. To avoid this, just umap all pages before initrd rellocation.
     */
    size_t init_page_count =  ALIGN_BY_PAGE_SIZE(INIT_SECTION_END - INIT_SECTION_START) / PAGE_SIZE;
    for (size_t i = 0; i < init_page_count; i++) {
        unmap_page(INIT_SECTION_START + i);
    }

    // Relocate initrd to the first available page after the kernel bss, i.e start of init section
    physaddr_t new_initrd_addr = ALIGN_BY_PAGE_SIZE(INIT_SECTION_START - HIGHER_HALF_ADDR);
    initrd_relocation(boot_data.initrd_start, new_initrd_addr, boot_data.initrd_size);
    boot_data.initrd_start = new_initrd_addr;

    struct memory_range reserved = {
        .addr = KERNEL_START,
        .size = (boot_data.initrd_start + boot_data.initrd_size) - KERNEL_START
    };
    ret = memory_map_reserve_initial_memory(&reserved, 1);
    if (ret)
        kpanic("Inital memory map reservation failed %i\n", ret);

    // Enter kernel main
    kernel_main(&boot_data);
}
