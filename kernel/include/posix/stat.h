#ifndef POSIX_STAT_H
#define POSIX_STAT_H
#include <posix/types.h>
/*
    stat.h: POSIX defined stats struct

    see https://pubs.opengroup.org/onlinepubs/009696799/basedefs/sys/stat.h.html
*/

struct stat {
    dev_t   st_dev;    // Device ID of device containing file.
    ino_t   st_ino;    // File serial number.
    mode_t  st_mode;   // Mode of file.
    nlink_t st_nlink;  // Number of hard links to the file.
    uid_t   st_uid;    // User ID of file.
    gid_t   st_gid;    // Group ID of file.
    off_t   st_size;   // For regular files, the file size in bytes.
                       // For symbolic links, the length in bytes of the
                       //  pathname contained in the symbolic link.
    time_t st_mtime;   // Time of last data modification.
    time_t st_ctime;   // Time of last status change.
};

#endif /* POSIX_STAT_H */
