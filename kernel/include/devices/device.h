/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef DEVICES_DEVICE_H
#define DEVICES_DEVICE_H

#include <fs.h>
#include <list.h>
#include <stdbool.h>
#include <stddef.h>
#include <uapi/sys/types.h>

#define MAJOR_MAX 255
#define MINOR_MAX 255

#define GET_DEV_NUM(major, minor) ((((major) & 0xff) << 8) | ((minor) & 0xff))
#define GET_MINOR(dev_no)         ((dev_no) & 0xff)
#define GET_MAJOR(dev_no)         (((dev_no) >> 8) & 0xff)

/* Struct representing a single device instance */
struct device {
    // devfs data
    dev_t dev_no;
    struct device_fops *ops;
    struct list_entry minor_list_entry;

    // Bus/driver managment
    struct device *parent;
    struct driver *driver;
    struct bus *bus;
    struct list_entry driver_list;
    void *drv_data;
};

/* File operations for devfs devices */
struct device_fops {
    ssize_t (*read)(struct device *dev, char* buf, size_t size, off_t offset,
            struct open_file* file);
    ssize_t (*write)(struct device *dev, const char* buf, size_t size, off_t offset,
            struct open_file* file);
    int (*open)(struct device* dev, struct open_file* file);
    int (*close)(struct device* dev, struct open_file* file);
};

/*
 * Devices a major number and device operations with the supplied name
 */
#define DEFINE_DEVICE_TYPE(name, ...)           \
    static unsigned int name##_major;           \
    static struct device_fops name##_ops = {    \
        __VA_OPT__() __VA_ARGS__                \
    };

/*
 * Helper macro to bind and create a file for the give device and device type
*/
#define DEVICE_TYPE_BIND_AND_CREATE_FILE(name, dev, cdev)       \
    ({                                                          \
        struct device *_dev = dev;                              \
        _dev->ops = &name##_ops;                                \
        _dev->dev_no = allocate_device_number(&name##_major);   \
        create_device_file(_dev, #name, cdev);                  \
     })

/*
 * Allocates a new device number for the major number stored in the supplied ptr,
 * if major is 0, allocate an new major number as well.
 *
 * If out of numbers, or the major pointer is null, returns 0
 */
dev_t allocate_device_number(unsigned int *major_ptr);

/* Device lookup based on dev_no, returns null if no device exists */
struct device *device_get(dev_t dev_no);

/*
 * Creates a charater or block device file in devfs for the supplied device with the
 * supplied name
 */
int create_device_file(struct device *dev, const char *name, bool cdev);

#endif /* DEVICES_DEVICE_H */
