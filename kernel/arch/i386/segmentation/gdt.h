#ifndef ARCH_i386_GDT_H
#define ARCH_i386_GDT_H

#include <stdint.h>

/*
    Properly formatted pointer for the gdt/idt tables
*/
typedef struct gdt_ptr_t {
    uint16_t size;     // The upper 16 bits of all selector limits.
    uint32_t address;  // The address of the first gdt_entry_t struct.
} __attribute__((packed)) gdt_ptr_t;

/*
    Assembly routine for properly loading the GDT registers (see load_gdt.S),
*/
extern void load_gdt(gdt_ptr_t *ptr);

#endif /* ARCH_i386_GDT_H */
