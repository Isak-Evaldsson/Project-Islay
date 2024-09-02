#include <arch/paging.h>

#include "fs-internals.h"
#include "romfs/romfs.h"

/* Filesystems to be registered at boot */
static struct fs* boot_fs_list[] = {
    &romfs,
    &sysfs,
};

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

    ret = mount("/sys", SYSFS_FS_NAME, NULL);
    if (ret < 0) {
        LOG("Failed to mount sysfs %i", ret);
        return ret;
    }

    return 0;
}
