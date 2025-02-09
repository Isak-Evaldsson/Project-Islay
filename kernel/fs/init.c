/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/paging.h>

#include "devfs/devfs.h"
#include "fs-internals.h"
#include "kinfo/kinfo.h"
#include "romfs/romfs.h"

void kinfo_dump_inodes(struct kinfo_buffer* buff);
void kinfo_dump_open_files(struct kinfo_buffer* buff);
void kinfo_dump_vfs(struct kinfo_buffer* buff);

/* Filesystems to be registered at boot */
static struct fs* boot_fs_list[] = {
    &devfs,
    &romfs,
    &kinfo,
};

/* FS related kinfo data */
static struct kinfo_file* kinfo_fs_dir;
static struct kinfo_file* kinfo_inodes;
static struct kinfo_file* kinfo_open_files;
static struct kinfo_file* kinfo_vfs;

int fs_init(struct boot_data* boot_data)
{
    int ret;

    for (size_t i = 0; i < COUNT_ARRAY_ELEMS(boot_fs_list); i++) {
        ret = register_fs(boot_fs_list[i]);
        if (ret < 0) {
            LOG("Failed to register %s %i", boot_fs_list[i]->name, ret);
            return ret;
        }
    }

    // Mount initrd
    struct romfs_mount_data intird_mnt_data = {
        .data = (char*)P2L(boot_data->initrd_start),
        .size = boot_data->initrd_size,
    };

    ret = mount_rootfs(ROMFS_FS_NAME, &intird_mnt_data);
    if (ret < 0) {
        LOG("Failed to mount initrd as rootfs: %i", ret);
        return ret;
    }

    ret = mount("/kinfo", KINFO_FS_NAME, 0, NULL);
    if (ret < 0) {
        LOG("Failed to mount kinfo %i", ret);
        return ret;
    }

    ret = kinfo_create_file(NULL, &kinfo_fs_dir, "fs", S_IFDIR, NULL);
    if (ret < 0) {
        LOG("Failed to create kinfo/fs directory %i", ret);
        return ret;
    }

    ret = kinfo_create_file(kinfo_fs_dir, &kinfo_inodes, "inodes", S_IFREG, kinfo_dump_inodes);
    if (ret < 0) {
        LOG("Failed to create kinfo/fs/inodes file %i", ret);
        return ret;
    }

    ret =
        kinfo_create_file(kinfo_fs_dir, &kinfo_open_files, "files", S_IFREG, kinfo_dump_open_files);
    if (ret < 0) {
        LOG("Failed to create kinfo/fs/files file %i", ret);
        return ret;
    }

    ret = kinfo_create_file(kinfo_fs_dir, &kinfo_vfs, "vfs", S_IFREG, kinfo_dump_vfs);
    if (ret < 0) {
        LOG("Failed to create kinfo/fs/files file %i", ret);
        return ret;
    }

    ret = mount("/dev", DEVFS_FS_NAME, 0, NULL);
    if (ret < 0) {
        LOG("Failed to mount devfs %i", ret);
        return ret;
    }

    return 0;
}
