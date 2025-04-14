#include <devices/device.h>

#include "devfs.h"

static struct pseudo_file root = {.inode   = (ino_t)&root,
                                  .mode    = S_IFDIR,
                                  .name    = "",
                                  .parent  = NULL,
                                  .child   = NULL,
                                  .sibling = NULL};

int devfs_open(struct open_file* file, int oflag)
{
    struct pseudo_file* pseudo_file = GET_PSEUDO_FILE(file);

    if (S_ISBLK(pseudo_file->mode) || S_ISCHR(pseudo_file->mode)) {
        return dev_open((dev_t)pseudo_file->data, file, oflag);
    }
    return 0;
}

int devfs_close(struct open_file* file)
{
    struct pseudo_file* pseudo_file = GET_PSEUDO_FILE(file);

    if (S_ISBLK(pseudo_file->mode) || S_ISCHR(pseudo_file->mode)) {
        return dev_close((dev_t)pseudo_file->data, file);
    }
    return 0;
}

static ssize_t devfs_read(char* buf, size_t size, off_t offset, struct open_file* file)
{
    struct pseudo_file* pseudo_file = GET_PSEUDO_FILE(file);

    return dev_read((dev_t)pseudo_file->data, buf, size, offset);
}

static ssize_t devfs_write(const char* buf, size_t size, off_t offset, struct open_file* file)
{
    struct pseudo_file* pseudo_file = GET_PSEUDO_FILE(file);

    return dev_write((dev_t)pseudo_file->data, buf, size, offset);
}

static int devfs_mount(struct superblock* super, void* data, ino_t* root_ptr)
{
    (void)data;
    (void)super;

    *root_ptr = root.inode;
    return 0;
}

static struct fs_ops devfs_ops = {
    .mount       = devfs_mount,
    .read        = devfs_read,
    .readdir     = pseudo_file_readdir,
    .fetch_inode = pseudo_fetch_inode,
    .write       = devfs_write,
    .open        = devfs_open,
};

DEFINE_FS(devfs, DEVFS_FS_NAME, &devfs_ops, 0);

/*
    Devfs API
*/

/*
    Adds the supplied device object to devfs  relative to the specified directory
    (or within fs root if NULL). Returns 0 on success, and -ERRNO on failure.
*/
int devfs_add_dev(struct pseudo_file* dir, struct pseudo_file* file, dev_t dev_no, char* name,
                  bool cdev)
{
    int ret;
    // struct pseudo_file* file = &dev->file;

    if (!dir) {
        dir = &root;
    }

    init_pseudo_file(file, (cdev) ? S_IFCHR : S_IFBLK, name);

    ret = add_pseudo_file(dir, file);
    if (ret < 0) {
        return ret;
    }

    file->data = (void*)dev_no;
    return 0;
}
