# TODO
This contains various features/improvements for the kernel that will hopefully be done some day

## Memory systems
* Ensure that both heap allocator and page allocator is using locks
* Buddy allocator?

## ARCH
* Upgrade target i386 to i686 (update build script + kernel src dir names)

### Paging
* Mark data/bss, rodata and text with different security bits in page table

## Task/Scheduler
* Implement spin_lock, even if where are still UMP, it's nice to have a good api ready
* See todo list in scheduler.c
* Fil the task struct with more data such as task name, tid, parent thread etc.

## Klib
* make formatted writer support 64-bit integers
* fix bug in formatted printing with multiple %u:s in a row
* Make log print timestamp in the beginning of every message

See: https://wiki.osdev.org/Going_Further_on_x86 
