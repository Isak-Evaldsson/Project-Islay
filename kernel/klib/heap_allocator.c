#include <arch/paging.h>
#include <klib/klib.h>
#include <memory/vmem_manager.h>
#include <stdalign.h>
#include <stdbool.h>

/* Eanables logging and extra extra validations of the heap data structure to simplify debugging */
#define DEBUG_HEAP_ALLOCATOR 1

/*
    Disable/enable ptr input validation to get better error messages with the cost of slightly
    bigger tags
*/
#define PTR_VALIDATION 1

/* Heap logging macro */
#if DEBUG_HEAP_ALLOCATOR
#define LOG(...) log("[HEAP_ALLOCATOR]: " __VA_ARGS__)
#else
#define LOG(...)
#endif

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/*
    The minimal size for each heap segment
*/
#define NPAGES_PER_SEGMENT (16)
#define MIN_ALLOC          (NPAGES_PER_SEGMENT * PAGE_SIZE)

/*
    Magic number flags
*/
#define MAGIC 0xc001c0de  // Allows us to check if free/realloc is provided with a valid pointer
#define DEAD  0xdeadbeef  // Allows us to catch double frees

/* Ensures correct alignment for the specific architecture  */
#define ALIGNMENT (alignof(max_align_t))

/* The total size for the tags */
#define TAGS_SIZE (sizeof(start_tag_t) + sizeof(end_tag_t))

/*
    Tag access macros
*/

/* Remove allocation bit form size value */
#define CLEAR_ALLOC_BIT(value) ((value) & ~(0x01))

/* Make sure the allocated bit is clear when getting the size */
#define GET_SIZE(tag_ptr) (CLEAR_ALLOC_BIT(tag_ptr->size))

/* Gets a pointer to the start tag from an free list entry */
#define GET_START_TAG(list_entry) ((start_tag_t*)((uintptr_t)list_entry - sizeof(start_tag_t)))

/* Gets a pointer to the end tag from a start tag */
#define GET_END_TAG(start_tag, size) \
    ((end_tag_t*)((uintptr_t)start_tag + CLEAR_ALLOC_BIT(size) - sizeof(end_tag_t)))

#define GET_START_TAG_FROM_END(end_tag) \
    ((start_tag_t*)((uintptr_t)(end_tag + 1) - GET_SIZE(end_tag)))

/* Marco to verify correctly built free blocks */
#ifdef PTR_VALIDATION
#define VERIFY_FREE_BLOCK(start, end)          \
    {                                          \
        kassert(((start)->size & 0x01) == 0);  \
        kassert(((end)->size & 0x01) == 0);    \
        kassert((start)->size == (end)->size); \
        kassert((start)->magic == DEAD);       \
    }

#else
#define VERIFY_FREE_BLOCK(start, end)          \
    {                                          \
        kassert(((start)->size & 0x01) == 0);  \
        kassert(((end)->size & 0x01) == 0);    \
        kassert((start)->size == (end)->size); \
    }
#endif

/*
    Implementation details:
    This is a linked list heap allocator with an explicit free list, which use start/end tags to
    allow free block coalescing.

    How the heap i structured into a linked list of heap segments, where each continuos segment
    is divided like this:
    |--------------------------------------------------------|
    | boundary_tag | object | object | object | boundary_tag |
    |--------------------------------------------------------|

    Each heap object is structured into packages:
    |-----------|-----------------------------------------|---------|
    | start_tag | alignment-padding + data || free_list_t | end_tag |
    |-----------|-----------------------------------------|---------|

    And the boundary tags are simply regular tags mark allocated with size 0

    TODO/Possible improvements:
    * Change over allocation strategy form first fet to best fit
    * Implement a system of bucketing to speed the process of finding a best fit
    * Try to expand block if possible in realloc
    * Can we improve the fragmentation/locality looking over the free list insertions
    * If there exits multiple free segments, free some of them
*/

/*
    The heap tags
*/

typedef struct start_tag {
    size_t size;   // Lowest bit is set to 0 if non-free
#if PTR_VALIDATION
    size_t magic;  // Allows pointer input validation
#endif
} start_tag_t;

typedef struct end_tag {
    size_t size;  // Lowest bit is set to 0 if non-free
} end_tag_t;

typedef size_t boundary_tag_t;  // As for the rest of the tags, the first bit marks if free

/*
    Heap segments
*/
typedef struct heap_segment heap_segment_t;

struct heap_segment {
    heap_segment_t* next;
    heap_segment_t* prev;
    size_t          size;
};

/*
    Free list
*/
typedef struct free_list free_list_t;

struct free_list {
    free_list_t* prev;
    free_list_t* next;
    size_t       size;
};

/* Head of free list */
free_list_t* free_list = NULL;

/* The linked list of heap segments */
heap_segment_t* segments = NULL;

#if DEBUG_HEAP_ALLOCATOR
#define VERIFY_FREE_LIST() verify_free_list(__FILE__, __FUNCTION__, __LINE__)

// Debug function allowing us to dump the contents of the heap
static void dump_heap()
{
    size_t          i;
    heap_segment_t* seg;

    LOG("Dumping free list");
    for (free_list_t* entry = free_list; entry != NULL; entry = entry->next) {
        LOG("(%x) Prev: %x Next: %x Size: %u", entry, entry->prev, entry->next, entry->size);
    }

    LOG("Dumping heap segments");
    for (seg = segments, i = 0; seg != NULL; seg = seg->next, i++) {
        boundary_tag_t* seg_start = (boundary_tag_t*)(seg + 1);
        start_tag_t*    entry     = (start_tag_t*)(seg_start + 1);

        LOG("Segment %u of at %x of size %u:", i, seg, seg->size);
        while ((uintptr_t)entry < ((uintptr_t)seg + seg->size - sizeof(boundary_tag_t))) {
            bool       allocated = entry->size & 0x01;
            end_tag_t* end       = GET_END_TAG(entry, entry->size);

            LOG("Block at %x(%u) %x(%u) - Allocated: %u", entry, GET_SIZE(entry), end,
                GET_SIZE(end), allocated);
            entry = (start_tag_t*)((uintptr_t)entry + GET_SIZE(entry));
        }
    }
}

// Debug function ensuring that the free list is properly built
static void verify_free_list(const char* file, const char* function, unsigned int line)
{
    bool correct = true;

    for (free_list_t* entry = free_list; entry != NULL; entry = entry->next) {
        if (entry->next != NULL && entry->next->prev != entry) {
            LOG("Incorrect prev and next pointer for entry %x", entry);
            correct = false;
        }

        // Get tags
        start_tag_t* start = GET_START_TAG(entry);
        end_tag_t*   end   = GET_END_TAG(start, start->size);

        if ((start->size & 0x01) != 0) {
            LOG("start tag %x is marked 0x01, i.e. allocated", start);
            correct = false;
        }

        if ((end->size & 0x01) != 0) {
            LOG("end tag %x is marked 0x01, i.e. allocated", end);
            correct = false;
        }

        if (start->size != end->size) {
            LOG("start tag %x and end tag %x of different size", start, end);
            correct = false;
        }

        if (start->magic != DEAD) {
            LOG("start tag %x not marked dead", start);
            correct = false;
        }
    }

    /* If heap is ill-formated, dump head data and panic */
    if (!correct) {
        dump_heap();
        kpanic("%s():%s:%u: incorrectly formated heap!!!\nSee heap dump in logs", function, file,
               line);
    }

    return; /* Success */
}

#else
#define VERIFY_FREE_LIST()
#endif /* DEBUG_HEAP_ALLOCATOR */

/*
    Free list helper functions
*/
static void insert_entry_after(free_list_t* previous, free_list_t* entry)
{
    entry->next    = previous->next;
    entry->prev    = previous->prev;
    previous->next = entry;
}

static void unlink_entry(free_list_t* entry)
{
    // Handle head of list
    if (entry == free_list) {
        free_list = entry->next;
    }

    // Change next nodes pointer if entry isn't the last node
    if (entry->next != NULL) {
        entry->next->prev = entry->prev;
    }

    // Change the previous nodes pointer if not the first node
    if (entry->prev != NULL) {
        entry->prev->next = entry->next;
    }
}

static void replace_node(free_list_t* old, free_list_t* new)
{
    new->next = old->next;
    new->prev = old->prev;

    if (old == free_list) {
        free_list = new;
    }

    if (old->prev != NULL) {
        old->prev->next = new;
    }

    if (old->next != NULL) {
        old->next->prev = new;
    }
}

static void append_heap_segment(heap_segment_t* segment)
{
    // We should never append to empty list
    kassert(segments != NULL);

    heap_segment_t* seg;

    // Find last segment in list
    for (seg = segments; seg->next != NULL; seg = seg->next)
        ;

    // append our new segment to it
    seg->next     = segment;
    segment->prev = seg;
    segment->next = NULL;
}

static free_list_t* create_entry_for_segment(heap_segment_t* segment)
{
    boundary_tag_t* heap_area  = (boundary_tag_t*)(segment + 1);
    start_tag_t*    list_entry = (start_tag_t*)(heap_area + 1);
    free_list_t*    head       = (free_list_t*)(list_entry + 1);
    head->next                 = NULL;
    head->prev                 = NULL;
    head->size                 = GET_SIZE(list_entry);

    return head;
}

static heap_segment_t* alloc_heap_segment(size_t size)
{
    // Adjust the size to fit the boundary tags and segment header
    size_t header_size = 2 * sizeof(boundary_tag_t) + sizeof(heap_segment_t);
    size_t alloc_size  = ALIGN_BY_MULTIPLE(MAX(size + header_size, MIN_ALLOC), (8 * PAGE_SIZE));
    size_t n_8pages    = alloc_size / (8 * PAGE_SIZE);

    heap_segment_t* heap_seg = (heap_segment_t*)vmem_request_free_pages(FPO_CLEAR, n_8pages);
    if (heap_seg == NULL) {
        return NULL;
    }

    // Keep track of segment size
    heap_seg->size = alloc_size;
    heap_seg->next = NULL;
    heap_seg->prev = NULL;

    /* Insert our boundary tags */
    boundary_tag_t* heap_start = (boundary_tag_t*)(heap_seg + 1);
    boundary_tag_t* heap_end =
        (boundary_tag_t*)((uintptr_t)heap_seg + alloc_size - sizeof(boundary_tag_t));
    *heap_start = 0x01;
    *heap_end   = 0x01;

    /* Insert the free object between the boundaries */
    start_tag_t* start = (start_tag_t*)(heap_start + 1);
    end_tag_t*   end   = (end_tag_t*)(heap_end - 1);

    start->size = alloc_size - header_size;
    end->size   = alloc_size - header_size;

#ifdef PTR_VALIDATION
    start->magic = DEAD;
#endif

    VERIFY_FREE_BLOCK((start_tag_t*)(heap_start + 1), (end_tag_t*)(heap_end - 1));

    return heap_seg;
}

void* kmalloc(size_t size)
{
    LOG("kmalloc(%u)", size);

    if (size == 0) {
        return NULL;
    }

    // The total size need to have space for tags and alignment
    size_t total = (size + TAGS_SIZE + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

    // Init heap
    if (segments == NULL) {
        segments = alloc_heap_segment(total);
        if (segments == NULL) {
            return NULL;  // out of memory
        }

        // Place free list data after the start tag of the free block
        free_list_t* head = create_entry_for_segment(segments);
        free_list         = head;
    }

search_free_list:

    // Iterate over the free list
    for (free_list_t* entry = free_list; entry != NULL; entry = entry->next) {
        // Found our candidate
        if (entry->size >= total) {
            // Get tags
            start_tag_t* start = GET_START_TAG(entry);
            end_tag_t*   end   = GET_END_TAG(start, start->size);

            VERIFY_FREE_BLOCK(start, end);

            size_t space_left = entry->size - total;

            // The block can be splitted
            if (space_left > (TAGS_SIZE + ALIGNMENT)) {
                // Create our new block tags
                start_tag_t* new_start = (start_tag_t*)((uintptr_t)start + total);
                end_tag_t*   new_end   = GET_END_TAG(new_start, space_left);

                // Set them appropriately
                new_start->size = space_left;
                new_end->size   = space_left;

#ifdef PTR_VALIDATION
                new_start->magic = DEAD;
#endif
                // Re-adjust the original tags
                end         = GET_END_TAG(start, total);
                start->size = total;
                end->size   = total;

                // Check that the split is done correctly
                kassert((start_tag_t*)(end + 1) == new_start);
                VERIFY_FREE_BLOCK(start, end);
                VERIFY_FREE_BLOCK(new_start, new_end);

                // Add new object block to free list
                free_list_t* new_entry = (free_list_t*)(new_start + 1);
                new_entry->size        = space_left;
                insert_entry_after(entry, new_entry);
            }

            // Mark tags
            start->size |= 0x01;
            end->size |= 0x01;

            // ensure a correctly built free-list
            VERIFY_FREE_LIST();

#if PTR_VALIDATION
            // Allows us to detect correct pointers
            start->magic = MAGIC;
#endif
            return entry;
        }
    }

    // No free block of suitable since available, lets request a new one
    heap_segment_t* new_seg = alloc_heap_segment(total);
    if (new_seg == NULL) {
        return NULL;  // failed to request more memory
    }

    append_heap_segment(new_seg);

    // Prepend it to free list
    free_list_t* new_entry = create_entry_for_segment(new_seg);
    free_list_t* prev_head = free_list;

    free_list       = new_entry;
    new_entry->next = prev_head;

    if (prev_head != NULL) {
        prev_head->prev = new_entry;
    }

    // Re-do search with our new block
    goto search_free_list;

    // should not be reached
    kassert(false);
    return NULL;
}

void kfree(void* ptr)
{
    LOG("kfree(%x)", ptr);

    if (ptr == NULL) {
        return;
    }

    // Compute tags
    volatile start_tag_t* start = GET_START_TAG(ptr);
    volatile end_tag_t*   end   = GET_END_TAG(start, start->size);

#if PTR_VALIDATION
    // Input validation
    if (start->magic == DEAD) {
        kpanic("free(): 0x%x was free'd twice\n", ptr);
    }

    if (start->magic != MAGIC) {
        kpanic("free(): invalid pointer 0x%x\n", ptr);
    }
#endif

    // Clear allocation bit
    start->size ^= 0x01;
    end->size ^= 0x01;

    // Get the sounding blocks
    start_tag_t* next_block_start = (start_tag_t*)(end + 1);
    end_tag_t*   prev_block_end   = (end_tag_t*)((uintptr_t)start - sizeof(end_tag_t));
    end_tag_t*   next_block_end   = GET_END_TAG(next_block_start, next_block_start->size);

    // Can we merge this block with the previous one (+ the next one as well)
    if ((prev_block_end->size & 0x01) == 0) {
        // Find our start tag from the end tag
        start_tag_t* prev_entry_start = GET_START_TAG_FROM_END(prev_block_end);
        free_list_t* prev_entry       = (free_list_t*)(prev_entry_start + 1);

        // Resize our new tags (previous start + the original end tag):
        size_t new_size = prev_entry_start->size + start->size;

        // Can we also merge in the next block
        if ((next_block_start->size & 0x01) == 0) {
            new_size += next_block_start->size;
            end = next_block_end;

            free_list_t* next_entry = (free_list_t*)(next_block_start + 1);
            unlink_entry(next_entry);
        }

        prev_entry_start->size = new_size;
        end->size              = new_size;

        // The merge changes the start pointer
        start = prev_entry_start;

        // Since we merge an already free block the linked list entry can be reused, but with a new
        // size
        prev_entry->size = new_size;

        // If we can't merge prev, can merge next only?
    } else if ((next_block_start->size & 0x01) == 0) {
        // Resize our new tags (original start + next end tag):
        size_t new_size = start->size + next_block_start->size;

        start->size          = new_size;
        next_block_end->size = new_size;

        // This merge changes the end pointer
        end = next_block_end;

        // Requires new free_list_entry
        free_list_t* next_entry = (free_list_t*)(next_block_start + 1);
        free_list_t* entry      = (free_list_t*)(start + 1);

        entry->size = new_size;

        // Replace our next block list entry, with the new one
        replace_node(next_entry, entry);

        // No merging is possible, i.e.insert new free list entry
    } else {
        // TODO: Address sorted insertion may yield better fragmentation
        free_list_t* entry = ptr;
        if (free_list != NULL) {
            free_list->prev = entry;
        }

        entry->prev = NULL;
        entry->next = free_list;
        entry->size = GET_SIZE(start);
        free_list   = entry;
    }

#if PTR_VALIDATION
    // Marks our free pointer, allow us to detect double frees
    start->magic = DEAD;
#endif

    // Make sure our free block is built correctly
    VERIFY_FREE_BLOCK(start, end);

    // ensure a correctly built free-list
    VERIFY_FREE_LIST();
}

void* kcalloc(size_t num, size_t size)
{
    LOG("kcalloc(%u, %u)", num, size);
    void* ret;

    ret = kmalloc(num * size);

    // Only clear on success
    if (ret != NULL) {
        memset(ret, 0, num * size);
    }
    return ret;
}

void* krealloc(void* ptr, size_t new_size)
{
    LOG("krealloc(%x, %u)", ptr, new_size);

    if (ptr == NULL) {
        return kmalloc(new_size);
    }

    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    void*        ret_ptr = ptr;  // The pointer to be returned
    start_tag_t* start   = GET_START_TAG(ptr);
    end_tag_t*   end     = GET_END_TAG(start, start->size);

#if PTR_VALIDATION
    // Input validation
    if (start->magic == DEAD) {
        kpanic("%s(): trying to realloc dead pointer 0x%x\n", __FUNCTION__, ptr);
    }

    if (start->magic != MAGIC) {
        kpanic("%s(): invalid pointer 0x%x\n", __FUNCTION__, ptr);
    }
#endif

    // The actual amount of space available for the pointer
    size_t size = (uintptr_t)end - (uintptr_t)ptr;

    // The pointer needs to be expanded
    if (new_size > size) {
        // TODO: check if the next block is free and of sufficient size before allocating a new
        // pointer

        ret_ptr = kmalloc(new_size);
        if (ret_ptr != NULL) {
            memcpy(ret_ptr, ptr, size);
        }
        kfree(ptr);
    }

    return ret_ptr;
}
