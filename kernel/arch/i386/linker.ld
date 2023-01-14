/* The bootloader will start execution at the symbol designated as the entry point. In this case, that's '_start' (defined in start.s) */
ENTRY(_start)
 
/* Tell the linker part of the compiler where the various sections of the kernel will be put in the final kernel executable. */
SECTIONS
{
	/* Begin putting sections at 1 Megabyte (1M), a good place for kernels to be loaded at by the bootloader. */
	/* This is because memory below 1 Megabyte is reserved for other x86-related things, so we can't use it */
	. = 1M;
 
	/* Add a symbol that indicates the start address of the kernel. */
	_kernel_start = .;

	/* First put the multiboot header, as it's required to be near the start of the executable otherwise the bootloader won't find it */
	.multiboot.data : {
		*(.multiboot.data)
	}

	.multiboot.text : {
		*(.multiboot.text)
	}

	/* The kernel will live at 3.5 GiB + 1MiB in the virtual address space, */
	/* which will be mapped to 1MiB in the physical address space. */
	/* Note that we page-align the sections. */
	HIGHER_HALF_ADDR = 0xE0000000;

	/* Incremeting our addresses to HIGH_HALF */
	. += HIGHER_HALF_ADDR;

	/* Defining our binary sections, includig symbols marking the start/end of the writable data sections */
	.text ALIGN (4K) : AT (ADDR (.text) - HIGHER_HALF_ADDR)
	{
		*(.text)
	}
	.rodata ALIGN (4K) : AT (ADDR (.rodata) - HIGHER_HALF_ADDR)
	{
		*(.rodata)
	}
	.data ALIGN (4K) : AT (ADDR (.data) - HIGHER_HALF_ADDR)
	{
		_wdata_start = .;
		*(.data)
	}
	.bss ALIGN (4K) : AT (ADDR (.bss) - HIGHER_HALF_ADDR)
	{
		*(COMMON)
		*(.bss)
		*(.bootstrap_stack)
		_wdata_end = .;
	}

	/* Add a symbol that indicates the end address of the kernel. */
	_kernel_end = .;
}