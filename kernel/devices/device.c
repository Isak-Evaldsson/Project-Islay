/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#include "internals.h"

#define LOG(fmt, ...) __LOG(1, "[DEVICE]", fmt, ##__VA_ARGS__)

struct device *get_device(struct driver *driver, unsigned int minor)
{
    struct device     *device;

    LIST_ITER_STRUCT(&driver->devices, device, struct device, list)
    {
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

int create_device_file(struct device *dev, bool cdev)
{
    dev_t dev_no;
    char  buff[DRIVER_NAME_MAXLEN + 10];

    if (!dev->minor || !dev->driver || !dev->driver->major) {
        LOG("Trying to create file for invalid device");
        return -EINVAL;
    }

    dev_no = GET_DEV_NUM(dev->driver->major, dev->minor);
    snprintf(buff, sizeof(buff), "%s%u", dev->driver->name, dev->minor);
    return devfs_create_file(buff, dev_no, cdev);
}

struct device* device_get(dev_t dev_no)
{
    struct driver *driver;
    unsigned int major = GET_MAJOR(dev_no);
    unsigned int minor = GET_MINOR(dev_no);

    driver = get_driver(major);
    if (driver == NULL) {
        return NULL;
    }

    return get_device(driver, minor);
}
