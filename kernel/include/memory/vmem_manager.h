/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef MEMORY_VMEM_MANAGER_H
#define MEMORY_VMEM_MANAGER_H
#include <arch/arch.h>
/*
    Virtual memory manager - responsible for management of virtual memory
*/

/*
    Request free page options (FPO), used to specific options when allocating physical or virtual
    pages
*/
#define FPO_HIGHMEM (1 << 0)  // If bit 0 is high, alloc high-memory
#define FPO_CLEAR   (1 << 1)  // If bit 1 is high, clear allocated pages

// Allocates a single page and returns its virtual address
virtaddr_t vmem_request_free_page(unsigned int fpo);

// Allocates a segment 8 * n pages and returns its virtual address. Does only grantee a
// continuos memory space when allocating low memory.
virtaddr_t vmem_request_free_pages(unsigned int fpo, unsigned int n);

// Frees virtual page with a given address
void vmem_free_page(virtaddr_t addr);

// Frees a 8 * n page segment starting a the given virtual address
void vmem_free_pages(virtaddr_t addr, unsigned int n);

#endif /* MEMORY_VMEM_MANAGER_H */
