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

    list_add(&driver->devices, &device->list);
    return 0;
}

int dev_read(dev_t dev_no, char *buf, size_t size, off_t offset)
{
    struct device *device;
    struct driver *driver;
    unsigned int   major = GET_MAJOR(dev_no);
    unsigned int   minor = GET_MINOR(dev_no);

    driver = get_driver(major);
    if (driver == NULL) {
        return -EINVAL;
    }

    device = get_device(driver, minor);
    if (device == NULL) {
        return -ENODEV;
    }

    return driver->device_read(device, buf, size, offset);
}
