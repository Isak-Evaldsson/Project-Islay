/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2023 Sebastian Raase (original FUSE implementation)
   Copyright (C) 2024 Isak Evaldsson
*/
#include "romfs.h"

#define ROMFS_MAXLEN 128
static_assert(ROMFS_MAXLEN <= NAME_MAX); /* Ensure consistency with the kernel macro */

/* 3 lowest bit of next pointer stores file type */
#define GET_TYPE(num) GET_LOWEST_BITS(num, 3)

/* the forth bit in next pointer stores executable status */
#define GET_EXECUTABLE(num) MASK_BIT(num, 3)

#define GET_NEXT(next) MASK_LOWEST_BITS(next, 4)

/* Executable + type*/
#define GET_FLAGS(num) GET_LOWEST_BITS(num, 4)

enum romfs_filetype_t {
    ROMFS_TYPE_HLINK,
    ROMFS_TYPE_DIR,
    ROMFS_TYPE_FILE,
    ROMFS_TYPE_SYMLINK,
    ROMFS_TYPE_BLKDEV,
    ROMFS_TYPE_CHARDEV,
    ROMFS_TYPE_SOCK,
    ROMFS_TYPE_FIFO,
};

struct romfs_superblk {
    uint8_t  magic[8];       // should be -rom1fs-
    uint32_t fullsize;       // number accessible bytes
    uint32_t checksum;       // set so that the checksum computes to 0
    char     volume_name[];  // 16 byte padded, null-terminated string
};

struct romfs_header {
    uint32_t next;
    uint32_t info;
    uint32_t size;
    uint32_t checksum;
    char     file_name[];
};

static struct romfs_mount_data mount_data;

/* computes checksum of block, if result is non-zero, then the block is
 * inconsistent */
static uint32_t checksum(uint32_t* buff, size_t size)
{
    uint32_t checksum = 0;
    size /= 4;
    while (size--) {
        checksum += be32ton(*buff++);
    }
    return checksum;
}

/* Reads data in the ram block specified in the supplied mount data. Contains logic to ensure the
 * reading to not overflow. */
static size_t read_data(struct romfs_mount_data* mdata, void* buff, size_t buf_size, size_t offset)
{
    size_t size = MIN(mdata->size - offset, buf_size);

    if (offset >= mdata->size) {
        return 0;
    }

    memcpy(buff, mdata->data + offset, size);
    return size;
}

static int read_header(struct romfs_header* header, size_t offset)
{
    size_t size = sizeof(struct romfs_header);
    if (read_data(&mount_data, header, size, offset) != size) {
        return -1;
    }
    header->next     = be32ton(header->next);
    header->info     = be32ton(header->info);
    header->size     = be32ton(header->size);
    header->checksum = be32ton(header->checksum);
    return 0;
}

/* Read filename at the given offset, returns the offset for the end of the name */
static int read_filename(size_t offset, char* name)
{
    // filename must be at least 16 bytes long
    if (read_data(&mount_data, name, ROMFS_MAXLEN, offset) < 16) {
        return -1;
    }
    return offset + ALIGN_BY_MULTIPLE(strnlen(name, ROMFS_MAXLEN), 16);
}

/* Loads file at given node within file system while performing hardlink traversal. Returns the
 * actual offset of the node */
static off_t load_file(off_t offset, struct romfs_header* header, char* name, size_t* data)
{
    int   end;
    off_t node = offset, next;

    if (read_header(header, node) < 0) {
        return -1;
    }

    if (name) {
        end = read_filename(node + sizeof(struct romfs_header), name);
        if (end < 0) {
            return -EIO;
        }
        if (data) {
            *data = end;  // file data starts directly after the filename
        }
    }

    // Iterate over hardlinks
    next = GET_NEXT(header->next);
    while (GET_TYPE(header->next) == ROMFS_TYPE_HLINK) {
        node = header->info;
        if (read_header(header, node) < 0) {
            return -1;
        }

        // Keep original next pointer, but ensure that flags reflects whatever the hardlink points
        // to. Without this, future uses of the header objects in directory iteration will break.
        header->next = next | GET_FLAGS(header->next);

        // Make sure that data points to the file that the hardlink points to
        if (data) {
            char buff[ROMFS_MAXLEN];
            end = read_filename(node + sizeof(struct romfs_header), buff);
            if (end < 0) {
                return -EIO;
            }
            *data = end;
        }
    }

    return node;
}

static mode_t header_mode_bits(struct romfs_header* header)
{
    mode_t mode = 0444;  // read only for all users
    if (GET_EXECUTABLE(header->next)) {
        mode |= 0111;  // execute permission
    }

    mode_t type = 0;
    switch (GET_TYPE(header->next)) {
        case ROMFS_TYPE_DIR:
            type = S_IFDIR;
            break;
        case ROMFS_TYPE_HLINK:
        case ROMFS_TYPE_FILE:
            type = S_IFREG;
            break;
        case ROMFS_TYPE_SYMLINK:
            type = S_IFLNK;
            break;
        case ROMFS_TYPE_BLKDEV:
            type = S_IFBLK;
            break;
        case ROMFS_TYPE_CHARDEV:
            type = S_IFCHR;
            break;
        case ROMFS_TYPE_SOCK:
            type = S_IFSOCK;
            break;
        case ROMFS_TYPE_FIFO:
            type = S_IFIFO;
            break;
    }
    return type | mode;
}

static void read_attr(struct romfs_header* header, struct stat* stat)
{
    // TODO: Handle hard links
    stat->st_nlink = 1;
    stat->st_size  = header->size;
    stat->st_mode  = header_mode_bits(header);

    // Since this fs doesn't support gid/uid we set it to root
    stat->st_gid = 0;
    stat->st_uid = 0;

    // Not stored within this fs
    stat->st_mtime = 0;
    stat->st_ctime = 0;
    stat->st_atime = 0;

    // TODO: Define blksize
    stat->st_blksize = 0;
    stat->st_blocks  = stat->st_blksize > 0 ? stat->st_size / stat->st_blksize : 0;
}

static int romfs_getattr(const struct open_file* file, struct stat* stat)
{
    int                 ret = 0;
    struct romfs_header header;

    ret = load_file(file->inode->id, &header, NULL, NULL);
    if (ret < 0) {
        return ret;
    }

    read_attr(&header, stat);
    return 0;
}

static ssize_t romfs_read(char* buf, size_t size, off_t offset, struct open_file* file)
{
    int                 ret;
    size_t              data;
    struct romfs_header header;
    char                name[ROMFS_MAXLEN];

    ret = load_file(file->inode->id, &header, name, &data);
    if (ret < 0) {
        return ret;
    }

    // Make sure that the read doesn't overflow
    size_t read_size = size;
    if (offset + size > header.size) {
        read_size = header.size - offset;
    }

    if (read_data(&mount_data, buf, read_size, data) != read_size) {
        return -EIO;
    };

    return read_size;
}

int romfs_fetch_inode(const struct superblock* super, ino_t id, struct inode* inode)
{
    int                 ret;
    struct romfs_header header;

    ret = load_file(id, &header, NULL, NULL);
    if (ret < 0) {
        return ret;
    }

    inode->mode = header_mode_bits(&header);
    return 0;
}

static int romfs_readdir(const struct open_file* file, struct dirent* dirent, off_t offset)
{
    int                 ret;
    off_t               next = offset;
    char                name[ROMFS_MAXLEN];
    struct romfs_header header;

    if (offset == 0) {
        // If first call to readdir, fetch the directory node
        ret = load_file(file->inode->id, &header, NULL, NULL);
        if (ret < 0) {
            return ret;
        }

        if (GET_TYPE(header.next) != ROMFS_TYPE_DIR)
            return -ENOTDIR;

        next = header.info;
    }

    ret = load_file(next, &header, name, NULL);
    if (ret < 0) {
        return -EIO;
    }

    dirent->d_ino = ret;
    strcpy(dirent->d_name, name);
    return GET_NEXT(header.next);
}

static int romfs_mount(struct superblock* super, void* data, ino_t* root_ptr)
{
    int                      ret;
    char                     buff[512];
    struct romfs_superblk*   blk   = (struct romfs_superblk*)&buff;
    struct romfs_mount_data* mdata = data;

    // TODO: Allow multiple romfs instances to be mounted simultaneously
    if (mount_data.data != NULL || mount_data.size != 0) {
        LOG("romfs can't be mounted twice");
        return -EEXIST;
    }

    // Read all first 512 bytes at once
    if (read_data(mdata, buff, sizeof(buff), 0) < 512) {
        LOG("Image needs to be a least 512 bytes");
        return -ENOMEM;
    }

    // Verify superblock correctness
    if (strncmp((char*)blk->magic, "-rom1fs-", 8) != 0) {
        LOG("Invalid magic value");
        return -EINVAL;
    }

    if (checksum((uint32_t*)buff, 512) != 0) {
        LOG("Incorrect checksum");
        return -EINVAL;
    }

    size_t name_length = ALIGN_BY_MULTIPLE(strnlen(blk->volume_name, ROMFS_MAXLEN), 16);
    if (name_length >= ROMFS_MAXLEN) {
        LOG("Volume name to long (max %u)", ROMFS_MAXLEN);
        return -EINVAL;
    }

    size_t size = be32ton(blk->fullsize);
    if (size > mdata->size) {
        LOG("file system size %u bigger the allocated amount %u", size, mdata->size);
        return -EINVAL;
    }

    LOG("Mounting romfs image of size %u containing romfs-volume: '%s'", size, blk->volume_name);
    mount_data.data = mdata->data;
    mount_data.size = size;
    *root_ptr       = sizeof(struct romfs_superblk) + name_length;

    return 0;
}

static struct fs_ops romfs_ops = {
    .mount       = romfs_mount,
    .getattr     = romfs_getattr,
    .read        = romfs_read,
    .readdir     = romfs_readdir,
    .fetch_inode = romfs_fetch_inode,
};

DEFINE_FS(romfs, ROMFS_FS_NAME, &romfs_ops, MOUNT_READONLY);
