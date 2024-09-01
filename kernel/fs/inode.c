#include "fs-internals.h"

/* In memory inode cache. */
static struct inode inode_table[MAX_OPEN_GLOBAL];

/* Checks that the inode is correctly filed in by the fs implementation. Returns -ERRN0 or 0 on
 * success */
static int verify_inode(const struct inode *inode)
{
    // Must have a file type
    if ((inode->mode & S_IFMT) == 0) {
        return -EINVAL;
    }

    return 0;
}

/*
    Gets the inode with id for a certain superblock. Ensures that the inode is properly read and
    initalized. If successful it returns the inode, on failure it returns NULL and fills the errno
    variable with -ERRNO.
*/
struct inode *get_inode(const struct superblock *super, ino_t id, int *errno)
{
    struct inode *inode, *free = NULL;
    *errno = 0;  // Initial assume success

    // Search table for the desired node if cached, otherwise we find a new free node
    for (inode = inode_table; inode < END_OF_ARRAY(inode_table); inode++) {
        if (inode->count > 0) {
            if (inode->super == super && inode->id == id) {
                inode->count++;
                return inode;
            }
        } else {
            free = inode;
        }
    }

    // If there's no space left in the cache, return an empty node
    if (free == NULL) {
        *errno = -ENOENT;
        return NULL;
    }

    *errno = super->fs->ops->fetch_inode(super, id, free);
    if (*errno < 0) {
        return NULL;
    }

    *errno = verify_inode(free);
    if (*errno < 0) {
        return NULL;
    }

    free->id          = id;
    free->super       = super;
    free->count       = 1;
    free->inode_dirty = false;
    free->mountpoint  = false;

    return free;
}

/*
    Get a copy of a inode that already exits in memory.

    Allow one the later call put on it without messing upp the refcount;
 */
struct inode *clone_inode(struct inode *inode)
{
    kassert(inode->count > 0);
    inode->count++;

    return inode;  // To make a more neat api allowing: inode = clone_inode(old);
}

/* Hands a no longer used inode back to cache. */
void put_node(struct inode *node)
{
    kassert(node->count > 0);
    node->count--;

    if (node->count == 0) {
        // Most inodes have a short life time, however some inodes needs to be kept i memory for the
        // vfs glue to work properly.
        kassert(node != vfs_root);
        kassert(!node->mountpoint);

        // TODO: Add logic for inode write-back if node->inode_dirty
    }
}

void sysfs_dump_inodes()
{
    sysfs_writer("inode cache:\n");
    for (struct inode *inode = inode_table; inode < END_OF_ARRAY(inode_table); inode++) {
        if (inode->count > 0) {
            sysfs_writer("  (%x) id: %u, count: %u, mode: %u, super: %x, mnt: %u\n", inode,
                         inode->id, inode->count, inode->mode, inode->super, inode->mountpoint);
        }
    }
}
