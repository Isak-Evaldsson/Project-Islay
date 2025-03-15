/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <fs.h>
#include <uapi/errno.h>
#include <utils.h>

#include "../fs/fs-internals.h"
#include "test.h"

struct data {
    int         fail;
    const char *str;
};

static int test_fs_mount(void *data)
{
    struct data *msg = data;
    if (msg->fail) {
        return -EIO;
    }

    TEST_LOG("mounted testfs: %s\n", msg->str);
    return 0;
}

static int test_fs_read(char *buf, size_t size, off_t offset, struct open_file *file)
{
    return -EINVAL;
}

static int test_fs_fetch_inode(const struct superblock *super, ino_t id, struct inode *inode)
{
    return -EINVAL;
}

static int test_fs_readdir(const struct open_file *file, struct dirent *dirent, off_t offset)
{
    return -EINVAL;
}

static struct fs_ops test_fs_ops = {.mount       = test_fs_mount,
                                    .read        = test_fs_read,
                                    .fetch_inode = test_fs_fetch_inode,
                                    .readdir     = test_fs_readdir};
static DEFINE_FS(test_fs, "test_fs", &test_fs_ops, 0);
static DEFINE_FS(test_fs2, "test_fs2", &test_fs_ops, 0);

static int register_fs_test()
{
    /* TODO: Break into sub-tests*/
    int ret;

    ret = register_fs(&test_fs);
    if (ret) {
        TEST_LOG("Failed to register test_fs (%i)", ret);
        return ret;
    }

    ret = register_fs(&test_fs);
    if (ret != -EEXIST) {
        TEST_LOG("Registering the same filesystem twice should yield an error (%i)", ret);
        return ret;
    }

    ret = register_fs(&test_fs2);
    if (ret) {
        TEST_LOG("Failed to register test_fs2 (%i)", ret);
        return ret;
    }
    return 0;
}

static int test_mounting()
{
    int         ret;
    struct data success = {.str = "Hello"};
    struct data fail    = {.fail = 10};

    ret = mount("/testdir", "test_fs", 0, &success);
    if (ret) {
        TEST_LOG("Failed to mount testdir1 (%x)\n", ret);
        return -1;
    }

    ret = mount("/testdir2", "test_fs", 0, &success);
    if (ret) {
        TEST_LOG("Failed to mount testdir2 (%x)\n", ret);
        return -1;
    }

    ret = mount("/failure", "test_fs2", 0, &fail);
    if (ret != -EIO) {
        TEST_LOG("A failed call to fs->mount should yield and error (%i)\n", ret);
        return -1;
    }

    ret = mount("/testdir", "test_fs", 0, &success);
    if (ret != -EEXIST) {
        TEST_LOG("Mounting at the same path twice should yield an error (%i)\n", ret);
        return -1;
    }

    ret = mount("/failure2", "does_not_exits", 0, &success);
    if (ret != -ENOENT) {
        TEST_LOG("Trying to mount an not registered file system (%i)\n", ret);
        return -1;
    }

    ret = mount("/dir1/dir2/test1", "test_fs", 0, &success);
    if (ret) {
        TEST_LOG("Failed to mount '/dir1/dir2/test1' (%x)\n", ret);
        return -1;
    }

    ret = mount("/dir1/dir2/test2", "test_fs", 0, &success);
    if (ret) {
        TEST_LOG("Failed to mount '/dir1/dir2/test2' (%x)\n", ret);
        return -1;
    }

    ret = mount("/dir1/dir2/test2", "test_fs", 0, &success);
    if (ret != -EEXIST) {
        TEST_LOG("Mounting at the same path twice should yield an error (%i)\n", ret);
        return -1;
    }

    // Do we clean-up correctly (no trailing dirs?)
    ret = mount("/dir1/dir2/di3/dir4/dir5/dir6", "test_fs", 0, &fail);
    if (ret != -EIO) {
        TEST_LOG("A failed call to fs->mount should yield and error (%i)\n", ret);
        return -1;
    }

    ret = mount("/", "test_fs", 0, &success);
    if (ret != -ENOTSUP) {
        TEST_LOG("Mounting at root should fail (%i)\n", ret);
        return -1;
    }

    return 0;
}

struct test_func fs_tests[] = {
    CREATE_TEST_FUNC(register_fs_test),
    /* TODO: Fix mounting tests... */
};

struct test_suite fs_test_suite = {
    .name     = "file system tests",
    .setup    = NULL,
    .teardown = NULL,
    .tests    = fs_tests,
    .n_tests  = COUNT_ARRAY_ELEMS(fs_tests),
};
