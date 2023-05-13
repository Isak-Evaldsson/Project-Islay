#ifndef MEMORY_PAGE_FRAME_MANAGER_H
#define MEMORY_PAGE_FRAME_MANAGER_H
#include <stddef.h>
#include <stdint.h>

/*
    Page frame manger - responsible for the management of physical memory frames
*/

// Memory segment in memory map
typedef struct memory_segment {
    uint32_t addr;
    uint32_t length;
} memory_segment_t;

// Architecture independet memory map
typedef struct memory_map {
    size_t            memory_amount;
    size_t            n_segments;  // Array size
    memory_segment_t *segments;    // Array of memory segment
} memory_map_t;

// Struct holding memory statistics provided by the page frame manager
typedef struct memory_stats {
    size_t memory_amount;
    size_t n_frames;
    size_t n_available_frames;
} memory_stats_t;

// Initialise the page frame manager based on the supplied memory map
void page_frame_manager_init(memory_map_t *mmap);

// Returns memory statistics from the page frame manager
void page_frame_manger_memory_stats(memory_stats_t *stats);

// Returns physical address to the page that was allocated, 0 marks failure
uint32_t page_frame_alloc();

// Frees the page starting at the supplied physical address
void page_frame_free(uint32_t addr);

#endif /* MEMORY_PAGE_FRAME_MANAGER_H */
