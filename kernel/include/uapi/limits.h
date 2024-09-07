/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef POSIX_LIMITS_H
#define POSIX_LIMITS_H
#include <stdint.h>
/*
    types.h: POSIX defined constants

    see https://pubs.opengroup.org/onlinepubs/009695399/basedefs/limits.h.html
*/

/* Maximum number of bytes in a filename (not including terminating null). */
#define NAME_MAX (255)

/* Maximum number of bytes in a pathname, including the terminating null character. */
#define PATH_MAX (4096)

#endif /* POSIX_LIMITS_H */
