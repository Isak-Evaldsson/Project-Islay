#ifndef MEMORY_PAGING_H
#define MEMORY_PAGING_H

/*
    Common defintions and macro used within the kernel memory management code
*/

#define PAGE_SIZE (4096)

// Rounds up a number to a multiple of n, assuming n is a factor of 2
#define ALIGN_BY_MULTIPLE(num, n) (((num) + ((n)-1)) & ~((n)-1))

// Ensures the num is aligned by page size
#define ALIGN_BY_PAGE_SIZE(num) ALIGN_BY_MULTIPLE(num, PAGE_SIZE)

#endif /* MEMORY_PAGING_H */
