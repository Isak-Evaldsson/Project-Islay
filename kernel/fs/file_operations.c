/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include "fs-internals.h"

int open(struct task_fs_data* task_data, const char* path, int oflag)
{
    int               fd, ret;
    struct open_file* file;
    struct inode*     inode;

    fd = alloc_fd(task_data, &file);
    if (fd < 0) {
        return fd;
    }

    ret = pathwalk(*path == '/' ? task_data->rootdir : task_data->workdir, path, &inode);
    if (ret < 0) {
        return ret;
    }

    if ((oflag & O_DIRECTORY) && !S_ISDIR(inode->mode)) {
        put_node(inode);
        return -ENOTDIR;
    }

    file->file_ops = inode->super->fs->ops;
    file->inode    = inode;

    if (file->file_ops->open) {
        ret = file->file_ops->open(file);
        if (ret < 0) {
            put_node(inode);
            return ret;
        }
    }

    file->ref_count           = 1;  // Marks the file object as allocated
    task_data->file_table[fd] = file;
    return fd;
}

int close(struct task_fs_data* task_data, int fd)
{
    int ret;

    // TODO: Do we need to call back to fs at close?
    ret = free_fd(task_data, fd);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

static ssize_t rw_helper(struct task_fs_data* task_data, int fd, void* buf, size_t nbyte,
                         off_t offset, bool use_file_offset, bool write)
{
    int               rw_bytes;
    off_t             rw_offset;
    struct open_file* file;

    if (fd < 0 || fd >= MAX_OPEN_PER_PROC) {
        return -EBADF;
    }

    file = task_data->file_table[fd];
    if (!file) {
        return -EBADF;
    }

    if (S_ISDIR(file->inode->mode)) {
        return -EISDIR;
    }

    if (!(file->oflags & O_RDWR)) {
        if (write && !(file->oflags & O_WRONLY)) {
            return -EPERM;
        } else if (!write && !(file->oflags & O_RDONLY)) {
            return -EPERM;
        }
    }

    rw_offset = use_file_offset ? file->offset : offset;

    if (write && use_file_offset && (file->oflags & O_APPEND)) {
        // TODO: Adjust rw_offset to end of file
    }

    // TODO: Should we count the offset/bytes to ensure that we don't over-read or should we hand
    // over the responsibilty to the fs implementation?
    if (write) {
        if (file->inode->super->flags & MOUNT_READONLY) {
            return -EPERM;  // can't write to a readonly fs
        }
        rw_bytes = file->file_ops->write(buf, nbyte, rw_offset, file);
    } else {
        rw_bytes = file->file_ops->read(buf, nbyte, rw_offset, file);
    }
    if (rw_bytes < 0) {
        return rw_bytes;
    }

    if (use_file_offset) {
        file->offset += rw_bytes;
    }

    return rw_bytes;
}

ssize_t pwrite(struct task_fs_data* task_data, int fd, const void* buf, size_t count, off_t offset)
{
    return rw_helper(task_data, fd, buf, count, offset, false, true);
}

ssize_t write(struct task_fs_data* task_data, int fd, const void* buf, size_t count)
{
    return rw_helper(task_data, fd, buf, count, 0, true, true);
}

ssize_t pread(struct task_fs_data* task_data, int fd, void* buf, size_t nbyte, off_t offset)
{
    return rw_helper(task_data, fd, buf, nbyte, offset, false, false);
}

ssize_t read(struct task_fs_data* task_data, int fd, void* buf, size_t nbyte)
{
    return rw_helper(task_data, fd, buf, nbyte, 0, true, false);
}

int readdirents(struct task_fs_data* task_data, int fd, struct dirent* buf, int buf_count)
{
    off_t             offset;
    int               count;
    struct open_file* file;
    struct dirent*    dirent;

    if (fd < 0 || fd >= MAX_OPEN_PER_PROC) {
        return -EBADF;
    }

    file = task_data->file_table[fd];
    if (!file) {
        return -EBADF;
    }

    if (!S_ISDIR(file->inode->mode) || !file->file_ops->read) {
        return -ENOTDIR;
    }

    count  = 0;
    offset = file->offset;
    if (offset == EOF) {
        count = 0;  // No more dirents left to read
        goto end;
    }

    // Iterate over the dir until the buffer is full or EOF is reached. Counter needs to be separate
    // from index variable since its count + 1 when there are more files to read and count when
    // we're on the last iteration
    for (int i = 0; i < buf_count; i++) {
        dirent = buf + i;
        offset = file->file_ops->readdir(file, dirent, offset);
        if (offset < 0) {
            return offset;
        }

        // Verify the dirent object
        kassert(dirent->d_ino != 0 && dirent->d_name[0] != '\0');
        count++;

        if (offset == 0) {
            offset = EOF;
            break;
        }
    }

    // Store offset so we can continue if the buffer is full at the next call
    file->offset = offset;

end:
    return count;
}
