#ifndef POSIX_STAT_H
#define POSIX_STAT_H
#include <posix/types.h>
/*
    stat.h: POSIX defined stats struct

    see https://pubs.opengroup.org/onlinepubs/009696799/basedefs/sys/stat.h.html
*/

/*
    Mode bits
*/
#define S_IFMT   0170000 /* File type mask */
#define S_IFIFO  0010000 /* FIFO special */
#define S_IFCHR  0020000 /* Character special */
#define S_IFDIR  0040000 /* Directory */
#define S_IFBLK  0060000 /* Block special. */
#define S_IFREG  0100000 /* Regular */
#define S_IFLNK  0120000 /* Symbolic link */
#define S_IFSOCK 0140000 /* Socket */

/*
    Test macros
*/
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

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
