#include <devices/device.h>

#include "../fs-internals.h"

struct devfs_file {
    struct pseudo_file file;
    struct device *dev;
    dev_t dev_no;
};

#define GET_DEV_FILE(file_ptr) GET_STRUCT(struct devfs_file, file, GET_PSEUDO_FILE(file_ptr))

static struct pseudo_file root = {.inode   = (ino_t)&root,
                                  .mode    = S_IFDIR,
                                  .name    = "",
                                  .parent  = NULL,
                                  .child   = NULL,
                                  .sibling = NULL};

int devfs_open(struct open_file* file, int oflag)
{
    int ret;
    struct devfs_file *dev_file = GET_DEV_FILE(file);
    struct device_fops *ops;


    if (S_ISDIR(dev_file->file.mode))
        return 0;

    ops = dev_file->dev->ops;
    if (oflag & O_RDONLY || oflag & O_RDWR) {
        if (!ops->read)
            return -ENOTSUP;
    }

    if (oflag & O_WRONLY || oflag & O_RDWR) {
        if (!ops->write)
            return -ENOTSUP;
    }

    if (ops->open) {
        ret = ops->open(dev_file->dev, file);
        if (ret)
            return ret;
    }
    return 0;
}

int devfs_close(struct open_file* file)
{
    int ret = 0;
    struct devfs_file *dev_file = GET_DEV_FILE(file);
    struct device_fops *ops = dev_file->dev->ops;

    if (ops->close)
        ret = ops->close(dev_file->dev, file);

    return ret;
}

static ssize_t devfs_read(char* buf, size_t size, off_t offset, struct open_file* file)
{
    struct devfs_file *dev_file = GET_DEV_FILE(file);

    return dev_file->dev->ops->read(dev_file->dev, buf, size, offset, file);
}

static ssize_t devfs_write(const char* buf, size_t size, off_t offset, struct open_file* file)
{
    struct devfs_file *dev_file = GET_DEV_FILE(file);

    return dev_file->dev->ops->write(dev_file->dev, buf, size, offset, file);
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

DEFINE_FS(devfs, &devfs_ops, 0);

/*
    Devfs API
*/

/*
    Adds device file within the devfs root dir assoiocated to the supplied device.
    Returns 0 on success, and -ERRNO on failure.
*/
int devfs_create_file(const char *name, dev_t dev_no, bool cdev)
{
    int ret;
    struct pseudo_file *node;
    struct devfs_file *file;
    struct device *dev;

    for (node = root.child; node; node = node->sibling) {
        if (!strcmp(node->name, name))
            return -EEXIST;
    }

    dev = device_get(dev_no);
    if (!dev)
        return -ENODEV;

    file = kalloc(sizeof(*file));
    if (!file)
        return -ENOMEM;

    init_pseudo_file(&file->file, (cdev) ? S_IFCHR : S_IFBLK, name);
    file->dev_no = dev_no;
    file->dev = dev;

    ret = add_pseudo_file(&root, &file->file);
    if (ret < 0) {
        return ret;
    }

    return 0;
}
