#include "fs-internals.h"

/* In memory inode cache. */
static struct inode inode_table[MAX_OPEN_GLOBAL];

/*
    Gets the inode with id for a certain vfs_node. Ensures that the inode is properly read and
    initalized. Returns the inode object on success, NULL if cache is empty.
*/
struct inode *get_inode(struct vfs_node *vfs_node, unsigned int id)
{
    struct inode *inode, *free = NULL;

    // Search table for the desired node if cached, otherwise we find a new free node
    for (inode = inode_table; inode < END_OF_ARRAY(inode_table); inode++) {
        if (inode->count > 0) {
            if (inode->vfs_node == vfs_node && inode->id == id) {
                inode->count++;
                return inode;
            }
        } else {
            free = inode;
        }
    }

    // If there's no space left in the cache, return an empty node
    if (free == NULL) {
        return NULL;
    }

    // TODO: Handle reading of inode data from the actual fs
    free->id          = id;
    free->vfs_node    = vfs_node;
    free->count       = 1;
    free->inode_dirty = false;
    return free;
}

/* Hands a no longer used inode back to cache. */
void put_node(struct inode *node)
{
    node->count--;
    // TODO: Add logic for inode write-back if node->count == 0 && node->inode_dirty
}

void sysfs_dump_inodes()
{
    sysfs_writer("inode cache:\n");
    for (struct inode *inode = inode_table; inode < END_OF_ARRAY(inode_table); inode++) {
        if (inode->count > 0) {
            sysfs_writer("  (0x%x) id: %u, count: %u, vfs_node: 0x%x\n", inode, inode->id,
                         inode->count, inode->vfs_node);
        }
    }
}
