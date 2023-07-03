#include <arch/boot.h>
#include <arch/paging.h>
#include <klib/klib.h>
#include <memory/page_frame_manager.h>
#include <memory/vmem_manager.h>

// Allocates a single page and returns its virtual address
virtaddr_t vmem_request_free_page(unsigned int fpo)
{
    if (MASK_BIT(fpo, FPO_HIGHMEM) == 1) {
        kpanic("Highmem not yet implemented");
    }

    physaddr_t physaddr = page_frame_alloc_page(0);
    if (physaddr == 0) {
        return 0;  // could not allocate page
    }

    // For low memory, logical addressing is used
    virtaddr_t virtaddr = P2L(physaddr);

    // Perform memory mapping
    map_page(physaddr, virtaddr, PAGE_OPTION_WRITABLE);

    // clear page if required
    if (MASK_BIT(fpo, FPO_CLEAR) == 1) {
        memset((void *)virtaddr, 0, PAGE_SIZE);
    }

    return virtaddr;
}

// Allocates a segment 8 * n pages and returns its virtual address. Does only grantee a
// continuos memory space when allocating low memory.
virtaddr_t vmem_request_free_pages(unsigned int fpo, unsigned int n)
{
    if (MASK_BIT(fpo, FPO_HIGHMEM) == 1) {
        kpanic("Highmem not yet implemented");
    }

    physaddr_t physaddr = page_frame_alloc_pages(0, n);
    if (physaddr == 0) {
        return 0;  // could not allocate page
    }

    // For low memory, logical addressing is used
    virtaddr_t virtaddr = P2L(physaddr);

    // Map all the pages
    size_t npages = n * 8;
    for (size_t i = 0; i < npages; i++) {
        map_page(physaddr + (i * PAGE_SIZE), virtaddr + (i * PAGE_SIZE), PAGE_OPTION_WRITABLE);
    }

    // clear pages if required
    if (MASK_BIT(fpo, FPO_CLEAR) == 1) {
        memset((void *)virtaddr, 0, PAGE_SIZE * npages);
    }

    return virtaddr;
}

// Frees virtual page with a given address
void vmem_free_page(virtaddr_t addr)
{
    // Assumes lowmem, TODO: check if low or highmem and thereby compute physaddr correctly
    physaddr_t paddr = L2P(addr);

    unmap_page(addr);
    page_frame_free(paddr, 0);
}

// Frees a 8 * n page segment starting a the given virtual address
void vmem_free_pages(virtaddr_t addr, unsigned int n)
{
    // Assumes lowmem, TODO: check if low or highmem and thereby compute physaddr correctly
    physaddr_t paddr = L2P(addr);

    size_t npages = n * 8;
    for (size_t i = 0; i < npages; i++) {
        unmap_page(addr + i * PAGE_SIZE);
    }

    page_frame_free(paddr, n);
}
