# TODO
This contains various features/improvements for the kernel that will hopefully be done some day

## Memory systems
### Paging
* Mark data/bss, rodata and text with different security bits in page table

## Task/Scheduler
* See todo list in scheduler.c
* Fil the task struct with more data such as task name, tid, parent thread etc.

## Klib
* make formatted writer support 64-bit integers
* fix bug in formatted printing with multiple %u:s in a row
* Make log print timestamp in the beginning of every message

See: https://wiki.osdev.org/Going_Further_on_x86 
