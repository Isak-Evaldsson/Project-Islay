#ifndef POSIX_TYPES_H
#define POSIX_TYPES_H
#include <stdint.h>
/*
    types.h: POSIX defined types

    see https://pubs.opengroup.org/onlinepubs/009604499/basedefs/sys/types.h.html
*/

typedef unsigned int blkcnt_t;   // Used for file block counts;
typedef unsigned int blksize_t;  // Used for block sizes;
typedef long         off_t;
typedef unsigned int dev_t;
typedef unsigned int ino_t;
typedef unsigned int mode_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef unsigned int nlink_t;
typedef uint64_t     time_t;
typedef intmax_t     ssize_t;

#endif /* POSIX_TYPES_H */
