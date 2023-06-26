#ifndef ARCH_PAGING_H
#define ARCH_PAGING_H
#include <arch/arch.h>
#include <arch/boot.h>

// Architecture dependent paging properties
#if ARCH(i386)
#define PAGE_SIZE (4096)
#else
#error "Unkown architecture"
#endif

// Macros to covert between phyiscal and logical address
#define P2L(paddr) ((paddr) + HIGHER_HALF_ADDR)
#define L2P(laddr) ((laddr)-HIGHER_HALF_ADDR)

// Rounds up a number to a multiple of n, assuming n is a factor of 2
#define ALIGN_BY_MULTIPLE(num, n) (((num) + ((n)-1)) & ~((n)-1))

// Ensures the num is aligned by page size
#define ALIGN_BY_PAGE_SIZE(num) ALIGN_BY_MULTIPLE(num, PAGE_SIZE)

#define PAGE_OPTION_WRITABLE (2)

/* Paging functions required to be implemented for each architecture */
void map_page(physaddr_t physaddr, virtaddr_t virtaddr, uint16_t flags);
void unmap_page(virtaddr_t virtaddr);

#endif /* ARCH_PAGING_H */