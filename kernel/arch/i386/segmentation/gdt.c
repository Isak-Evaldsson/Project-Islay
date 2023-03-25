/*
    Functions to setup/handle gpt tables for x86
*/

#include "gdt.h"

#include <klib/klib.h>
#include <stdint.h>

/*
    Segment entry macros:

    Each define here is for a specific flag in the descriptor.
    Refer to the intel documentation for a description of what each one does.
*/
#define SEG_DESCTYPE(x) ((x) << 0x04)         // Descriptor type (0 for system, 1 for code/data)
#define SEG_PRES(x)     ((x) << 0x07)         // Present
#define SEG_SAVL(x)     ((x) << 0x0C)         // Available for system use
#define SEG_LONG(x)     ((x) << 0x0D)         // Long mode
#define SEG_SIZE(x)     ((x) << 0x0E)         // Size (0 for 16-bit, 1 for 32)
#define SEG_GRAN(x)     ((x) << 0x0F)         // Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB)
#define SEG_PRIV(x)     (((x)&0x03) << 0x05)  // Set privilege level (0 - 3)

#define SEG_DATA_RD        0x00  // Read-Only
#define SEG_DATA_RDA       0x01  // Read-Only, accessed
#define SEG_DATA_RDWR      0x02  // Read/Write
#define SEG_DATA_RDWRA     0x03  // Read/Write, accessed
#define SEG_DATA_RDEXPD    0x04  // Read-Only, expand-down
#define SEG_DATA_RDEXPDA   0x05  // Read-Only, expand-down, accessed
#define SEG_DATA_RDWREXPD  0x06  // Read/Write, expand-down
#define SEG_DATA_RDWREXPDA 0x07  // Read/Write, expand-down, accessed
#define SEG_CODE_EX        0x08  // Execute-Only
#define SEG_CODE_EXA       0x09  // Execute-Only, accessed
#define SEG_CODE_EXRD      0x0A  // Execute/Read
#define SEG_CODE_EXRDA     0x0B  // Execute/Read, accessed
#define SEG_CODE_EXC       0x0C  // Execute-Only, conforming
#define SEG_CODE_EXCA      0x0D  // Execute-Only, conforming, accessed
#define SEG_CODE_EXRDC     0x0E  // Execute/Read, conforming
#define SEG_CODE_EXRDCA    0x0F  // Execute/Read, conforming, accessed

#define GDT_CODE_PL0                                                                        \
    SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) | SEG_SIZE(1) | SEG_GRAN(1) | \
        SEG_PRIV(0) | SEG_CODE_EXRD

#define GDT_DATA_PL0                                                                        \
    SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) | SEG_SIZE(1) | SEG_GRAN(1) | \
        SEG_PRIV(0) | SEG_DATA_RDWR

#define GDT_CODE_PL3                                                                        \
    SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) | SEG_SIZE(1) | SEG_GRAN(1) | \
        SEG_PRIV(3) | SEG_CODE_EXRD

#define GDT_DATA_PL3                                                                        \
    SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) | SEG_SIZE(1) | SEG_GRAN(1) | \
        SEG_PRIV(3) | SEG_DATA_RDWR

/* The global descriptor table, hardcode size since it will only be filled with the bare minimum
 * required for flat mode  */
uint64_t gdt[5];

/*
    Creates a segment descriptor according the following layout:

    Upper 32-bit half:
    | Bit     | Field        |
    |---------|--------------|
    | 32 - 24 | Base         |
    | 23 - 20 | Flags        |
    | 19 - 16 | Limit        |
    | 7 - 0   | Access byte  |

    Lower 32-bit half:
    | Bit     | Field        |
    |---------|--------------|
    | 31 - 16 | Base         |
    | 15 -  0 | Limit        |

    The 16 byte flag parameter encodes both flags (4 msb) and access byte (8 lsb)
*/
uint64_t create_descriptor(uint32_t base, uint32_t limit, uint16_t flag)
{
    uint64_t descriptor;
    // Create the high 32 bit segment
    descriptor = limit & 0x000F0000;          // set limit bits 19:16
    descriptor |= (flag << 8) & 0x00F0FF00;   // set type, p, dpl, s, g, d/b, l and avl fields
    descriptor |= (base >> 16) & 0x000000FF;  // set base bits 23:16
    descriptor |= base & 0xFF000000;          // set base bits 31:24

    // Shift by 32 to allow for low part of segment
    descriptor <<= 32;

    // Create the low 32 bit segment
    descriptor |= base << 16;          // set base bits 15:0
    descriptor |= limit & 0x0000FFFF;  // set limit bits 15:0 printf("0x%.16llX\n", descriptor);
    return descriptor;
}

/*
    Setups up the kernel and userspace global descriptor table

    In order to be compatibly with modern compilers we setup our gdp in a flat mode setup, meaning
    that we define a data and code segment spanning the whole 4 GiB ram for both kernel and user
    mode.

    This configuration ignores the segment mechanism and instead relies on having the access
    control handled through paging mechanics
*/
void init_gdt()
{
    /* Defining our table entries */
    uint64_t null_segment        = create_descriptor(0, 0, 0);
    uint64_t kernel_code_segment = create_descriptor(0, 0x000FFFFF, (GDT_CODE_PL0));
    uint64_t kernel_data_segment = create_descriptor(0, 0x000FFFFF, (GDT_DATA_PL0));
    uint64_t user_code_segment   = create_descriptor(0, 0x000FFFFF, (GDT_CODE_PL3));
    uint64_t user_data_segment   = create_descriptor(0, 0x000FFFFF, (GDT_DATA_PL3));

    /* Filling the table */
    gdt[0] = null_segment;
    gdt[1] = kernel_code_segment;
    gdt[2] = kernel_data_segment;
    gdt[3] = user_code_segment;
    gdt[4] = user_data_segment;

    gdt_ptr_t ptr;
    ptr.size    = sizeof(gdt) - 1;
    ptr.address = (uint32_t)&gdt;
    load_gdt(&ptr);
    kprintf("Successfully initiated GDT\n");
}
