/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#include "internals.h"

#define LOG(fmt, ...) __LOG(1, "[DEVICE]", fmt, ##__VA_ARGS__)

struct device *get_device(struct driver *driver, unsigned int minor)
{
    struct list_entry *entry;
    struct device     *device;

    LIST_ITER(&driver->devices, entry)
    {
        device = LIST_ENTRY_TO_DEV(entry);
        if (device->minor == minor) {
            return device;
        }
    }
    return NULL;
}

int register_device(struct driver *driver, struct device *device)
{
    unsigned int major = driver->major;

    if (!major) {
        LOG("driver %s is not registered", driver->name);
        return -EINVAL;
    }

    if (driver->next_minor > MINOR_MAX) {
        LOG("driver '%s' (%u) is out of minor numbers", driver->name, driver->major);
        return -EINVAL;
    }

    device->minor  = driver->next_minor++;
    device->driver = driver;

    list_add_last(&driver->devices, &device->list);
    return 0;
}

int create_device_file(struct pseudo_file *dir, struct device *dev, bool cdev)
{
    dev_t dev_no;
    char  buff[DRIVER_NAME_MAXLEN + 10];

    if (!dev->minor || !dev->driver || !dev->driver->major) {
        LOG("Trying to create file for invalid device");
        return -EINVAL;
    }

    dev_no = GET_DEV_NUM(dev->driver->major, dev->minor);
    snprintf(buff, sizeof(buff), "%s%u", dev->driver->name, dev->minor);
    return devfs_add_dev(dir, &dev->file, dev_no, buff, cdev);
}

static int parse_devno(dev_t dev_no, struct device **device, struct driver **driver)
{
    unsigned int major = GET_MAJOR(dev_no);
    unsigned int minor = GET_MINOR(dev_no);

    *driver = get_driver(major);
    if (*driver == NULL) {
        return -EINVAL;
    }

    *device = get_device(*driver, minor);
    if (*device == NULL) {
        return -ENODEV;
    }
    return 0;
}

int dev_open(dev_t dev_no, struct open_file *file, int oflag)
{
    struct device *device;
    struct driver *driver;

    int ret = parse_devno(dev_no, &device, &driver);
    if (ret < 0) {
        return ret;
    }

    LOG("Got device %u, driver %s", device, driver->name);
    return driver->device_open(device, file, oflag);
}

int dev_close(dev_t dev_no, struct open_file *file)
{
    struct device *device;
    struct driver *driver;

    int ret = parse_devno(dev_no, &device, &driver);
    if (ret < 0) {
        return ret;
    }
    return driver->device_close(device, file);
}

ssize_t dev_read(dev_t dev_no, char *buf, size_t size, off_t offset)
{
    struct device *device;
    struct driver *driver;

    int ret = parse_devno(dev_no, &device, &driver);
    if (ret < 0) {
        return ret;
    }

    if (!driver->device_read) {
        return -ENOTSUP;
    }
    return driver->device_read(device, buf, size, offset);
}

ssize_t dev_write(dev_t dev_no, const char *buf, size_t size, off_t offset)
{
    struct device *device;
    struct driver *driver;

    int ret = parse_devno(dev_no, &device, &driver);
    if (ret < 0) {
        return ret;
    }

    if (!driver->device_write) {
        return -ENOTSUP;
    }
    return driver->device_write(device, buf, size, offset);
}
