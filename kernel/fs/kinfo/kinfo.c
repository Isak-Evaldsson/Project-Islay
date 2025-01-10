/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#include <arch/paging.h>
#include <memory/vmem_manager.h>

#include "kinfo.h"

/* Passed to the kinfo_write() so it knows what buffer to write to */
struct kinfo_buffer {
    char*  buff;
    size_t size;
    off_t  offset;
};

/* Object representing a kinfo pseudo file */
struct kinfo_file {
    struct pseudo_file file;
    kinfo_read_t       read;
};

static struct kinfo_buffer read_buffer;

static struct kinfo_file root = {
    .file = {.inode   = (ino_t)&root.file,
             .mode    = S_IFDIR,
             .name    = "",
             .parent  = NULL,
             .child   = NULL,
             .sibling = NULL}
};

static int kinfo_read(char* buf, size_t size, off_t offset, struct open_file* file)
{
    struct kinfo_file* f = GET_STRUCT(struct kinfo_file, file, GET_PSEUDO_FILE(file));

    // Reset global buffer
    read_buffer.offset = 0;
    memset(read_buffer.buff, '\0', read_buffer.size);

    // Dump the whole 'file'
    f->read(&read_buffer);

    // Then copy according to size and offset
    size_t read_size = size;
    if (offset + size > read_buffer.size) {
        read_size = read_buffer.size - offset;
    }

    memcpy(buf, read_buffer.buff + offset, read_size);
    return read_size;
}

static int kinfo_mount(struct superblock* super, void* data, ino_t* root_ptr)
{
    (void)data;
    (void)super;

    read_buffer.buff = (char*)vmem_request_free_page(0);
    read_buffer.size = PAGE_SIZE;
    if (!read_buffer.buff) {
        return -ENOMEM;
    }

    *root_ptr = root.file.inode;
    return 0;
}

static struct fs_ops kinfo_ops = {
    .mount       = kinfo_mount,
    .read        = kinfo_read,
    .readdir     = pseudo_file_readdir,
    .fetch_inode = pseudo_fetch_inode,
};

DEFINE_FS(kinfo, KINFO_FS_NAME, &kinfo_ops);

/*
    kinfo API
*/

/*
    Creates a file within kinfo relative to the specified directory (or within fs root if NULL). On
    success it returns 0 and sets the result_ptr, otherwise it returns -ERRNO.
*/
int kinfo_create_file(struct kinfo_file* dir, struct kinfo_file** result_ptr, const char* name,
                      mode_t mode, kinfo_read_t read)
{
    int                ret;
    struct kinfo_file* file;

    if (!(S_ISDIR(mode) || (S_ISREG(mode) && read))) {
        return -EINVAL;
    }

    if (!dir) {
        dir = &root;
    }

    file = kalloc(sizeof(struct kinfo_file));
    if (!file) {
        return -ENOMEM;
    }
    init_pseudo_file(&file->file, mode, name);

    ret = add_pseudo_file(&dir->file, &file->file);
    if (ret < 0) {
        kfree(file);
        return ret;
    }

    if (S_ISREG(mode)) {
        file->read = read;
    }
    *result_ptr = file;
    return 0;
}

/* Prints the data to be read when reading a kinfo file */
void kinfo_write(struct kinfo_buffer* buff, const char* restrict format, ...)
{
    size_t  nbytes;
    char*   start = buff->buff + buff->offset;
    size_t  size  = buff->size - buff->offset;
    va_list args;

    va_start(args, format);
    nbytes = vsnprintf(start, size, format, args);
    va_end(args);

    if (nbytes > 0) {
        buff->offset += nbytes;
    }
}
