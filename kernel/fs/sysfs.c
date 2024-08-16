/*
    sysfs - a filesystem exposing kernel info the userspace.

    Conceptually similar to sys/procfs in Linux.
*/

#include <arch/paging.h>
#include <libc.h>
#include <memory/vmem_manager.h>

#include "fs-internals.h"

/* Global buffer used when reading from the virtual files within sysfs */
static char*  read_buffer;
static size_t read_buffer_size;
static size_t read_buffer_offset;

/* Sysfs file object */
struct sysfs_file {
    char* name;      // file name
    void (*read)();  // function called we reading the file
};

/* Root dir, currently no directory support */
static struct sysfs_file files[] = {
    {.name = "vfs",    .read = sysfs_dump_vfs       },
    {.name = "files",  .read = sysfs_dump_open_files},
    {.name = "inodes", .read = sysfs_dump_inodes    },
};

/*
    File system opreations
*/

static int sysfs_read(char* buf, size_t size, off_t offset, struct open_file* file)
{
    struct sysfs_file* f = file->inode->id;

    // Reset global buffer
    read_buffer_offset = 0;
    memset(read_buffer, '\0', read_buffer_size);

    // Dump the whole 'file'
    f->read();

    // Then copy according to size and offset
    size_t read_size = size;
    if (offset + size > read_buffer_size) {
        read_size = read_buffer_size - offset;
    }

    memcpy(buf, read_buffer + offset, read_size);
    return read_size;
}

static int sysfs_open(const char* path, unsigned int* inode_id, struct file_info* info)
{
    // Search files, and store point in inode
    struct sysfs_file* f;

    for (f = files; f < END_OF_ARRAY(files); f++) {
        if (strcmp(path, f->name) == 0) {
            *inode_id = f;
            return 0;
        }
    }
    return -ENOENT;
}

static int sysfs_mount(void* data)
{
    (void)data;

    read_buffer      = (char*)vmem_request_free_page(0);
    read_buffer_size = PAGE_SIZE;

    if (!read_buffer) {
        return -ENOMEM;
    }

    return 0;
}

static struct fs_ops sysfs_ops = {
    .mount = sysfs_mount,
    .open  = sysfs_open,
    .read  = sysfs_read,
};

static DEFINE_FS(sysfs, "sysfs", &sysfs_ops);

/*
    Sysfs API
*/

/* Writes formatted strings to the global sysfs read buffer */
void sysfs_writer(const char* restrict format, ...)
{
    size_t  nbytes;
    char*   start = read_buffer + read_buffer_offset;
    size_t* size  = read_buffer_size - read_buffer_offset;
    va_list args;

    va_start(args, format);
    nbytes = vsnprintf(start, size, format, args);
    va_end(args);

    if (nbytes > 0) {
        read_buffer_offset += nbytes;
    }
};

/* Mounts sysfs at the specified path */
int mount_sysfs(const char* path)
{
    int ret;

    ret = register_fs(&sysfs);
    if (ret && ret != -EEXIST) {
        kprintf("Failed to register sysfs (%i)\n", ret);
        return ret;
    }

    ret = mount(path, "sysfs", NULL);
    if (ret) {
        log("Failed to mount sysfs at /sys (%i)\n", ret);
        return ret;
    }
}