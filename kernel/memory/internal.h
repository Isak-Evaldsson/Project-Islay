/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2026 Isak Evaldsson
*/
#ifndef MEMORY_INTERNAL_H
#define MEMORY_INTERNAL_H

#include <memory/memory_map.h>

/* Hooks into the page frame allocator used by the memory map */
int page_frame_manager_prepare(physaddr_t last_free_addr);
int page_frame_manger_mark_free(struct memory_range *range);

#endif /* MEMORY_INTERNAL_H */