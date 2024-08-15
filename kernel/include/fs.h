#ifndef FS_H
#define FS_H
#include <libc.h>
#include <posix/stat.h>
#include <stdbool.h>

#define MAX_OPEN_GLOBAL   100
#define MAX_OPEN_PER_PROC 20
static_assert(MAX_OPEN_GLOBAL >= MAX_OPEN_PER_PROC, "MAX_OPEN_GLOBAL less than MAX_OPEN_PER_PROC");

#define FS_NAME_MAXLEN (127)

#define DEFINE_FS(_var, _name, _ops) struct fs _var = {.name = (_name), .ops = (_ops), .next = NULL}

/*  The primary data structure for all file-system objects */
struct inode {
    unsigned int     id;        // Unique inode_id for all open files with mounted fs
    unsigned int     count;     // How many processes references this inode
    struct vfs_node* vfs_node;  // Which mounted file system does this inode belong to

    // TODO: Mode bits
    bool  inode_dirty;  // Has the inode data changed compared to the one on disk,
    bool  file_dirty;   // Has the file change compared to the one on disk.
    void* data;         // For the specific file system to store relevant data
};

/* Information about an open file */
struct file_info {
    off_t fh;  // Internal file-handle that may be used by the fs implementation
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
    int (*read)(const char* path, char* buf, size_t size, off_t offset, struct file_info* info);

    int (*open)(const char* path, struct file_info* info);
    int (*readdir)(const char* path, fill_dir_t filler, off_t offset, struct file_info);

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
 * Registers a file system for future mounting. The name of each registered file system is required
 * to be unique and at least 3 characters.
 * @param fs static file system data such as name, opts etc.
 * @return 0 on success, -EEXIST if there's already exists a file system with the same.
 */
int register_fs(struct fs* fs);

/**
 * Mounts a file system to a given path, the caller is responsible for supplying correct mounting
 * data for a particular file system.
 * @param path were to mount the fs
 * @param name the name of the file system to mount
 * @param data fs specific data needed for mounting
 * @return 0 on success, -errno on failure */
int mount(const char* path, const char* name, void* data);

#endif /* FS_H */
