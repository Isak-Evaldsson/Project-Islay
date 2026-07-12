/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2026 Isak Evaldsson
*/
#ifndef MEMORY_MEMORY_MAP_H
#define MEMORY_MEMORY_MAP_H

#include <arch/arch.h>
#include <stdbool.h>

/*
 * The memory maps track all memory available to the system. It's built during boot and allows
 * the kernel to reserve/allocate memory before the page frame allocator is up and running.
 */

struct memory_range {
    physaddr_t addr;
    size_t size;
};

/*
 * Memory map construction API, when parsing the bios memory map add new entries using
 * memory_map_add_entry(). Once done, lock the memory map with memory_map_reserve_initial_memory().
 * By providing a list of initial reservations, once can make sure that pre-loaded data such as the
 * kernel binary or initrd are properly reserved.
 */
int memory_map_add_entry(physaddr_t address, size_t size, bool free);
int memory_map_reserve_initial_memory(struct memory_range reservations[], size_t count);

/* Hands over the all the free memory that is left to page frame allocator. */
int memory_map_init_page_frame_allocator();

/* Allocates pages from the memory map, can only be called after the memory map is locked in. */
int memory_map_allocate_pages(physaddr_t *ptr, physaddr_t start, physaddr_t end, size_t count);

/*
 * Permanently allocates memory through the memory map, useful for dynamically allocations that
 * needs to be done before the page frame manager is up and running. Note, once allocted this
 * memory can *never* be free'd, so use it with care.
 */
void *bootmem_alloc(size_t size);

#endif /* MEMORY_MEMORY_MAP_H */
