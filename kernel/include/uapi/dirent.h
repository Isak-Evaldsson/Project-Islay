/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef POSIX_DIRENT_H
#define POSIX_DIRENT_H
#include "limits.h"
#include "sys/types.h"
/*
    POSIX defined defined directory entries

    see https://pubs.opengroup.org/onlinepubs/007908799/xsh/dirent.h.html
*/

struct dirent {
    ino_t d_ino;
    char  d_name[NAME_MAX + 1];
};

#endif /* POSIX_DIRENT_H */
