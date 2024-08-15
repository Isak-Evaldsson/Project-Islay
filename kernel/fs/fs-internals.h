#ifndef FS_INTERNALS_H
#define FS_INTERNALS_H
#include <fs.h>
#include <posix/errno.h>
#include <utils.h>

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

/*
    Is needed to allow a future fork implementation to create child processes that
    inherit the parents open files.
*/
struct open_file {
    int           ref_count;  // How many process are referring to this object with their fd table
    int           offset;     // What position within the file are we currently at
    struct inode* inode;      // Which file does the object correspond to
    struct file_info info;    // File information passed to the fs implementation
};

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
