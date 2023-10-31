# Project Islay Kernel
This folder contains the kernel, the makefile builds a kernel binary that can either be run directly in qemu or be packaged together with grub into an iso.

## Structure
* include - header files defining the kernel api
* arch - architecture specific code
* devices 
* tasks - task, process and scheduling related code
* memory - memory related code such as page or vmem allocations
* utils - utility functions handling for example printing or logging
* boot - architecture independent boot code
