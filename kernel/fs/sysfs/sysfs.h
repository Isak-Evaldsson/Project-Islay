/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef FS_SYSFS_H
#define FS_SYSFS_H
/*
    sysfs - a filesystem exposing kernel info the userspace.

    Conceptually similar to sys/procfs in Linux.
*/

#include "../fs-internals.h"

#define SYSFS_FS_NAME "sysfs"

/* Exposing the fs struct so it can be registered */
extern struct fs sysfs;

/* Writes formatted strings to the global sysfs read buffer */
void sysfs_writer(const char* restrict format, ...);

#endif /* FS_SYSFS_H */
