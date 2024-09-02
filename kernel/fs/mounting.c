#include <libc.h>
#include <utils.h>

#include "fs-internals.h"

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

/* Helper function for the mounting procedure that allocates a superblock for the fs specfied in by
 * fs_name. Returns a superblock on success, on failure returns NULL and sets errno. */
static struct superblock* alloc_superblock(const char* fs_name, int* errno)
{
    struct fs*         fs = NULL;
    struct superblock* superblk;

    for (fs = fs_list; fs != NULL; fs = fs->next) {
        if (strncmp(fs->name, fs_name, FS_NAME_MAXLEN) == 0)
            break;
    }

    if (!fs) {
        *errno = -ENOENT;
        return NULL;
    }

    // Find superblock
    for (superblk = superblocks; superblk < END_OF_ARRAY(superblocks); superblk++) {
        if (superblk->fs == NULL) {
            superblk->fs = fs;  // Mark superblock as allocated
            return superblk;
        }
    }

    *errno = -ENOMEM;
    LOG("Out of superblocks");
    return NULL;
}

/* Helper functions that performs the actual mounting by calling the fs implementation and "gluing"
 * the inodes together. Returns 0 on success or -ERRNO on failure */
static int mount_helper(void* data, struct superblock* superblk, struct inode* mnt_inode)
{
    int   ret;
    ino_t root;

    ret = superblk->fs->ops->mount(superblk, data, &root);
    if (ret < 0) {
        return ret;
    }

    superblk->root_inode = get_inode(superblk, root, &ret);
    if (!superblk->root_inode) {
        return ret;
    }

    // Ensure that the superblock contains a valid root inode
    if (!S_ISDIR(superblk->root_inode->mode) || superblk->root_inode->count != 1) {
        put_node(superblk->root_inode);
        return -EINVAL;
    }

    // 'Glue' the mounted filesystems together
    if (mnt_inode) {
        mnt_inode->mountpoint   = true;
        superblk->mounted_inode = mnt_inode;
    }

    // Add superblk to list the per fs mountlist
    superblk->next       = superblk->fs->mounts;
    superblk->fs->mounts = superblk;
    return 0;
}

int mount_rootfs(char* name, void* data)
{
    int                ret;
    struct superblock* superblk;

    superblk = alloc_superblock(name, &ret);
    if (!superblk) {
        return ret;
    }

    ret = mount_helper(data, superblk, NULL);
    if (ret < 0) {
        superblk->fs = NULL;  // Mark superblock as free
        return ret;
    }

    vfs_root = superblk->root_inode;
    return 0;
}

int mount(const char* path, const char* name, void* data)
{
    int                ret;
    struct inode*      inode;
    struct superblock* superblk;

    if (*path != '/')
        return -EINVAL;

    superblk = alloc_superblock(name, &ret);
    if (!superblk) {
        return ret;
    }

    // Find inode to mount upon, since we never call put on success, the mountpoint will in memory
    ret = pathwalk(vfs_root, path, &inode);
    if (ret < 0) {
        superblk->fs = NULL;  // Mark superblock as free
        return ret;
    }

    // TODO: Check if already mounted (and busy?)
    if (!S_ISDIR(inode->mode)) {
        superblk->fs = NULL;  // Mark superblock as free
        put_node(inode);
        return -ENOTDIR;
    }

    ret = mount_helper(data, superblk, inode);
    if (ret < 0) {
        superblk->fs = NULL;  // Mark superblock as free
        put_node(inode);
        return ret;
    }
    return 0;
}

/* To ensure that the per task fs data struct is properly initialised */
void task_data_init(struct task_fs_data* task_data)
{
    memset(task_data->file_table, 0, sizeof(task_data->file_table));
    task_data->rootdir = vfs_root;
    task_data->workdir = vfs_root;  // Or begin in home folder?
}
