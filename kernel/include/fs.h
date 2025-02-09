/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef FS_H
#define FS_H
#include <arch/boot.h>
#include <libc.h>
#include <stdbool.h>
#include <uapi/dirent.h>
#include <uapi/sys/fnctl.h>
#include <uapi/sys/stat.h>

#define MAX_OPEN_GLOBAL   100
#define MAX_OPEN_PER_PROC 20
static_assert(MAX_OPEN_GLOBAL >= MAX_OPEN_PER_PROC, "MAX_OPEN_GLOBAL less than MAX_OPEN_PER_PROC");

/* The primary data structure for all file-system objects */
struct inode;

/*  Information about an open file */
struct open_file;

/* Mount flags, controls how a fs should be mounted */
#define MOUNT_READONLY 0x01

/*
   Object representing a pseudo file, it can be used to build arbitrary file graphs, allowing for
   the implementation of various pseudo filesystems.
*/
struct pseudo_file {
    char   name[NAME_MAX];
    ino_t  inode;
    mode_t mode;
    void*  data;

    struct pseudo_file* parent;
    struct pseudo_file* child;
    struct pseudo_file* sibling;
};

/* Struct containing all per task fs related data */
struct task_fs_data {
    // Note, the inode pointers needs to be cloned on usage, otherwise calls to put_inode() may lead
    // to the being free which may break the vfs it they point to a root inode
    struct inode* rootdir;  // Root directory, stored per process to enable chroot syscall
    struct inode* workdir;  // Start inode for relative paths

    struct open_file* file_table[MAX_OPEN_PER_PROC];
};

/* To ensure that the per task fs data struct is properly initialised */
void task_data_init(struct task_fs_data* task_data);

/** Initialise the file system based on the boot data parameters */
int fs_init(struct boot_data* boot_data);

/**
 * Mounts a file system to a given path, the caller is responsible for supplying correct
 * mounting data for a particular file system.
 * @param path were to mount the fs
 * @param name the name of the file system to mount
 * @param flags allows for specific mount options
 * @param data fs specific data needed for mounting
 * @return 0 on success, -errno on failure */
int mount(const char* path, const char* name, unsigned int flags, void* data);

/* Open file */
int open(struct task_fs_data* task_data, const char* path, int oflag);

/* Close file */
int close(struct task_fs_data* task_data, int fd);

/* Write to file at fixed offset */
ssize_t pwrite(struct task_fs_data* task_data, int fd, const void* buf, size_t count, off_t offset);

/* Write to file continuously */
ssize_t write(struct task_fs_data* task_data, int fd, const void* buf, size_t count);

/* Read file continuously */
ssize_t read(struct task_fs_data* task_data, int fd, void* buf, size_t nbyte);

/* Read file at fixed offset */
ssize_t pread(struct task_fs_data* task_data, int fd, void* buf, size_t nbyte, off_t offset);

/*
    Reads from at certain fd into the supplied buffer and returns the number of dirents written to
    the buffer or -ERRNO on failure.

    Similar to read it automatically increments the read offset. If the return value is 0 or less
    than buf_count we have reach the end of the dir.
*/
int readdirents(struct task_fs_data* task_data, int fd, struct dirent* buf, int buf_count);

/*
    Devfs API - the fs side of minimal glue required to connect the device and fs subsystems
    with each others.
*/

/*
    Adds the supplied device object to devfs  relative to the specified directory
    (or within fs root if NULL). Returns 0 on success, and -ERRNO on failure.
*/
int devfs_add_dev(struct pseudo_file* dir, struct pseudo_file* file, dev_t dev_no, char* name,
                  bool cdev);
#endif /* FS_H */
