#include <arch/i386/segmentation/tss.h>
#include <klib/klib.h>

/* The internal variable allocating space for the kernel tss */
static tss_t internal_tss;

/* The exposed pointer, need to make it easier to access from assembly */
tss_t *kernel_tss = &internal_tss;

void init_kernel_tss()
{
    // clear structure
    memset(kernel_tss, 0, sizeof(tss_t));

    /*  Setup tss for software multitasking */
    kernel_tss->ssp  = 0x010;  // Kernel data segment gdt offset
    kernel_tss->esp0 = 0;      // Set it to 0 for now, needs to be modifed when entering user-space
    kernel_tss->iopb = sizeof(tss_t);

    load_tss();
}

void tss_set_stack(uint32_t segment_index, uint32_t sp)
{
    kernel_tss->ssp  = segment_index;
    kernel_tss->esp0 = sp;
}