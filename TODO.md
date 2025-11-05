# TODO
This contains various features/improvements for the kernel that will hopefully be done some day

## Memory systems
* Ensure that both heap allocator and page allocator is using locks

### Paging
* Mark data/bss, rodata and text with different security bits in page table

## Task/Scheduler
* See todo list in scheduler.c
* Fill the task struct with more data such as task name, tid, parent thread etc.
* Scheduler:
    * Double-check so that the time counting works correctly
    * Better algorithm than round robin
    * Optimize preemption callbacks. Don't create new ones if preemption_timestamp_ns == 0,
      register new ones when it's set again

## Klib
* make formatted writer support 64-bit integers
* fix bug in formatted printing with multiple %u:s in a row
* Make log print timestamp in the beginning of every message

## Others
* Pick a license

See: https://wiki.osdev.org/Going_Further_on_x86 
