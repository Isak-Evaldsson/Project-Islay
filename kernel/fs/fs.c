#include <arch/paging.h>
#include <libc.h>
#include <utils.h>

#include "fs-internals.h"
#include "romfs/romfs.h"

/*
    VFS main data structures
*/
static struct fs* fs_list = NULL;

struct inode* vfs_root;

#define N_SUPERBLOCK 10
static struct superblock superblocks[N_SUPERBLOCK];

/* Find the superblock which is mounted upon the supplied inode, returns NULL on failure. */
struct superblock* find_superblock(const struct inode* mounted)
{
    struct superblock* super;

    for (super = superblocks; super < END_OF_ARRAY(superblocks); super++) {
        if (super->fs != NULL && super->mounted_inode == mounted) {
            return super;
        }
    }
    return NULL;
}

int mount_rootfs(char* name, void* data)
{
    int                ret;
    struct fs*         fs;
    struct superblock* superblk = NULL;

    if (vfs_root != NULL) {
        // vfs root can't be re-mounted
        return -EEXIST;
    }

    for (fs = fs_list; fs != NULL; fs = fs->next) {
        if (strncmp(fs->name, name, FS_NAME_MAXLEN) == 0)
            break;
    }
    if (!fs)
        return -ENOENT;  // No fs with name

    // Find superblock
    for (size_t i = 0; i < N_SUPERBLOCK; i++) {
        if (superblocks[i].fs == NULL) {
            superblk = superblocks + i;
            break;
        }
    }

    if (!superblk) {
        LOG("Out of superblocks");
        return -ENOMEM;
    }

    // Needs to be assigned here so that the fs implementation can call get_inode within mount()
    superblk->fs = fs;

    ret = fs->ops->mount(superblk, data);
    if (ret < 0) {
        superblk->fs = NULL;  // Mark superblock as free
        return ret;
    }

    // Ensure that the superblock contains a valid root inode
    if (!superblk->root_inode || !superblk->root_inode->id ||
        !S_ISDIR(superblk->root_inode->mode) || superblk->root_inode->count != 1) {
        superblk->fs = NULL;  // Mark superblk as free
        return -EINVAL;
    }

    // Special case for root inode
    superblk->mounted_inode = NULL;

    // Add superblk to list the per fs mountlist
    superblk->next = fs->mounts;
    fs->mounts     = superblk;

    vfs_root = superblk->root_inode;
    return 0;
}

void sysfs_dump_vfs()
{
    sysfs_writer("VFS Dump\n");
    sysfs_writer("Registered file systems:\n");

    for (struct fs* fs = fs_list; fs != NULL; fs = fs->next) {
        sysfs_writer("  (%x) name: %s, ops: %x, mountpoints:\n", fs, fs->name, fs->ops);
        for (struct superblock* super = fs->mounts; super != NULL; super = super->next) {
            sysfs_writer("    ->(%x) mounted inode: %x, root inode: %x\n", super,
                         super->mounted_inode, super->root_inode);
        }
    }
}

static int check_required_fs_ops(const struct fs_ops* ops)
{
    if (!ops->mount || !ops->read || !ops->fetch_inode || !ops->readdir)
        return -1;

    return 0;
}

int register_fs(struct fs* fs)
{
    size_t     len;
    struct fs *prev, *next;

    if (!fs)
        return -EFAULT;

    if (!fs->ops || check_required_fs_ops(fs->ops))
        return -EINVAL;

    len = strlen(fs->name);
    if (len < 3 || len > FS_NAME_MAXLEN)
        return -ENAMETOOLONG;

    fs->next = NULL;
    if (fs_list == NULL) {
        fs_list = fs;
        return 0;
    }

    prev = next = fs_list;
    while (next != NULL) {
        if (strncmp(fs->name, next->name, FS_NAME_MAXLEN) == 0) {
            return -EEXIST;
        }

        prev = next;
        next = next->next;
    }

    prev->next = fs;
    return 0;
};

int mount(const char* path, const char* name, void* data)
{
    int                ret;
    struct fs*         fs;
    struct inode*      inode;
    struct superblock* superblk;

    if (*path != '/')
        return -EINVAL;

    for (fs = fs_list; fs != NULL; fs = fs->next) {
        if (strncmp(fs->name, name, FS_NAME_MAXLEN) == 0)
            break;
    }
    if (!fs)
        return -ENOENT;  // No fs with name

    // Find superblock
    for (size_t i = 0; i < N_SUPERBLOCK; i++) {
        if (superblocks[i].fs == NULL) {
            superblk = superblocks + i;
            break;
        }
    }

    if (!superblk) {
        LOG("Out of superblocks");
        return -ENOMEM;
    }

    // Find inode to mount upon, since we never call put on success, the mountpoint will in memory
    ret = pathwalk(vfs_root, path, &inode);
    if (ret < 0) {
        return ret;
    }

    // TODO: Check if already mounted (and busy?)
    if (!S_ISDIR(inode->mode)) {
        put_node(inode);
        return -ENOTDIR;
    }

    // Needs to be assigned here so that the fs implementation can call get_inode within
    // mount()
    superblk->fs = fs;

    ret = fs->ops->mount(superblk, data);
    if (ret < 0) {
        superblk->fs = NULL;  // Mark superblock as free
        put_node(inode);
        return ret;
    }

    // Ensure that the superblock contains a valid root inode
    if (!superblk->root_inode || !superblk->root_inode->id ||
        !S_ISDIR(superblk->root_inode->mode) || superblk->root_inode->count != 1) {
        superblk->fs = NULL;  // Mark superblk as free
        put_node(inode);
        return -EINVAL;
    }

    // 'Glue' the mounted filesystems together
    inode->mountpoint       = true;
    superblk->mounted_inode = inode;

    // Add superblk to list the per fs mountlist
    superblk->next = fs->mounts;
    fs->mounts     = superblk;
    return 0;
}

int fs_init(struct boot_data* boot_data)
{
    int ret;

    // TODO: Add a list of all file systems to be registered at boot

    ret = register_fs(&romfs);
    if (ret < 0) {
        LOG("Failed to register romfs %i", ret);
        return ret;
    }

    // Mount initrd
    struct romfs_mount_data intird_mnt_data = {
        .data = (char*)P2L(boot_data->initrd_start),
        .size = boot_data->initrd_size,
    };

    ret = mount_rootfs(ROMFS_FS_NAME, &intird_mnt_data);
    if (ret < 0) {
        LOG("Failed to mount initrd as rootfs: %i", ret);
        return ret;
    }

    ret = mount_sysfs("/sys");
    if (ret < 0) {
        LOG("Failed to mount sysfs %i", ret);
        return ret;
    }

    return 0;
}

