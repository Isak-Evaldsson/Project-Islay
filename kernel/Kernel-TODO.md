# Kernel TODO

## FS
1. ~~Romfs + working initrd (DONE, just needs to be committed)~~
2. ~~Add root file system to vfs tree (When opening if search_vfs returns null, the give the full path to the rootfs)~~
3. ~~Implement a path walker that find the inode for a path by calls get_inode (backed by fs implemented fetch inode method if node is not cached) and fs-impl.readdir.~~
4. ~~Implement som kind of superblk / per mount specific data~~
5. ~~Get rid of the VFS tree (if possible) + research how other file systems (for example mimix) solves the problem of mouting filesystem inside of filesystems.~~
6. Implement ramfs and kinfofs
7. Locking, per file? per mounted fs?
8. Fix tests
9. ~~Restructure the files~~
10. See todo functions

## Kshell / Devices
* Add support for keymaps (see Sebastians avrterm-kbd as an example)

## Scheduling / Interrupts
* Split timer
* Consider Minix style context switch, i.e. schedule() does not explicitly call a context switch it rather changes a pointer that will be compared with the current_task at the end of the interrupt. Might be clear, I guess it depends e bit on how userspace context switches are handled.

## Arch x86:
* Cleanup files, one file for ASM, move drivers to /devices
* Rename folders to i686 or just x86
* Check cpuid >= i686 on boot

## Build system
* Make build system aware of changes in change defines

# Memory
* Ensure that the VGA memory is marked as not available

## Utils
* ~~Replace kmalloc and kcalloc with a single kalloc which is safe and clearing out if the box~~
* ~~License + copyright headers + check pyhon-script~~
* ~~Merge all the build scripts~~
* Use stdatomics?
* Make hooks warn on bad formating instead of fail the commit
* Implement a generic statically allocated buffer data structure, something like this:

```c

// If possible assert that type is a valid type, and size positive (check so it is doesn't overflow: sizeof(type) * size >= size)
#define DEFINE_STATIC_BUFF_TYPE(name, type, size) \
    struct static_buff_#NAME { \
        size_t first; \
        type buff[size]; \
    }

// INIT MACRO

// ALLOC_MACRO which is a typesafe call to a generic function

// FREE_MACRO which is a typesafe call to a generic function
```
