/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#ifndef DEVICES_INTERNALS_H
#define DEVICES_INTERNALS_H

#include <devices/device.h>
#include <uapi/errno.h>
#include <utils.h>

#define MAJOR_MAX 255
#define MINOR_MAX 255

#define GET_DEV_NUM(major, minor) ((((major) & 0xff) << 8) | ((minor) & 0xff))
#define GET_MINOR(dev_no)         ((dev_no) & 0xff)
#define GET_MAJOR(dev_no)         (((dev_no) >> 8) & 0xff)

#define DRIVER_NAME_MAXLEN (50)

int register_device(struct driver *driver, struct device *device);

struct driver *get_driver(unsigned int major);
struct device *get_device(struct driver *driver, unsigned int minor);

#endif /* DEVICES_INTERNALS_H */
