/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef FS_ROMFS_H
#define FS_ROMFS_H

#include "../fs-internals.h"

/* Exposing the fs struct so it can be registered */
extern struct fs romfs;

#define ROMFS_FS_NAME "romfs"

/* Mountpoint data for romfs */
struct romfs_mount_data {
    char*  data;
    size_t size;
    size_t start;
};

#endif /* FS_ROMFS_H */
