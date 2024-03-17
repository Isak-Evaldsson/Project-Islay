#ifndef FS_INTERNALS_H
#define FS_INTERNALS_H
#include <fs.h>

typedef enum {
    VFS_NODE_TYPE_DIR,
    VFS_NODE_TYPE_MNT,
} vfs_node_type_t;

struct vfs_node {
    char             name[FS_NAME_MAXLEN + 1];
    vfs_node_type_t  type;
    struct fs*       fs;
    struct vfs_node* parent;
    struct vfs_node* child;
    struct vfs_node* sibling;
};

#endif /* FS_INTERNALS_H */
