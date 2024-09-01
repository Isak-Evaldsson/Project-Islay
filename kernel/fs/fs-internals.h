#ifndef FS_INTERNALS_H
#define FS_INTERNALS_H
#include <fs.h>
#include <uapi/errno.h>
#include <utils.h>

#define DEBUG_FS 1

#define LOG(fmt, ...) __LOG(DEBUG_FS, "[FS]", fmt, ##__VA_ARGS__)

/* The root vfs root inode, i.e. the root_inode of the rootfs superblock */
extern struct inode* vfs_root;

/*
    Gets the inode with id for a certain superblock. Ensures that the inode is properly read and
    initalized. If successful it returns the inode, on failure it returns NULL and fills the errno
    variable with -ERRNO.
*/
struct inode* get_inode(const struct superblock* super, ino_t id, int* errno);

/* Hands a no longer used inode back to cache. */
void put_node(struct inode* node);

/*
    Get a copy of a inode that already exits in memory.

    Allow one the later call put on it without messing upp the refcount;
 */
struct inode* clone_inode(struct inode* inode);

/* Finds a free fd with the task and allocates an open file object from the global table.
   Returns the found fd and sets the file object pointer.
 */
int alloc_fd(struct task_fs_data* task_data, struct open_file** file);

/* Frees the open file associated with the fd. Returns 0 on success, else errno. */
int free_fd(struct task_fs_data* task_data, int fd);

/* Find the superblock which is mounted upon the supplied inode, returns NULL on failure. */
struct superblock* find_superblock(const struct inode* mounted);

/*
    Iterates over the path until the correct inode is found. On success it returns 0 and sets
    the inode_ptr correctly, otherwise it returns -ERRNO.
*/
int pathwalk(struct inode* root, const char* path, struct inode** inode_ptr);

/*
    sysfs debug functions
*/
void sysfs_dump_vfs();
void sysfs_dump_open_files();
void sysfs_dump_inodes();

#endif /* FS_INTERNALS_H */
