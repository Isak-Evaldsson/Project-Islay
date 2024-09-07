/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <uapi/limits.h>

#include "fs-internals.h"

/*
    Contains all path information needed by the pathwalker. Once, user-space support is implemented
    for the fs, this and its associated methods will also be responsible for validation and copying
    of userspace data.
*/
struct path {
    const char* path;  // Not strictly needed, but nice for debugging/error messages
    char*       current_token;
    char*       next_token;
    char        path_buff[PATH_MAX];
};

/* Currently paths is only allowed to contain [a-zA-z0-9] */
static bool valid_path_char(char c)
{
    // To allow for even more chars without this becomes unbearably long, replace with a lookup
    // table
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '/';
}

static int path_init(const char* path, struct path* path_obj)
{
    size_t len;

    // Copy, compute length and verify chars all in one iteration
    for (len = 0; len < PATH_MAX && path[len] != '\0'; len++) {
        char c = path[len];

        // TODO: Do we want fs implementations to have their own validation as well?
        if (!valid_path_char(c)) {
            return -EINVAL;
        }
        path_obj->path_buff[len] = c;
    }

    path_obj->path_buff[len] = '\0';
    path_obj->path           = path;
    path_obj->current_token  = NULL;
    path_obj->next_token     = path_obj->path_buff;
    return 0;
}

/*
    Iterate the path by advancing the current and next pointer one step
*/
static char* path_next(struct path* path)
{
    if (path->current_token == NULL) {
        path->current_token = strtok(path->path_buff, "/", &path->next_token);
    } else {
        path->current_token = strtok(NULL, "/", &path->next_token);
    }
    return path->current_token;  // Not needed, but it makes the api a bit neater
}

/*
    Iterates over the path until the correct inode is found. On success it returns 0 and sets the
    inode_ptr correctly, otherwise it returns -ERRNO.
*/
int pathwalk(struct inode* root, const char* path, struct inode** inode_ptr)
{
    int           ret;
    struct inode* inode;

    // Or static alloc?
    struct path* path_obj = kmalloc(sizeof(struct path));
    if (!path_obj) {
        return -ENOMEM;
    }

    ret = path_init(path, path_obj);
    if (ret < 0) {
        return ret;
    }

    // Inode reference needs to be cloned since it later will be called put on in either this
    // function or later call to close
    if (root->mountpoint) {
        struct superblock* super = find_superblock(root);
        if (!super) {
            kpanic("Critical fs failure: inode %x marked mounted by has no superblock\n", inode);
        }
        inode = clone_inode(super->root_inode);
    } else {
        inode = clone_inode(root);
    }

    // What if out root node is a mountpoint?

    while (*path_next(path_obj)) {
        ino_t            next   = 0;
        off_t            offset = 0;
        struct dirent    dirent;
        struct open_file file = {.inode = inode};

        if (!S_ISDIR(inode->mode)) {
            ret = -ENOENT;
            goto error;
        }

        // Search dir for the next inode
        do {
            offset = inode->super->fs->ops->readdir(&file, &dirent, offset);
            if (offset < 0) {
                ret = offset;
                goto error;
            }

            if (!strcmp(dirent.d_name, path_obj->current_token)) {
                // Success, mark the next inode ot load
                next = dirent.d_ino;
                break;
            }

        } while (offset > 0);

        // Found our inode, load the next one
        if (next > 0) {
            struct inode* new = get_inode(inode->super, next, &ret);
            if (!inode) {
                goto error;  // the current inode will be free'd in error
            }

            if (new->mountpoint) {
                struct superblock* super = find_superblock(new);
                if (!super) {
                    kpanic("Critical fs failure: inode %x marked mounted by has no superblock\n",
                           inode);
                }

                // Replace the current inode, inode needs to be cloned since it will free'd later in
                // the pathwalking loop
                put_node(new);
                new = clone_inode(super->root_inode);
            }

            put_node(inode);
            inode = new;
            continue;
        }

        // No direntry found, put there's still a token left
        ret = -ENOENT;
        goto error;
    }

    // If we passed through the loop without errors, then we found the inode.
    // NOTE; On success, the caller is responsible for freeing the inode.
    *inode_ptr = inode;
    return 0;

error:
    // Free current inode,
    put_node(inode);

    // Ensure no inode leakage
    *inode_ptr = NULL;
    return ret;
}
