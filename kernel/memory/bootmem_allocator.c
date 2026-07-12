/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2026 Isak Evaldsson
*/
#include <arch/paging.h>
#include <memory/memory_map.h>
#include <stdalign.h>

// To have working linear mappings, the memory needs to be keept in following range
#define BOOTMEM_START   (0)
#define BOOTMEM_END     (VIRTADDR_MAX - HIGHER_HALF_ADDR)

static void *addr;
static size_t addr_offset;

static void *memmap_alloc_page(size_t count)
{
    int ret;
    physaddr_t paddr;

    ret = memory_map_allocate_pages(&paddr, BOOTMEM_START, BOOTMEM_END, count);
    if (ret)
        return NULL;

    kassert(!(paddr % PAGE_SIZE));
    for (physaddr_t offset = 0; offset < PAGE_SIZE * count; offset += PAGE_SIZE)
        map_page(paddr + offset, P2L(paddr + offset), PAGE_OPTION_WRITABLE);

    return (void*)P2L(paddr);
}

void *bootmem_alloc(size_t size)
{
    void *ptr;

    if (size >= PAGE_SIZE)
        return memmap_alloc_page(ALIGN_BY_PAGE_SIZE(size) / PAGE_SIZE);

    // To ensure that we always return aligned addresses, round up to max alignment
    size = ALIGN_BY_MULTIPLE(size, alignof(max_align_t));

    if (!addr || ((PAGE_SIZE - addr_offset) < size)) {
        addr = memmap_alloc_page(1);
        if (!addr)
            return NULL;

        addr_offset = 0;
    }

    ptr = addr + addr_offset;
    addr_offset += size;
    return ptr;
}
