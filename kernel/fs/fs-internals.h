/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef FS_INTERNALS_H
#define FS_INTERNALS_H
#include <fs.h>
#include <uapi/errno.h>
#include <utils.h>

#include "sysfs/sysfs.h"

#define DEBUG_FS 1

#define LOG(fmt, ...) __LOG(DEBUG_FS, "[FS]", fmt, ##__VA_ARGS__)

#define FS_NAME_MAXLEN (127)

#define DEFINE_FS(_var, _name, _ops) struct fs _var = {.name = (_name), .ops = (_ops), .next = NULL}

/*  The primary data structure for all file-system objects */
struct inode {
    ino_t        id;     // Unique inode_id for all open files with mounted fs
    mode_t       mode;   // File modes as defined in posix/stat.h
    unsigned int count;  // How many processes references this inode

    struct superblock* super;       // The superblock for the mounted fs the inodes belongs to
    bool               mountpoint;  // Is a file system mounted upon this inode

    bool  inode_dirty;  // Has the inode data changed compared to the one on disk,
    bool  file_dirty;   // Has the file change compared to the one on disk.
    void* data;         // For the specific file system to store relevant data
};

/* All mounted file systems requires a superblock that store data specific for each mountpoint */
struct superblock {
    struct inode* root_inode;     // The root inode of the file system
    struct inode* mounted_inode;  // On which inode is this fs mounted
    struct fs*    fs;             // Which fs does this mountpoint implement
    void*         data;           // For file system specific mountpoint data

    struct superblock* next;  // Used by the per fs mounts list
};

/*
    Information about an open file

    Is needed to a separate object and not stored with the task specific filet_table in order to
    allow a future fork implementation to create child processes that inherit the parents open
    files.
 */
struct open_file {
    // TODO: ADD type field
    unsigned int   ref_count;  // How many process are referring to this object with their fd table
    off_t          offset;     // What position within the file are we currently at
    struct inode*  inode;      // Which file does the object correspond to
    struct fs_ops* file_ops;   // Copy of inode->vfs_node->fs->fs_ops to speed up file accesses
};

/* The different operations a file system can implement. */
struct fs_ops {
    // Returns -ERRNO or 0 on success, as well as filling in the root_ptr with the id of the root
    // inode (septate variable and not return value due to to signedness). The the data pointer
    // allows the function to be supplied with file system specific data.
    int (*mount)(struct superblock* super, void* data, ino_t* root_ptr);

    // Gets attributes for a particular file, returns -errno or 0 on success
    int (*getattr)(const struct open_file* file, struct stat* stat);

    // Reads size bytes from the file at the specified path at the given offset. Returns -errno on
    // failure, or number of read byte on success
    int (*read)(char* buf, size_t size, off_t offset, struct open_file* file);

    // If defined, open() will be called during file opening after its inode is fetched. This allows
    // the fs implementation to provide additional initialisation before file reads/writes. Returns
    // 0 on success and, -errno on failure
    int (*open)(struct open_file* file);

    // Fetch inode from disk and fill the supplied inode pointer
    int (*fetch_inode)(const struct superblock* super, ino_t id, struct inode* inode);

    // Called when reading from a directory, the fs is responsible for filling the supplied
    // dirent entry at the specified offset. On failure return -ERRNO, on success return the
    // next offset or 0 if the full dir is read.
    int (*readdir)(const struct open_file* file, struct dirent* dirent, off_t offset);

    // Called upon when a open file is closed
    // TODO: Needs two methods, one is called on every close and one when refcount == 0
    int (*close)(const char* path, struct open_file* file);

    // TODO: Add support for writable file systems
};

/*  Static filesystem data */
struct fs {
    const char         name[FS_NAME_MAXLEN + 1];
    struct fs_ops*     ops;
    struct fs*         next;
    struct superblock* mounts;  // List's all superblock mounted to this fs
};

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

/**
 * Registers a file system for future mounting. The name of each registered file system is
 * required to be unique and at least 3 characters.
 * @param fs static file system data such as name, opts etc.
 * @return 0 on success, -EEXIST if there's already exists a file system with the same.
 */
int register_fs(struct fs* fs);

/* Specially mount function handling mounting of the root fs, retruns 0 on success and -ERRNO on
 * failure*/
int mount_rootfs(char* name, void* data);

/*
    Iterates over the path until the correct inode is found. On success it returns 0 and sets
    the inode_ptr correctly, otherwise it returns -ERRNO.
*/
int pathwalk(struct inode* root, const char* path, struct inode** inode_ptr);

/* Connects a file to its directory file.  Returns 0 on success, else -ERRNO */
int add_pseudo_file(struct pseudo_file* dir, struct pseudo_file* file);

/* Ensure all the fields in the pseudo file to be properly initalized */
void init_pseudo_file(struct pseudo_file* file, mode_t mode, const char* name);

/* Generic readdir function for pseudo filesystems */
int pseudo_file_readdir(const struct open_file* file, struct dirent* dirent, off_t offset);

/* Generic fetch_inode function for pseudo filesystems */
int pseudo_fetch_inode(const struct superblock* super, ino_t id, struct inode* inode);

/* TODO: Document */
#define GET_PSEUDO_FILE(open_file_ptr)            \
    ({                                            \
        struct open_file* _ptr = (open_file_ptr); \
        (struct pseudo_file*)_ptr->inode->id;     \
    })

/*
    sysfs debug functions
*/
void sysfs_dump_vfs();
void sysfs_dump_open_files();
void sysfs_dump_inodes();

#endif /* FS_INTERNALS_H */
