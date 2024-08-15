#ifndef FS_INTERNALS_H
#define FS_INTERNALS_H
#include <fs.h>
#include <posix/errno.h>
#include <utils.h>

#define DEBUG_FS 1

#if DEBUG_FS
#define LOG(...) log("[FS]: " __VA_ARGS__)
#else
#define LOG(...)
#endif

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
    // TODO: Once multithreaded, give each node a spinlock
};

/*
    Helper function for iterating over the vfs tree. Find the node at path and sets the pointer
    passed in node_path to the part of the path that is within the node.
 */
struct vfs_node* search_vfs(char* path, char** node_path);

/*
    Gets the inode with id for a certain vfs_node. Ensures that the inode is properly read and
    initalized. Returns the inode object on success, NULL if cache is empty.
*/
struct inode* get_inode(struct vfs_node* vfs_node, unsigned int id);

/* Hands a no longer used inode back to cache. */
void put_node(struct inode* node);

/* Finds a free fd with the task and allocates an open file object from the global table.
   Returns the found fd and sets the file object pointer.
 */
int alloc_fd(struct task_fs_data* task_data, struct open_file** file);

/* Frees the open file associated with the fd. Returns 0 on success, else errno. */
int free_fd(struct task_fs_data* task_data, int fd);

#endif /* FS_INTERNALS_H */
