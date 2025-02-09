/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#ifndef FS_DEVFS_H
#define FS_DEVFS_H

/*
    devfs - The glue that ties devices into the filesystem in a unixy way
*/

#include "../fs-internals.h"

#define DEVFS_FS_NAME "devfs"

/* Exposing the fs struct so it can be registered */
extern struct fs devfs;

#endif /* FS_DEVFS_H */
