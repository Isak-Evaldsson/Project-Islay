#include <libc.h>
#include <utils.h>

#include "fs-internals.h"

/*
    VFS main data structures
*/
static struct fs* fs_list = NULL;

static struct vfs_node root = {
    .type    = VFS_NODE_TYPE_DIR,
    .name    = "/",
    .fs      = NULL,
    .parent  = NULL,
    .child   = NULL,
    .sibling = NULL,
};

/* TODO: Hashtable or similar to make it fast to access open files. Each process maps
 * file-descriptors to indices within the table. */

#if DEBUG_FS
static void dump_fs_list()
{
    for (struct fs* fs = fs_list; fs != NULL; fs = fs->next) {
        kprintf("  (%x) name: %s, ops: %x\n", fs, fs->name, fs->ops);
    }
}

static void dump_vfs_node(struct vfs_node* node, int indent)
{
    for (int i = 0; i < indent; i++) kprintf("  ");

    kprintf("%s type: %u fs: %x (%x)\n", node->name, node->type, node->fs, node);
    for (node = node->child; node != NULL; node = node->sibling) {
        dump_vfs_node(node, indent + 1);
    }
}

void dump_vfs()
{
    kprintf("VFS Dump\n");
    kprintf("Registered file systems:\n");
    dump_fs_list();
    kprintf("\n");

    kprintf("VFS tree:\n");
    dump_vfs_node(&root, 0);
}
#endif

static int create_vfs_node(struct vfs_node* parent, char* name, struct vfs_node** new_node)
{
    struct vfs_node* node;
    size_t           len = strlen(name);

    if (!new_node)
        return -EINVAL;

    if (!len || len > FS_NAME_MAXLEN) {
        return -ENAMETOOLONG;
    }

    node = kmalloc(sizeof(struct vfs_node));
    if (!node)
        return -ENOMEM;

    node->parent = parent;
    node->child  = NULL;
    node->type   = VFS_NODE_TYPE_DIR;

    // Append node to child list
    node->sibling = parent->child;
    parent->child = node;

    memset(node->name, 0, sizeof(node->name));
    memcpy(node->name, name, len);

    *new_node = node;
    return 0;
}

static struct vfs_node* search_dir_node(struct vfs_node* parent, const char* name)
{
    struct vfs_node* node = parent->child;
    while (node != NULL) {
        if (strcmp(node->name, name) == 0) {
            break;
        }
        node = node->sibling;
    }
    return node;
}

static void remove_empty_dir_nodes(struct vfs_node* root)
{
    struct vfs_node *prev, *dead, *node = root->child;

    while (node != NULL) {
        if (node->type == VFS_NODE_TYPE_DIR) {
            if (node->child == NULL) {
                // Empty dir node, remove
                dead = node;
                if (dead == root->child) {
                    root->child = dead->sibling;
                    if (!root->child && root->parent) {
                        // Since we emptied the folder, we might have made the parent folder empty
                        // as well, walk up the tree to clear newly created empty dir
                        remove_empty_dir_nodes(root->parent);
                    }

                } else {
                    prev->sibling = dead->sibling;
                }

                // update node but not prev since we deleted the node that would be the new prev
                node = dead->sibling;
                kfree(dead);
                continue;

            } else {
                // might be an empty node further down the tree, continue...
                remove_empty_dir_nodes(node);
            }
        }

        prev = node;
        node = node->sibling;
    }
}

/*
    Helper function for iterating over the vfs tree. Find the node at path and sets the pointer
    passed in node_path to the part of the path that is within the node.
 */
struct vfs_node* search_vfs(char* path, char** node_path)
{
    struct vfs_node* node;
    char*            token;

    token = strtok(path, "/", node_path);
    if (!*token) {
        return &root;
    }

    node = &root;
    while (*token) {
        LOG("Token: %s node: %s %s", token, node->name, *node_path);
        node = search_dir_node(node, token);

        // If the node can't be found, or we have reached a mountpoint, exit
        if (!node || node->type == VFS_NODE_TYPE_MNT) {
            LOG("Here: 0x%x", node);
            goto end;
        }

        token = strtok(NULL, "/", node_path);
    }

end:
    return node;
}

static int check_required_fs_ops(const struct fs_ops* ops)
{
    if (!ops->mount)
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
    //  How to handle mounting a file system at root and then having the test as subfolders, eg.
    //  / (ramfs)
    //  |_ /dev (devfs)
    //  |_ /sys (sysf
    //  Then when doing a call to for example open /other/file, then when the function going through
    //  the vfs searches the root dir and not finding the other node, instead of saying, "oh no node
    //  found", we simply fallback to the root file system and call ramfs->open("other/file")
    //
    //  Can that be generalised to the whole tree or just the rootfs?
    int              res;
    char *           token, *token_ptr, *ppath;
    struct fs*       fs;
    struct vfs_node *node, *parent;

    if (*path != '/')
        return -EINVAL;

    for (fs = fs_list; fs != NULL; fs = fs->next) {
        if (strncmp(fs->name, name, FS_NAME_MAXLEN) == 0)
            break;
    }
    if (!fs)
        return -ENOENT;  // No fs with name

    ppath = strdup(path);
    token = strtok(ppath, "/", &token_ptr);
    if (!*token) {
        // TODO: Handle mount a fs at root
        LOG("Mounting root file systems not yet implemented\n");
        res = -ENOTSUP;
        goto end;
    }

    parent = &root;
    while (*token) {
        node = search_dir_node(parent, token);
        if (!node) {
            res = create_vfs_node(parent, token, &node);
            if (res) {
                LOG("mount error: Failed to allocate node %i\n", res);
                goto end;
            }
        }

        if (node->type == VFS_NODE_TYPE_MNT) {
            LOG("mount error: Trying to mount at already mounted mountpoint %s\n", path);
            res = -EEXIST;
            goto end;
        }

        kassert(node->type == VFS_NODE_TYPE_DIR);
        parent = node;
        token  = strtok(NULL, "/", &token_ptr);
        if (node->child && !*token)
            LOG("Warning, mounting a non-empty dir %x\n", node);
    }

    res = fs->ops->mount(data);
    if (res) {
        LOG("mount error: Failed to call mount on fs %x with data %x (%i)\n", fs, data, res);
        res = -EIO;
        goto end;
    }

    // Only change node state when we know we're successful
    node->type = VFS_NODE_TYPE_MNT;
    node->fs   = fs;
    res        = 0;

end:
    // If the mounting failed, we might have created empty dirs that needs to be cleaned up. We
    // don't want a failed mount to result in an changed vfs tree
    if (res)
        remove_empty_dir_nodes(&root);

    kfree(ppath);
    return res;
}
