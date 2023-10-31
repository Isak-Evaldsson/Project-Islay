#include <arch/arch.h>
#include <arch/paging.h>
#include <utils.h>

// asm function to handle TLB invalidation
extern void tlb_invalid_page(void *addr);

// Kernel boot page directory
extern uint32_t boot_page_directory[];

void map_page(physaddr_t physaddr, virtaddr_t virtaddr, uint16_t flags)
{
    uint32_t dir_index   = virtaddr >> 22;  // The 10 greatest bits yields our directory index.
    uint32_t table_index = virtaddr >> 12 & 0x03FF;  // Table index is stored in bits 22-12

    // Find which line in the page directory table to follow
    uint32_t page_dir_entry = boot_page_directory[dir_index];
    if (page_dir_entry == 0) {
        kpanic("Writing to no existing PDT entry not implemented");
    }

    // Clears the lowest 12 bits befor converting to virtual address
    // Assumes the page table address to be linearly mapped
    uint32_t *page_table = (uint32_t *)((page_dir_entry & ~0xfff) + HIGHER_HALF_ADDR);
    if (page_table[table_index] != 0) {
        kpanic("Handle page table overwrite");
    }

    // Insert out address + flags into the correct page table entry:
    page_table[table_index] = physaddr | (flags & 0xfff) | 0x01;

    // Make sure page table changes propagates pack to TLB
    tlb_invalid_page((void *)virtaddr);
}

void unmap_page(virtaddr_t virtaddr)
{
    uint32_t dir_index   = virtaddr >> 22;  // The 10 greatest bits yields our directory index.
    uint32_t table_index = virtaddr >> 12 & 0x03FF;  // Table index is stored in bits 22-12

    uint32_t page_dir_entry = boot_page_directory[dir_index];
    if (page_dir_entry == 0) {
        kpanic("Trying to unmap vaddr within an non-existing page table");
    }

    // Clears the lowest 12 bits befor converting to virtual address
    // Assumes the page table address to be linearly mapped
    uint32_t *page_table = (uint32_t *)((page_dir_entry & ~0xfff) + HIGHER_HALF_ADDR);
    if (page_table[table_index] == 0) {
        kpanic("Trying to unmap already unmapped virtual address");
    }

    page_table[table_index] = 0;

    // Make sure page table changes propagates pack to TLB
    tlb_invalid_page((void *)virtaddr);
};
