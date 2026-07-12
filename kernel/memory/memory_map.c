/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2026 Isak Evaldsson
*/
#include <arch/paging.h>
#include <list.h>
#include <stdint.h>
#include <uapi/errno.h>
#include <utils.h>

#include "internal.h"

#define MAX_MEM_AREAS 64

#define LOG(fmt, ...) __LOG(1, "[MEMORY_MAP]", fmt, ##__VA_ARGS__)

enum mem_map_state {
    MEM_MAP_STATE_UNINITALIZED, // Data structures are not yet fully initialized
    MEM_MAP_STATE_INITALIZED,   // Memory map is not fully built, no memory can be allocated
    MEM_MAP_STATE_COMPLETE,     // Memory map is built, its safe to start allocate memory
    MEM_MAP_STATE_LOCKED,       // The page allocator has taken control of all free memory
};

enum memory_area_type {
    MEM_TYPE_FREE,
    MEM_TYPE_BIOS_RESERV,
    MEM_TYPE_KERNEL_RESERV,
    MEM_TYPE_MAX,
};

struct memory_map_entry {
    enum memory_area_type type;
    struct memory_range range;
    struct list_entry entry;
};

struct memory_map {
    size_t total_size;
    size_t avail_size;
    physaddr_t last_avail_addr;
    enum mem_map_state state;

    // All memory map entires sorted in order
    struct list entries;

    // All map entries are pre-allocated since we don't know where to allocate
    // dynamic memory until the memory map is fully set up
    struct memory_map_entry entry_pool[MAX_MEM_AREAS];
    size_t next_free_entry;
    struct list free_list;
};

static struct memory_map map;

static struct memory_map_entry *alloc_map_entry(struct memory_map *map)
{
    struct list_entry *list_entry;

    list_entry = list_remove_first(&map->free_list);
    if (list_entry)
        return GET_STRUCT(struct memory_map_entry, entry, list_entry);

    if (map->next_free_entry >= COUNT_ARRAY_ELEMS(map->entry_pool))
        return NULL;

    return &map->entry_pool[map->next_free_entry++];
}

static void free_map_entry(struct memory_map *map, struct memory_map_entry *entry)
{
    memset(entry, 0, sizeof(entry));
    list_add_last(&map->free_list, &entry->entry);
}

static physaddr_t range_end(struct memory_range *range)
{
    physaddr_t end = range->addr + range->size;

    // Might overflow the range express the last part of memory
    if (range->size && !end)
        end = UINTPTR_MAX;

    return end;
}

static int add_entry(struct memory_map *map, struct memory_range *range,
        enum memory_area_type type)
{
    bool extended, found, add_after;
    physaddr_t entry_end, end;
    struct memory_map_entry *entry, *new, *next;
    physaddr_t address = range->addr;
    size_t size = range->size;

    if (!range || !size || type >= MEM_TYPE_MAX)
        return -EINVAL;

    if (LIST_EMPTY(&map->entries)) {
        new = alloc_map_entry(map);
        if (!new)
                return -ENOMEM;

        new->range = *range;
        new->type = type;
        list_add_first(&map->entries, &new->entry);
        return 0;
    }

    found = false;
    extended = false;
    end = range_end(range);

    // Find the first existing entry which lies after the new one,
    // or rather that ends after the new one begins
    LIST_ITER_STRUCT(&map->entries, entry, struct memory_map_entry, entry) {
        entry_end = range_end(&entry->range);
        if (entry_end < address)
            continue;

        // if the new range lies right after current entry, it might be possible to extend it
        if (entry_end == address) {
            // But first we must compare with next to avoid overlaps
            if (LIST_HAS_NEXT(&map->entries, &entry->entry)) {
                next = GET_STRUCT(struct memory_map_entry, entry, entry->entry.next);
                if (next->range.addr < end)
                    return -EINVAL; // Overlap

                // Can next be extend as well?
                if (end == next->range.addr && next->type == type) {
                    next->range.addr = address;
                    next->range.size += size;
                    extended = true;
                }
            }

            // Given the correct type, its time to coalesce
            if (entry->type == type) {
                if (extended) {
                    // already extend, the next element is no longer needed
                    entry->range.size += next->range.size;
                    list_entry_remove(&next->entry);
                    free_map_entry(map, next);
                } else {
                    entry->range.size += size;
                }
                return 0;
            }

            if (extended)
                return 0;

            // Could not coalesce, so allocate a new range and add after current
            // since entry_end == address
            add_after = true;
            found = true;
            break;
        }

        // entry_end > address, but there can still be overlaps if it begins before the new ends
        if (end > entry->range.addr)
            return -EINVAL;

        // if the new range lies right before current entry, it might be possible to extend it
        if (end == entry->range.addr && type == entry->type) {
            entry->range.addr = address;
            entry->range.size += size;
            return 0;
        }

        // Could not coalesce, so allocate a new range and add before current
        // since entry_end > address
        add_after = false;
        found = true;
        break;
    }

    // Since the loop exited, the new range could not be coalesced with any current entires,
    // or all current entires end before the new starts. Either way, a new entry needs to be
    // allocated and added to the list
    new = alloc_map_entry(map);
    if (!new)
        return -ENOMEM;

    new->range = *range;
    new->type = type;

    if (found)
        if (add_after)
            list_entry_append_single_element(&entry->entry, &new->entry);
        else
            list_entry_prepend_single_element(&entry->entry, &new->entry);
    else
        list_add_last(&map->entries, &new->entry);

    return 0;
}

static void memory_map_dump(const char *msg)
{
    struct memory_map_entry *entry;

    LOG("%s:", msg);
    LIST_ITER_STRUCT(&map.entries, entry, struct memory_map_entry, entry) {
        LOG("\taddr: %x, size %u, type %u", entry->range.addr, entry->range.size,
                entry->type);
    }
}

static int split_entry(struct memory_map *map, struct memory_map_entry *entry,
        struct memory_range *hole, enum memory_area_type type)
{
    int ret;
    physaddr_t hole_end = range_end(hole), entry_end = range_end(&entry->range);

    // The hole is in the beging of the entry
    if (entry->range.addr == hole->addr)
    {
        // If it covers the full entry, just change its type
        if (entry_end == hole_end) {
            entry->type = type;
            return 0;
        }

        // Otherwise re-adjust the existing entry
        entry->range.addr = hole_end;
        entry->range.size -= hole->size;

        return add_entry(map, hole, type);
    }

    // If the hole is in the middle of the entry,
    // re-adjust the current entry so it lies before the hole
    entry->range.size = hole->addr - entry->range.addr;

    ret = add_entry(map, hole, type);
    if (ret)
        return ret;

    // If the hole doesn't go to the end, add a new after after it
    if (hole_end < entry_end)
        return add_entry(map, &(struct memory_range){hole_end, entry_end - hole_end}, entry->type);

    return 0;
}

int memory_map_add_entry(physaddr_t address, size_t size, bool free)
{
    int ret;
    struct memory_range range = { .addr = address, .size = size };
    physaddr_t end = range_end(&range);

    if (map.state == MEM_MAP_STATE_UNINITALIZED) {
        list_init(&map.free_list);
        list_init(&map.entries);
        map.state = MEM_MAP_STATE_INITALIZED;
    } else if (map.state != MEM_MAP_STATE_INITALIZED)
        return -EBUSY;

    ret = add_entry(&map, &range, free ? MEM_TYPE_FREE : MEM_TYPE_BIOS_RESERV);
    if (!ret) {
        map.total_size += size;
        if (free) {
            map.avail_size += size;
            if (end > map.last_avail_addr)
                map.last_avail_addr = end - 1;
        }
    }
    return ret;
}

int memory_map_allocate_pages(physaddr_t *ptr, physaddr_t start, physaddr_t end, size_t count)
{
    int ret;
    struct memory_range range;
    struct memory_map_entry *entry;
    physaddr_t entry_start, offset;
    size_t size = PAGE_SIZE * count;
    bool found = false;

    if (map.state != MEM_MAP_STATE_COMPLETE)
        return -EBUSY;

    // Allocation impossible if the allowed range is smaller than the requested size
    if ((start - end) < size)
        return -EINVAL;

    // Find a free entry that fits the requested size and is within [start, end]
    LIST_ITER_STRUCT(&map.entries, entry, struct memory_map_entry, entry) {
        if (entry->type != MEM_TYPE_FREE)
            continue;

        // Might need to begin at an offset to satisfy the range requirement
        entry_start = entry->range.addr;
        offset = entry_start < start ? start - entry_start : 0;
        if (offset > entry->range.size)
            continue;

        if ((offset + entry_start) % PAGE_SIZE)
            offset = ALIGN_BY_MULTIPLE(entry_start + offset, PAGE_SIZE) - entry_start;

        // The entry must be big enough and the offset should not make the allocation
        // go out of range
        if ((entry->range.size - offset) >= size && (offset + size) <= end) {
            found = true;
            break;
        }
    }

    if (!found)
        return -ENOMEM;

    range.addr = entry->range.addr + offset;
    range.size = size;

    ret = split_entry(&map, entry, &range, MEM_TYPE_KERNEL_RESERV);
    if (ret)
        return ret;

    *ptr = range.addr;
    return 0;
}

int memory_map_reserve_initial_memory(struct memory_range reservations[], size_t count)
{
    int ret;
    struct memory_map_entry *entry;
    struct memory_range *reservation;
    physaddr_t entry_start, entry_end, end;
    bool found = false;

    if (map.state != MEM_MAP_STATE_INITALIZED)
        return -EBUSY;

    for (size_t i = 0; i < count; i++) {
        reservation = reservations + i;

        // page align to avoid fragmentation
        reservation->size = ALIGN_BY_PAGE_SIZE(reservation->size);
        end = range_end(reservation);

        // Find free entry containg the range to be reserved
        LIST_ITER_STRUCT(&map.entries, entry, struct memory_map_entry, entry) {
            if (entry->type != MEM_TYPE_FREE)
                continue;

            entry_start = entry->range.addr;
            entry_end = range_end(&entry->range);
            if (entry_start <= reservation->addr && entry_end >= end) {
                found = true;
                break;
            }
        }

        if (!found)
            return -ENOMEM;

        ret = split_entry(&map, entry, reservation, MEM_TYPE_KERNEL_RESERV);
        if (ret)
            return ret;
    }

    memory_map_dump("Memory map complete");
    map.state = MEM_MAP_STATE_COMPLETE;
    return 0;
}

int memory_map_init_page_frame_allocator()
{
    int ret;
    physaddr_t prev_end;
    struct memory_map_entry *entry;
    struct list_entry *list_entry;

    if (map.state != MEM_MAP_STATE_COMPLETE)
        return -EBUSY;

    if (!map.last_avail_addr)
        return -ENOMEM;

    list_entry = list_get_first(&map.entries);
    if (!list_entry)
        return -ENOMEM;

    // To avoid bugs due to assumptions that the zero pages represents NULL,
    // reserve it even if it's available
    entry = GET_STRUCT(struct memory_map_entry, entry, list_entry);
    if (entry->range.addr == 0 && entry->type == MEM_TYPE_FREE) {
        struct memory_range first_page = {
            .addr = 0,
            .size = PAGE_SIZE
        };

        ret = split_entry(&map, entry, &first_page, MEM_TYPE_KERNEL_RESERV);
        if (ret)
            return ret;
    }

    ret = page_frame_manager_prepare(map.last_avail_addr);
    if (ret)
        return ret;

    prev_end = 0;
    LIST_ITER_STRUCT(&map.entries, entry, struct memory_map_entry, entry) {
        // Ensure that a vaild memory map is give to the page frame alloctor
        // all entries should be ordered and non-overlapping
        kassert(prev_end <= entry->range.addr);
        prev_end = range_end(&entry->range);

        if (entry->type != MEM_TYPE_FREE)
            continue;

        ret = page_frame_manger_mark_free(&entry->range);
        if (ret)
            return ret;
    }

    memory_map_dump("Handing over all available memory to the page frame allocator");
    return 0;
}
