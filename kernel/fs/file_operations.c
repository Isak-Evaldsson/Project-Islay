#include "fs-internals.h"

int open(struct task_fs_data* task_data, const char* path, int oflag)
{
    int               fd, ret;
    unsigned int      id;
    char*             ppath = strdup(path);
    char*             internal_path;
    struct vfs_node*  node;
    struct open_file* file;
    struct inode*     inode;

    fd = alloc_fd(task_data, &file);
    if (fd < 0) {
        ret = fd;
        goto end;
    }

    // Search vfs for the mountpoint containing the supplied path
    node = search_vfs(ppath, &internal_path);
    if (!node) {
        ret = -ENOENT;
        goto end;
    }

    // We have 3 scenarios to handle:
    //    1. MNTPOINT - defer to fs implementation
    //    2. DIR - fallback to root fs?, Or do we want a "vfs-fs"? we're read/writes are not
    //       allowed, but READDIR is?
    if (!*internal_path) {
        // TODO: Handle opening a vfs dir!!!
    }

    // TODO: Will this be a problem? Assert for now so it's caught at least.
    kassert(node->type == VFS_NODE_TYPE_MNT);

    // Call open within the mounted file system
    inode = node->fs->ops->open(node, internal_path);
    if (inode == NULL) {
        ret = -ENOENT;
        goto end;
    }

    // Check that the fs implementation has filled in the inode correctly
    ret = verify_inode(inode);
    if (ret < 0) {
        goto end;
    }

    if ((oflag & O_DIRECTORY) && !S_ISDIR(inode->mode)) {
        ret = -ENOTDIR;
        goto end;
    }

    file->file_ops            = node->fs->ops;
    file->inode               = inode;
    file->ref_count           = 1;  // Marks the file object as allocated
    task_data->file_table[fd] = file;

    ret = fd;
end:
    kfree(ppath);
    return ret;
}

int close(struct task_fs_data* task_data, int fd)
{
    int ret;

    // TODO: Do we need to call back to fs at close?
    ret = free_fd(task_data, fd);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

static ssize_t read_helper(struct task_fs_data* task_data, int fd, void* buf, size_t nbyte,
                           off_t offset, bool use_file_offset)
{
    int               read_bytes;
    off_t             read_offset;
    struct open_file* file;

    if (fd < 0 || fd >= MAX_OPEN_PER_PROC) {
        return -EBADF;
    }

    file = task_data->file_table[fd];
    if (!file) {
        return -EBADF;
    }

    if (S_ISDIR(file->inode->mode)) {
        return -EINVAL;
    }

    read_offset = use_file_offset ? file->offset : offset;

    // TODO: Should we count the offset/bytes to ensure that we don't over-read or should we hand
    // over the responsibilty to the fs implementation?
    read_bytes = file->file_ops->read(buf, nbyte, file->offset, file);
    if (read_bytes < 0) {
        return read_bytes;
    }

    if (use_file_offset) {
        file->offset += read_bytes;
    }

    return read_bytes;
}

ssize_t pread(struct task_fs_data* task_data, int fd, void* buf, size_t nbyte, off_t offset)
{
    return read_helper(task_data, fd, buf, nbyte, offset, false);
}

ssize_t read(struct task_fs_data* task_data, int fd, void* buf, size_t nbyte)
{
    return read_helper(task_data, fd, buf, nbyte, 0, true);
}
