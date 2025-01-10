#include <arch/paging.h>

#include "fs-internals.h"

/* Connects a file to its directory file. Returns 0 on success, else -ERRNO */
int add_pseudo_file(struct pseudo_file* dir, struct pseudo_file* file)
{
    if (!S_ISDIR(dir->mode)) {
        return -ENOTDIR;
    }

    file->sibling = dir->child;
    file->parent  = dir;
    dir->child    = file;
    return 0;
}

/* Ensure all the fields in the pseudo file to be properly initalized */
void init_pseudo_file(struct pseudo_file* file, mode_t mode, const char* name)
{
    kassert(mode != 0);
    kassert(name && *name != '\0');

    strcpy(file->name, name);
    file->inode = (ino_t)file;
    file->mode  = mode;

    file->parent  = NULL;
    file->sibling = NULL;
    file->child   = NULL;
}

/* Generic readdir function for pseudo filesystems */
int pseudo_file_readdir(const struct open_file* file, struct dirent* dirent, off_t offset)
{
    struct pseudo_file* node;
    struct pseudo_file* dir = (struct pseudo_file*)file->inode->id;

    // Use offset to store the current state of the directory iteration
    // 0:   this node "." file
    // 1:   parent node ".." file
    // >1:  the pointer to the next child
    if (offset == 0) {
        dirent->d_ino = dir->inode;
        strcpy(dirent->d_name, ".");
        return 1;

    } else if (offset == 1) {
        if (dir->parent) {
            dirent->d_ino = dir->parent->inode;
        } else {
            // Special case since we hit a boundary between two file systems
            dirent->d_ino = file->inode->super->mounted_inode->id;
        }

        strcpy(dirent->d_name, "..");

        // Due to off_t being a signed 32-bit number on x86, higher half addresses will be converted
        // to a negative value. To go around this, the linear physical address is used
        return (dir->child) ? (off_t)L2P(dir->child) : 0;
    }
    node          = (struct pseudo_file*)P2L(offset);
    dirent->d_ino = node->inode;
    strcpy(dirent->d_name, node->name);

    if (node->sibling) {
        return (off_t)L2P(node->sibling);
    }
    return 0;
}

/* Generic fetch_inode function for pseudo filesystems */
int pseudo_fetch_inode(const struct superblock* super, ino_t id, struct inode* inode)
{
    (void)super;
    struct pseudo_file* file = (struct pseudo_file*)id;

    // Verify coherent inode ids
    if (file->inode != id) {
        return -EINVAL;
    }

    inode->mode = file->mode;
    return 0;
}
