#ifndef MEMORY_PAGE_FRAME_MANAGER_H
#define MEMORY_PAGE_FRAME_MANAGER_H
#include <arch/arch.h>
#include <stddef.h>
#include <stdint.h>

/*
    Page frame manger - responsible for the management of physical memory frames
*/

/*
    Allocation options
*/
#define PF_OPT_HIGH_MEM (1 << 0)  // First bit indicates request allocate high memory

// Struct holding memory statistics provided by the page frame manager
typedef struct memory_stats {
    size_t memory_amount;
    size_t n_frames;
    size_t n_available_frames;
} memory_stats_t;

// Initialise the page frame manager based on the supplied memory map
void page_frame_manager_init(struct boot_data *boot_data);

// Returns memory statistics from the page frame manager
void page_frame_manger_memory_stats(memory_stats_t *stats);

// Returns physical address to the page that was allocated, 0 marks failure
physaddr_t page_frame_alloc_page(uint8_t options);

// Allocations 8 * n pages, returns physical address to the first page that was allocated, 0 marks
// failure
physaddr_t page_frame_alloc_pages(uint8_t options, unsigned int n);

// Frees the segment of 8 * n pages starting at the supplied physical address, to free a single page
// set n = 0.
void page_frame_free(physaddr_t addr, unsigned int n);

#endif /* MEMORY_PAGE_FRAME_MANAGER_H */
