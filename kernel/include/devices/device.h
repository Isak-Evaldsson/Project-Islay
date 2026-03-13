/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef DEVICES_DEVICE_H
#define DEVICES_DEVICE_H

#include <fs.h>
#include <list.h>
#include <stddef.h>
#include <uapi/sys/types.h>

#define MAJOR_MAX 255
#define MINOR_MAX 255

#define GET_DEV_NUM(major, minor) ((((major) & 0xff) << 8) | ((minor) & 0xff))
#define GET_MINOR(dev_no)         ((dev_no) & 0xff)
#define GET_MAJOR(dev_no)         (((dev_no) >> 8) & 0xff)

/* Struct representing a single device instance belonging to a particular driver */
struct device {
    unsigned int       minor;
    struct driver*     driver;
    void*              data;
    struct list_entry  list;
};

/*
    Per driver object, stores both statical data (such as driver functions) as well as share data
    for all devices controlled by this driver.
*/
struct driver {
    const char*  name;
    unsigned int major;
    size_t       next_minor;

    // Driver functions
    ssize_t (*device_read)(struct device* dev, char* buf, size_t size, off_t offset);
    ssize_t (*device_write)(struct device* dev, const char* buf, size_t size, off_t offset);
    int (*device_open)(struct device* dev, struct open_file* file, int oflag);
    int (*device_close)(struct device* dev, struct open_file* file);

    struct list devices;  // TODO: Have an array as well to make indexing fast?
};

/* Convert a pointer for a list entry embedded within a device to device object itself, useful when
 * iterating over the device list within a driver */
#define LIST_ENTRY_TO_DEV(entry_ptr) GET_STRUCT(struct device, list, entry_ptr)

/* Initialise the device/driver subsystem */
void drivers_init();

/* Device lookup based on dev_no, returns null if no devices exsit */
struct device* device_get(dev_t dev_no);

/*
    Creates a device file within devfs for the specified device. Returns 0 on success, or -ERRNO on failure.
*/
int create_device_file(struct device* dev, bool cdev);

#endif /* DEVICES_DEVICE_H */
