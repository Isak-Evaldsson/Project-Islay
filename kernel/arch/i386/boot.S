/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#define ASM_FILE        1 /* Makes sure no c code is included */
#include <boot/multiboot.h>

# label to the kernel init function
.extern kernel_init

# Higher half staring address
.set HIGHER_HALF_ADDR, 0xE0000000
.set HIGHER_HALF_PAGE_TABLE_INDEX, 896 # = (HIGH_HALF_ADDR / 4.0 Gib) * 1024

# Multiboot configuration
#define MULTIBOOT_HEADER_FLAGS	MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO

# Declare a multiboot header that marks the program as a kernel.
.section .multiboot.data, "aw"
    .align 4
    .long MULTIBOOT_HEADER_MAGIC
    .long MULTIBOOT_HEADER_FLAGS 
    .long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

# Allocate the initial stack 16 KiB stack
.section .bootstrap_stack, "aw", @nobits
    stack_bottom:
    .skip 16384
    stack_top:

# Preallocate page tables. Placed BSS section since we what the page tables to be initialised to 0
.section .bss, "aw", @nobits
	.align 4096 # 4096 Aligend since 12 lsb is used for flags 
.global boot_page_directory
boot_page_directory:
	.skip 4096 # One directory fits 1024 (32 bit) entries
boot_page_table1:
	.skip 4096 # One table fits 1024 (32 bit) entries, i.e. can map 1024 * 4K pages = 4 Mib
# Note, Further page tables may be required if the kernel grows beyond 3 MiB.


# Intitial kernel boot code, contained in multiboot section.
# Serves the purpose of setting up the initial paging before entering higher half
.section .multiboot.text, "ax"
.global _start
.type _start, @function
_start:
	# Disable interrupts at boot
	cli

    # Physical address of boot_page_table1.
	# TODO: I recall seeing some assembly that used a macro to do the
	#       conversions to and from physical. Maybe this should be done in this
	#       code as well?
	movl $(boot_page_table1 - HIGHER_HALF_ADDR), %edi
	
    # First physical address to map is address 0.
	# TODO: Start at the first kernel page instead. Alternatively map the first
	#       1 MiB as it can be generally useful, and there's no need to
	#       specially map the VGA buffer.	
    movl $0, %esi

    # Page table setup loop	
table_loop:

    # Once the full kernel is mapped, exit loop
    cmpl $(_kernel_end - HIGHER_HALF_ADDR), %esi
	jge end

	# Set default permisisons, "present, read-only"
    movl $0x001, %ecx

	# If within are the write area (data, bss and stack), enable writing
   	cmpl $(_wdata_start - HIGHER_HALF_ADDR), %esi
   	jl  set_permission
   	cmpl $(_wdata_end - HIGHER_HALF_ADDR), %esi
   	jge  set_permission
   	orl  $0x002, %ecx
   
set_permission:
	# Map physical address with specified permissions
   	movl %esi, %edx
   	orl  %ecx, %edx
   	movl %edx, (%edi)

skip:
	addl $4096, %esi # Increment page address by 4 KiB page size
	addl $4, %edi    # Increment page table address by 4B (each entry is 32 bits)
	jmp table_loop
   
end:
	# Map VGA video memory to 0xC03FF000 as "present, writable".
	movl $(0x000B8000 | 0x003), boot_page_table1 - HIGHER_HALF_ADDR + 1023 * 4

	# The page table is used at both page directory entry 0 (virtually from 0x0
	# to 0x3FFFFF) (thus identity mapping the kernel) and page directory entry
	# 896 (virtually from 0xE0000000 to 0xE03FFFFF) (thus mapping it in the
	# higher half). The kernel is identity mapped because enabling paging does
	# not change the next instruction, which continues to be physical. The CPU
	# would instead page fault if there was no identity mapping.

	# Map the page table to both virtual addresses 0x00000000 and 0xE0000000.
	movl $(boot_page_table1 - HIGHER_HALF_ADDR + 0x003), boot_page_directory - HIGHER_HALF_ADDR + 0
	movl $(boot_page_table1 - HIGHER_HALF_ADDR + 0x003), boot_page_directory - HIGHER_HALF_ADDR + HIGHER_HALF_PAGE_TABLE_INDEX * 4

	# Set cr3 to the address of the boot_page_directory.
	movl $(boot_page_directory - HIGHER_HALF_ADDR), %ecx
	movl %ecx, %cr3

	# Enable paging and the write-protect bit.
	movl %cr0, %ecx
	orl $0x80010000, %ecx
	movl %ecx, %cr0

	# Jump to higher half with an absolute jump. 
	lea kernel_start, %ecx
	jmp *%ecx


# Implementing function defined in arch/boot.h
.section .text
.global unmap_identity_mapping
unmap_identity_mapping:
	
	# Loop removing the < 1 MiB section from boot_page_table1
	movl $boot_page_table1, %edi # Table start address (no need to remove higher-half addr since paging is now enabled)
loop:
	movl $0, (%edi)  # clear table entry
	addl $4, %edi    # Increment page table address by 4 B (each entry is 32 bits)
	cmpl $(boot_page_table1 + 256 * 4), %edi # < 1 MiB covers the first 256 entries (0-255)
	jl loop

	# Unmap the identity mapping as it is now unnecessary. 
	movl $0, boot_page_directory + 0

	# Reload cr3 to force a TLB flush so the changes to take effect.
 	movl %cr3, %ecx
	movl %ecx, %cr3
	ret


# Setup and launch kernel C code
.section .text
kernel_start:
    # At this point, paging is fully set up and enabled.

    # sets correct stack pointer (x86 stack grows downward)
    mov $stack_top, %esp

	# fetch multiboot values from %eax and %ebx
	pushl %eax /* magic number */
	pushl %ebx /* multiboot_info_t */

    # Call the global constructors.
    call _init

    # calls the kernel init function
    call kernel_init

    # If, by some mysterious circumstances, the kernel's C code ever returns, all we want to do is to hang the CPU
    hang:
        cli         # disable interrupts
        hlt         # halt cpu
        jmp hang    # retry on failure
             
