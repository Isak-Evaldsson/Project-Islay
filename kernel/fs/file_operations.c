#include "fs-internals.h"

int open(struct task_fs_data* task_data, const char* path)
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
    node = search_vfs(path, &internal_path);
    if (!node) {
        ret = -ENOENT;
        goto end;
    }

    // TODO: Will this be a problem? Assert for now so it's caught at least.
    kassert(node->type == VFS_NODE_TYPE_MNT);

    // Call open within the mounted file system
    ret = node->fs->ops->open(internal_path, &id, file);
    if (ret < 0) {
        goto end;
    }

    inode = get_inode(node, id);
    if (!inode) {
        ret = -ENOMEM;
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

