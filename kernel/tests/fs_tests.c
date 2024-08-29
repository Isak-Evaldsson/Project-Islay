#include <fs.h>
#include <uapi/errno.h>
#include <utils.h>

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

    kprintf("mounted testfs: %s\n", msg->str);
    return 0;
}

static struct fs_ops test_fs_ops = {.mount = test_fs_mount};
static DEFINE_FS(test_fs, "test_fs", &test_fs_ops);
static DEFINE_FS(test_fs2, "test_fs2", &test_fs_ops);

static int register_fs_test()
{
    int ret;

    ret = register_fs(&test_fs);
    if (ret) {
        kprintf("Failed to register test_fs (%i)\n", ret);
        return -1;
    }

    ret = register_fs(&test_fs);
    if (ret != -EEXIST) {
        kprintf("Registering the same filesystem twice should yield an error (%i)\n", ret);
        return -1;
    }

    ret = register_fs(&test_fs2);
    if (ret) {
        kprintf("Failed to register test_fs2 (%i)\n", ret);
        return -1;
    }
    return 0;
}

static int test_mounting()
{
    int         ret;
    struct data success = {.str = "Hello"};
    struct data fail    = {.fail = 10};

    ret = mount("/testdir", "test_fs", &success);
    if (ret) {
        kprintf("Failed to mount testdir1 (%x)\n", ret);
        return -1;
    }

    ret = mount("/testdir2", "test_fs", &success);
    if (ret) {
        kprintf("Failed to mount testdir2 (%x)\n", ret);
        return -1;
    }

    ret = mount("/failure", "test_fs2", &fail);
    if (ret != -EIO) {
        kprintf("A failed call to fs->mount should yield and error (%i)\n", ret);
        return -1;
    }

    ret = mount("/testdir", "test_fs", &success);
    if (ret != -EEXIST) {
        kprintf("Mounting at the same path twice should yield an error (%i)\n", ret);
        return -1;
    }

    ret = mount("/failure2", "does_not_exits", &success);
    if (ret != -ENOENT) {
        kprintf("Trying to mount an not registered file system (%i)\n", ret);
        return -1;
    }

    ret = mount("/dir1/dir2/test1", "test_fs", &success);
    if (ret) {
        kprintf("Failed to mount '/dir1/dir2/test1' (%x)\n", ret);
        return -1;
    }

    ret = mount("/dir1/dir2/test2", "test_fs", &success);
    if (ret) {
        kprintf("Failed to mount '/dir1/dir2/test2' (%x)\n", ret);
        return -1;
    }

    ret = mount("/dir1/dir2/test2", "test_fs", &success);
    if (ret != -EEXIST) {
        kprintf("Mounting at the same path twice should yield an error (%i)\n", ret);
        return -1;
    }

    // Do we clean-up correctly (no trailing dirs?)
    ret = mount("/dir1/dir2/di3/dir4/dir5/dir6", "test_fs", &fail);
    if (ret != -EIO) {
        kprintf("A failed call to fs->mount should yield and error (%i)\n", ret);
        return -1;
    }

    ret = mount("/", "test_fs", &success);
    if (ret != -ENOTSUP) {
        kprintf("Mounting at root should fail (%i)\n", ret);
        return -1;
    }

    return 0;
}

void fs_tests()
{
    if (register_fs_test()) {
        kprintf("register_fs_test failed\n");
        return;
    }

    if (test_mounting()) {
        kprintf("test_mounting failed\n");
        return;
    }

    kprintf("fs tests successful\n");
}
