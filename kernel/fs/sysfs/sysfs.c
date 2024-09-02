#include <arch/paging.h>
#include <libc.h>
#include <memory/vmem_manager.h>

#include "sysfs.h"

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

static struct sysfs_file root = {.name = ".", .read = NULL};

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

static int sysfs_readdir(const struct open_file* file, struct dirent* dirent, off_t offset)
{
    struct sysfs_file* f = files + offset;

    kassert(file->inode->id == (ino_t)&root);

    dirent->d_ino = f;
    strcpy(dirent->d_name, f->name);

    if (++offset < COUNT_ARRAY_ELEMS(files)) {
        return offset;
    }

    return 0;
}

static int sysfs_fetch_inode(const struct superblock* super, ino_t id, struct inode* inode)
{
    if (id == (ino_t)&root) {
        inode->mode = S_IFDIR;
        return 0;
    }

    // Check if it's a valid id
    if (id >= (ino_t)files && id < (ino_t)END_OF_ARRAY(files) &&
        !(id % sizeof(struct sysfs_file))) {
        inode->mode = S_IFREG;
        return 0;
    }
    return -EINVAL;
}

static int sysfs_mount(struct superblock* super, void* data, ino_t* root_ptr)
{
    int ret;
    (void)data;

    read_buffer      = (char*)vmem_request_free_page(0);
    read_buffer_size = PAGE_SIZE;

    if (!read_buffer) {
        return -ENOMEM;
    }

    *root_ptr = (ino_t)&root;
    return 0;
}

static struct fs_ops sysfs_ops = {
    .mount       = sysfs_mount,
    .read        = sysfs_read,
    .readdir     = sysfs_readdir,
    .fetch_inode = sysfs_fetch_inode,
};

DEFINE_FS(sysfs, SYSFS_FS_NAME, &sysfs_ops);

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
