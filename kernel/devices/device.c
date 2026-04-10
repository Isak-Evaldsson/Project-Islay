/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#include <libc.h>
#include <utils.h>
#include <devices/device.h>
#include <uapi/errno.h>

#define LOG(fmt, ...) __LOG(1, "[DEVICE]", fmt, ##__VA_ARGS__)

static unsigned int last_major;
static struct {
    struct list minor_devs;
    unsigned int last_minor;
} major_devs[MAJOR_MAX - 1];

struct device *device_get(dev_t dev_no)
{
    struct device *dev;
    unsigned int major = GET_MAJOR(dev_no);

    if (major > last_major)
        return NULL;

    LIST_ITER_STRUCT(&major_devs[major - 1].minor_devs, dev, struct device, minor_list_entry) {
        if (dev->dev_no == dev_no)
            return dev;
    }

    return NULL;
}

dev_t allocate_device_number(unsigned int *major_ptr)
{
    unsigned int major, minor;

    if (!major_ptr)
        return 0;

    major = *major_ptr;
    if (!major) {
        if (last_major >= MAJOR_MAX)
            return 0;

        *major_ptr = major = ++last_major;
        list_init(&major_devs[major - 1].minor_devs);
    }

    if (major_devs[major - 1].last_minor >= MINOR_MAX)
        return 0;

    minor = ++major_devs[major - 1].last_minor;
    return GET_DEV_NUM(major, minor);
}

int create_device_file(struct device *dev, const char *name, bool cdev)
{
    char buf[NAME_MAX];

    if (EMPTY_STR(name) || !dev || !dev->dev_no || !dev->ops)
        return -EINVAL;

    list_add_last(&major_devs[GET_MAJOR(dev->dev_no) - 1].minor_devs, &dev->minor_list_entry);
    snprintf(buf, sizeof(buf), "%s%u", name, GET_MINOR(dev->dev_no));

    return devfs_create_file(buf, dev->dev_no, cdev);
}
