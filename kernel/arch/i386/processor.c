#include "processor.h"

uint32_t get_esp()
{
    uint32_t esp;
    asm volatile("mov %%esp, %0" : "r="(esp));
    return esp;
}

uint32_t get_cr2()
{
    uint32_t cr2;
    asm volatile("mov %%cr2, %0" : "r="(cr2));
    return cr2;
}

uint32_t get_cr3()
{
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "r="(cr3));
    return cr3;
}
