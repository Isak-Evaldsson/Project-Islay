/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef POSIX_FNCTL_H
#define POSIX_FNCTL_H
/*
    fcntl.h: POSIX defined file control options

    https://pubs.opengroup.org/onlinepubs/009604499/basedefs/fcntl.h.html
*/

/*
    open flags
*/
#define O_CREAT     (1 << 0)  /* Create file if it does not exist */
#define O_EXCL      (1 << 1)  /* Exclusive use flag */
#define O_NOCTTY    (1 << 2)  /* Do not assign controlling terminal */
#define O_TRUNC     (1 << 3)  /* Truncate flag */
#define O_APPEND    (1 << 4)  /* Set append mode */
#define O_DIRECTORY (1 << 5)  /* Fail in not a directory */
#define O_DSYNC     (1 << 6)  /* Write according to synchronized I/O data integrity completion */
#define O_NONBLOCK  (1 << 7)  /* Non-blocking mode */
#define O_RSYNC     (1 << 8)  /* Synchronized read I/O operations */
#define O_SYNC      (1 << 9)  /* Write according to synchronized I/O file integrity completion */
#define O_ACCMODE   (1 << 10) /* Mask for file access modes */
#define O_RDONLY    (1 << 11) /* Open for reading only */
#define O_RDWR      (1 << 12) /* Open for reading and writing */
#define O_WRONLY    (1 << 13) /* Open for writing only */

#endif /* POSIX_FNCTL_H */
