#ifndef FS_H
#define FS_H
#include <arch/boot.h>
#include <libc.h>
#include <stdbool.h>
#include <uapi/dirent.h>
#include <uapi/sys/fnctl.h>
#include <uapi/sys/stat.h>

#define MAX_OPEN_GLOBAL   100
#define MAX_OPEN_PER_PROC 20
static_assert(MAX_OPEN_GLOBAL >= MAX_OPEN_PER_PROC, "MAX_OPEN_GLOBAL less than MAX_OPEN_PER_PROC");

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

/* Struct containing all per task fs related data */
struct task_fs_data {
    // Note, the inode pointers needs to be clone on usage, otherwise calls to put_inode() may lead
    // to the being free which may break the vfs it they point to a root inode
    struct inode* rootdir;  // Root directory, stored per process to enable chroot syscall
    struct inode* workdir;  // Start inode for relative paths

    struct open_file* file_table[MAX_OPEN_PER_PROC];
};

/* To ensure that the per task fs data struct is properly initialised */
void task_data_init(struct task_fs_data* task_data);

/** Initialise the file system based on the boot data parameters */
int fs_init(struct boot_data* boot_data);

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
int open(struct task_fs_data* task_data, const char* path, int oflag);

/* Close file */
int close(struct task_fs_data* task_data, int fd);

/* Read file continuously */
ssize_t read(struct task_fs_data* task_data, int fd, void* buf, size_t nbyte);

/* Read file at fixed offset */
ssize_t pread(struct task_fs_data* task_data, int fd, void* buf, size_t nbyte, off_t offset);

/*
    Reads from at certain fd into the supplied buffer and returns the number of dirents written to
    the buffer or -ERRNO on failure.

    Similar to read it automatically increments the read offset. If the return value is 0 or less
    than buf_count we have reach the end of the dir.
*/
int readdirents(struct task_fs_data* task_data, int fd, struct dirent* buf, int buf_count);

/* Mounts sysfs at the specified path */
int mount_sysfs(const char* path);

#endif /* FS_H */
