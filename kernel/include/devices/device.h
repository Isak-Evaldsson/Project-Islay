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

/*
    Per driver object, stores both statical data (such as driver functions) as well as share data
    for all devices controlled by this driver.
*/
struct driver {
    const char*  name;
    unsigned int major;
    size_t       next_minor;

    // Driver functions
    int (*device_read)(struct device* dev, char* buf, size_t size, off_t offset);

    struct list devices;  // TODO: Have an array as well to make indexing fast?
};

/* Struct representing a single device instance belonging to a particular driver */
struct device {
    unsigned int       minor;
    struct driver*     driver;
    struct pseudo_file file;
    void*              data;
    struct list_entry  list;
};

/* Convert a pointer for a list entry embedded within a device to device object itself, useful when
 * iterating over the device list within a driver */
#define LIST_ENTRY_TO_DEV(entry_ptr) GET_STRUCT(struct device, list, entry_ptr)

/* Initialise the device/driver subsystem */
void drivers_init();

/*
    FS interaction API - the device side of minimal glue required to connect the device and fs
    subsystems with each others.
*/
int dev_read(dev_t dev_no, char* buf, size_t size, off_t offset);

#endif /* DEVICES_DEVICE_H */
