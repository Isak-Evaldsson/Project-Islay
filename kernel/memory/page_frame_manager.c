#include <arch/boot.h>
#include <klib/klib.h>
#include <memory/page_frame_manager.h>
#include <stdbool.h>

/*
    Page frame manger - responsible for the management of physical memory frames
*/

#define PAGE_SIZE (4096)

#define FRAME_NUMBER(addr) ((addr) / (PAGE_SIZE))
#define BITMAP_INDX(fnum)  ((fnum) >> 3)  // Division by 8
#define BITMAP_BIT(fnum)   ((fnum) % 8)

// Rounds up a number to a multiple of n, assuming n is a factor of 2
#define ALIGN_BY_MULTIPLE(num, n) (((num) + ((n)-1)) & ~((n)-1))

// Ensures the num is aligned by page size
#define ALIGN_BY_PAGE_SIZE(num) ALIGN_BY_MULTIPLE(num, PAGE_SIZE)

#define SET_BIT(b, n)  ((b) |= (1 << (n)))
#define CLR_BIT(b, n)  ((b) &= ~((1) << (n)))
#define MASK_BIT(b, n) (b & (1 << n))

// Bitmap marking available page frames with a 1, not that fast (O(N) allocation) or space efficient
// (always full 4 GiB bitmap no matter actual ram size). A simple solution for 32-bit systems, but
// unfeasible for 64 bits
static unsigned char memory_bitmap[131072];

// Speeds up the bitmap search procedure by not always beginning at index 0
static uint32_t first_available_frame_idx = 0;

// Allows some basic memory usage statistic
static size_t n_available_frames = 0;
static size_t amount_of_memory   = 0;
static size_t n_frames           = 0;

/*
    Internal data structure dependent functions
*/

// Gets the availability of a specific page
static bool check_page_available(uint32_t page_num)
{
    unsigned char idx = BITMAP_INDX(page_num);
    unsigned char bit = BITMAP_BIT(page_num);
    return MASK_BIT(memory_bitmap[idx], bit);
}

// Marks page at page_num available/unavailable
static void mark_page(uint32_t page_num, bool available)
{
    unsigned char idx = BITMAP_INDX(page_num);
    unsigned char bit = BITMAP_BIT(page_num);

    if (available) {
        SET_BIT(memory_bitmap[idx], bit);
        n_available_frames++;

        // start next search at the free'ed page if it has a lower index
        if (idx < first_available_frame_idx) {
            first_available_frame_idx = idx;
        }
    } else {
        CLR_BIT(memory_bitmap[idx], bit);
        n_available_frames--;
    }
}

// Performs a linear search through the bitmap finding the first available page
static uint32_t find_available_page()
{
    // Increase speed by not starting at zero every time
    size_t i = first_available_frame_idx & ~(0b11);  // round down to multiple of 4

    // Increase speed by searching 4 bytes at the time
    for (; i < sizeof(memory_bitmap); i += 4) {
        uint32_t bytes = *(uint32_t *)(memory_bitmap + i);
        if (bytes > 0) {
            // Once a suitable 4-bytes sequence is found, search all the bits for an available page
            for (size_t j = 0; j < 32; j++) {
                if (MASK_BIT(bytes, j)) {
                    // save the index to speed up future searches
                    first_available_frame_idx = i;

                    return i * 8 + j;
                }
            }
        }
    }
    return 0;  // Marks unsuccess
}

// Allows the init function to mark bigger memory segments
static void init_mark_segment(size_t addr, size_t length, bool available)
{
    // Verify that addr and length is a multiple of PAGE_SIZE
    kassert(addr % PAGE_SIZE == 0);
    kassert(length % PAGE_SIZE == 0);

    // reset first available idx
    first_available_frame_idx = 0;

    uint32_t start_frame = FRAME_NUMBER(addr);
    uint32_t start_idx   = BITMAP_INDX(start_frame);
    uint8_t  start_bit   = BITMAP_BIT(start_frame);

    // Handle first byte if segment start in the middle of a byte
    if (start_bit != 0) {
        // Fill the bits for the relevant byte
        for (uint8_t i = start_bit; i < 8; i++) {
            if (available) {
                SET_BIT(memory_bitmap[start_idx], i);
                n_available_frames++;
            } else {
                CLR_BIT(memory_bitmap[start_idx], i);
                n_available_frames--;
            }
        }

        // Increment idx to point to first full bit
        start_idx++;
    }

    uint32_t end_frame = FRAME_NUMBER(addr + length);
    uint32_t end_idx   = BITMAP_INDX(end_frame);
    uint8_t  end_bit   = BITMAP_BIT(end_frame);

    // Fill all the bytes from start until end index
    for (uint32_t i = start_idx; i < end_idx; i++) {
        if (available) {
            memory_bitmap[i] = 0xff;
            n_available_frames += 8;
        } else {
            memory_bitmap[i] = 0x00;
            n_available_frames -= 8;
        }
    }

    // Handle if the segment ends in the middle of a byte, fill all bits less than it
    if (end_bit != 0) {
        for (uint8_t i = 0; i < end_bit; i++) {
            if (available) {
                SET_BIT(memory_bitmap[end_idx], i);
                n_available_frames++;
            } else {
                CLR_BIT(memory_bitmap[end_idx], i);
                n_available_frames--;
            }
        }
    }
}

/*
    Page frame manager api implementation
*/

// Initialise the page frame manager based on the supplied memory map
void page_frame_manager_init(memory_map_t *map)
{
    // 1: Initialise bitmap by setting all bytes to zero (i.e. unavailable)
    memset(memory_bitmap, 0, sizeof(memory_bitmap));

    // 2: Parse the supplied memory map, marking segments as available
    amount_of_memory = map->memory_amount;
    for (size_t i = 0; i < map->n_segments; i++) {
        memory_segment_t segment = map->segments[i];
        init_mark_segment(segment.addr, segment.length, true);
    }
    n_frames = n_available_frames;

    // 3: Mark kernel segments as unavailable
    size_t addr   = KERNEL_START;
    size_t length = KERNEL_END - HIGHER_HALF_ADDR - KERNEL_START;
    init_mark_segment(addr, ALIGN_BY_PAGE_SIZE(length), false);
}

// Returns memory statistics from the page frame manager
void page_frame_manger_memory_stats(memory_stats_t *stats)
{
    stats->memory_amount      = amount_of_memory;
    stats->n_available_frames = n_available_frames;
    stats->n_frames           = n_frames;
}

// Returns physical address to the page that was allocated, 0 marks failure
uint32_t page_frame_alloc()
{
    uint32_t page_num = find_available_page();
    if (page_num == 0) {
        return 0;
    }

    mark_page(page_num, false);
    return page_num * PAGE_SIZE;
}

// Frees the page starting at the supplied physical address
void page_frame_free(uint32_t addr)
{
    // Ensure that address in aligned correctly
    kassert(addr % PAGE_SIZE == 0);

    uint32_t page_num = FRAME_NUMBER(addr);
    if (check_page_available(page_num)) {
        kpanic("page_frame_free(): Double free at address 0x%x", addr);
    }

    mark_page(page_num, true);
}
