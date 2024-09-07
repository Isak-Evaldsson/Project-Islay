/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   open_file/flip table based on Minix 2, Copyright (C) 1987,1997, Prentice Hall

   Copyright (C) 2024 Isak Evaldsson
*/
#include "fs-internals.h"

/*
    Statically allocated table of all open files
    TODO: Once multi-threaded, protect with lock
*/
static struct open_file open_files[MAX_OPEN_GLOBAL];

/* Finds a free fd with the task and allocates an open file object from the global table.
   Returns the found fd and sets the file object pointer.
 */
int alloc_fd(struct task_fs_data *task_data, struct open_file **file)
{
    int fd = -1;

    // Find fd
    for (size_t i = 0; i < MAX_OPEN_PER_PROC; i++) {
        if (task_data->file_table[i] == NULL) {
            fd = i;
            break;
        }
    }

    if (fd < 0) {
        return -ENFILE;
    }

    // Find a free open file object
    for (struct open_file *f = open_files; f < END_OF_ARRAY(open_files); f++) {
        if (f->ref_count == 0) {
            // NOTE: We're not setting the ref_count field since the file opening process may fail
            // latter
            f->offset = 0;

            *file = f;
            return fd;
        }
    }

    return -ENFILE;
}

/* Frees the open file associated with the fd. Returns 0 on success, else errno. */
int free_fd(struct task_fs_data *task_data, int fd)
{
    struct open_file *file;

    if (fd < 0 || fd >= MAX_OPEN_PER_PROC) {
        return -EBADF;
    }

    file = task_data->file_table[fd];
    if (!file) {
        return -EBADF;
    }

    // Should we treat this as en error case?
    if (file->ref_count == 0) {
        return -EBADF;
    }

    // If the is the last task keeping this file open, free it's inode
    if (file->ref_count == 1) {
        put_node(file->inode);
        file->inode == NULL;
    }

    file->ref_count--;
    task_data->file_table[fd] = NULL;
    return 0;
}

void sysfs_dump_open_files(char *buff, size_t size)
{
    sysfs_writer("open_files table:\n");
    for (struct open_file *f = open_files; f < END_OF_ARRAY(open_files); f++) {
        if (f->ref_count > 0) {
            sysfs_writer("  (0x%x) ref_count: %u, offset %u, inode: 0x%x, file_ops: 0x%x\n", f,
                         f->ref_count, f->offset, f->inode, f->file_ops);
        }
    }
}
