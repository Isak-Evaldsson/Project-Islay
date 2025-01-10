/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#ifndef FS_KINFO_H
#define FS_KINFO_H

/*
    kinfo - a filesystem exposing kernel information the userspace.
*/

#include "../fs-internals.h"

#define KINFO_FS_NAME "kinfo"

/* Exposing the fs struct so it can be registered */
extern struct fs kinfo;

/*
    Kinfo API
*/

/* Passed to the kinfo_write() so it knows what buffer to write to */
struct kinfo_buffer;

/* Object representing a kinfo pseudo file */
struct kinfo_file;

/* Function called when a file within kinfo is read */
typedef void (*kinfo_read_t)(struct kinfo_buffer* buff);

/*
    Creates a file within kinfo relative to the specified directory (or within fs root if NULL).
    On success it returns 0 and sets the result_ptr, otherwise it returns -ERRNO.
*/
int kinfo_create_file(struct kinfo_file* dir, struct kinfo_file** result_ptr, const char* name,
                      mode_t mode, kinfo_read_t read);

/* Prints the data to be read when reading a kinfo file */
void kinfo_write(struct kinfo_buffer* buff, const char* restrict format, ...);

#endif /* FS_KINFO_H */
