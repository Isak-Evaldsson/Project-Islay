#ifndef FS_H
#define FS_H
#include <libc.h>
#include <posix/stat.h>
#include <posix/types.h>
#include <stdbool.h>

#define MAX_OPEN_GLOBAL   100
#define MAX_OPEN_PER_PROC 20
static_assert(MAX_OPEN_GLOBAL >= MAX_OPEN_PER_PROC, "MAX_OPEN_GLOBAL less than MAX_OPEN_PER_PROC");

#define FS_NAME_MAXLEN (127)

#define DEFINE_FS(_var, _name, _ops) struct fs _var = {.name = (_name), .ops = (_ops), .next = NULL}

/*  The primary data structure for all file-system objects */
struct inode {
    ino_t            id;        // Unique inode_id for all open files with mounted fs
    mode_t           mode;      // File modes as defined in posix/stat.h
    unsigned int     count;     // How many processes references this inode
    struct vfs_node* vfs_node;  // Which mounted file system does this inode belong to

    // TODO: Mode bits
    bool  inode_dirty;  // Has the inode data changed compared to the one on disk,
    bool  file_dirty;   // Has the file change compared to the one on disk.
    void* data;         // For the specific file system to store relevant data
};

/*
    Information about an open file

    Is needed to a separate object and not stored with the task specific filet_table in order to
    allow a future fork implementation to create child processes that inherit the parents open
    files.
 */
struct open_file {
    unsigned int   ref_count;  // How many process are referring to this object with their fd table
    off_t          offset;     // What position within the file are we currently at
    struct inode*  inode;      // Which file does the object correspond to
    struct fs_ops* file_ops;   // Copy of inode->vfs_node->fs->fs_ops to speed up file accesses
};

/* Helper function for the file system to create directory entries */
typedef int (*fill_dir_t)(void* buf, const char* name, const struct stat* stat, off_t off);

/* The different operations a file system can implement. */
struct fs_ops {
    // Returns -errno or 0 on success, the data pointer allows the function to be supplied with
    // file system specific data
    int (*mount)(void* data);

    // Gets attributes for a particular file, returns -errno or 0 on success
    int (*getattr)(const char* path, struct stat* stat);

    // Reads size bytes from the file at the specified path at the given offset. Returns -errno on
    // failure, or number of read byte on success
    int (*read)(char* buf, size_t size, off_t offset, struct open_file* file);

    // Returns a pointer to the inode at the specified path, or NULL If the inode can't be found
    struct inode* (*open)(const struct vfs_node* node, const char* path);

    int (*readdir)(const char* path, fill_dir_t filler, off_t offset, struct open_file* file);

    // Called upon when a open file is closed
    // TODO: Needs two methods, one is called on every close and one when refcount == 0
    int (*close)(const char* path, struct open_file* file);

    // TODO: Add support for writable file systems
};

/*  Static filesystem data */
struct fs {
    const char     name[FS_NAME_MAXLEN + 1];
    struct fs_ops* ops;
    struct fs*     next;
};

/* Struct containing all per task fs related data */
struct task_fs_data {
    struct open_file* file_table[MAX_OPEN_PER_PROC];
};

/**
 * Registers a file system for future mounting. The name of each registered file system is
 * required to be unique and at least 3 characters.
 * @param fs static file system data such as name, opts etc.
 * @return 0 on success, -EEXIST if there's already exists a file system with the same.
 */
int register_fs(struct fs* fs);

/**
 * Mounts a file system to a given path, the caller is responsible for supplying correct
 * mounting data for a particular file system.
 * @param path were to mount the fs
 * @param name the name of the file system to mount
 * @param data fs specific data needed for mounting
 * @return 0 on success, -errno on failure */
int mount(const char* path, const char* name, void* data);

/* Open file */
int open(struct task_fs_data* task_data, const char* path);

/* Close file */
int close(struct task_fs_data* task_data, int fd);

/* Read file continuously */
ssize_t read(struct task_fs_data* task_data, int fd, void* buf, size_t nbyte);

/* Read file at fixed offset */
ssize_t pread(struct task_fs_data* task_data, int fd, void* buf, size_t nbyte, off_t offset);

/*
    Sysfs API
*/

/* Writes formatted strings to the global sysfs read buffer */
void sysfs_writer(const char* restrict format, ...);

/* Mounts sysfs at the specified path */
int mount_sysfs(const char* path);

#endif /* FS_H */
