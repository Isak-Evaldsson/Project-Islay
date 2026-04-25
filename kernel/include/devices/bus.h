/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2026 Isak Evaldsson
*/
#ifndef DEVICES_BUS_H
#define DEVICES_BUS_H


#include <devices/device.h>
#include <initobj.h>
#include <list.h>
#include <tasks/spinlock.h>

/*
 * Bus-driver framework allowing device-driver matching through a common bus.
 *
 * The bus controller driver adds/removes new devices, the bus framework is then resposible for
 * matching it to a particular driver using the match function provided by the specific bus.
 * Once a matching driver is found, its attach function will be called
 *
 * All buses are required to define thier own <bus_name>_driver and <bus_name>_device structs
 * subsclassing the generic device and driver objects. This is done with the
 * DEFINE_BUS_DEVICE_STRUCT() and DEFINE_BUS_DRIVER_STRUCT() helper macros.
 *
 * The bus and drivers are defined with the DEFINE_BUS() and DEFINE_DRIVER() type macros,
 * making sure the objects is properly initialized and its methods of the correct type.
 */

struct driver {
    struct {
        struct list_entry bus_list_entry;
        struct list devices;
        struct bus *bus;
        const char *name;
        size_t drv_data_size;
    } data;

    // Must be last for bus sub-classing to work
    int (*device_attach)(struct device *dev);
    int (*device_remove)(struct device *dev);
};

struct bus {
    const char *name;
    struct list_entry bus_list;
    struct list unmatched_devices;
    struct list drivers;
    struct spinlock lock;

    // Checks if the device is suitable for the supplied device, returns 1 on match.
    int (*match)(struct driver *driver, struct device *dev);

    // Allows the bus to validate a driver before allowing it to be matched against a device,
    // in not valid returning -ERRNO.
    int (*valid_driver)(struct driver *driver);
};

/*
 * Helper macros to define vaild bus driver and device objects
 *
 * The vararg allows the bus to define additional struct fields within the second argument
 */
#define DEFINE_BUS_DEVICE_STRUCT(bus, ...)      \
    struct bus##_device {                       \
        struct device device;                   \
        __VA_OPT__() __VA_ARGS__                \
    };

#define DEFINE_BUS_DRIVER_STRUCT(bus, ...)              \
    struct bus##_driver {                               \
        GET_TYPE_OF_FIELD(struct driver, data) data;    \
        int (*device_attach)(struct bus##_device *dev); \
        int (*device_remove)(struct bus##_device *dev); \
        __VA_OPT__() __VA_ARGS__                        \
    };

/*
 * Defines a bus, validating the parmeters and make sure it will be registerd upon init.
 */
#define DEFINE_BUS(_name, _match, _valid_driver)                                    \
    static_assert(__VALID_DEVICE_TYPE(struct _name##_device));                      \
    static_assert(__VALID_DRIVER_TYPE(struct _name##_driver, _name));               \
                                                                                    \
    typedef int (*_name##_match_t)(struct _name##_driver*, struct _name##_device*); \
    static_assert(IS_SAME_TYPE(&_match, _name##_match_t));                          \
    typedef int (*_name##_valid_driver_t)(struct _name##_driver*);                  \
    static_assert(IS_SAME_TYPE(&_valid_driver, _name##_valid_driver_t));            \
                                                                                    \
    struct bus _name##_bus = {                                                      \
        .name = #_name,                                                             \
        .match = (GET_TYPE_OF_FIELD(struct bus, match))_match,                      \
        .valid_driver = (GET_TYPE_OF_FIELD(struct bus, valid_driver))_valid_driver, \
    };                                                                              \
    DEFINE_INITOBJ(INITOBJ_TYPE_BUS, bus_name, &_name##_bus)

/*
 * Allows for forward declaration of a bus type to be defined, also adds some
 * convenice macros for struct <bus>_device to struct device conversion and
 * type safe bus add function
 */
#define DECLARE_BUS(_name)                                                                      \
    extern struct bus _name##_bus;                                                              \
    static inline struct _name##_device* to_##_name##_device(struct device *dev)                \
    {                                                                                           \
        return (struct _name##_device *)dev;                                                    \
    }                                                                                           \
    static inline struct device* _name##_to_device(struct _name##_device *dev)                  \
    {                                                                                           \
        return &dev->device;                                                                    \
    }                                                                                           \
    static inline int _add_##_name##_device(struct device *parent, struct _name##_device *dev)  \
    {                                                                                           \
        return bus_add_device(&_name##_bus, parent, &dev->device);                              \
    }

/*
 * Defines a driver, validating the parmeters and make sure it will be registerd upon init.
 * The vararg allows the driver to set additional bus specific fields within the  driver object.
 */
#define DEFINE_DRIVER(_bus_name, _name, _attach, _remove, _drv_data_size, ...) \
    static struct _bus_name##_##driver _name##_##driver = { \
        .data.name = #_name,                                \
        .data.bus = &_bus_name##_bus,                       \
        .data.drv_data_size = _drv_data_size,               \
        .device_attach = _attach,                           \
        .device_remove = _remove                            \
        __VA_OPT__(,) __VA_ARGS__                           \
    };                                                      \
    DEFINE_INITOBJ(INITOBJ_TYPE_DRIVER, _name, &_name##_##driver);

#define GET_DRV_DATA(dev_ptr) ((dev_ptr)->device.drv_data)

/*
 * Adds a driver to its bus and attach it to a driver if a match is found.
 * Returns 0 on success, otherwise -ERRNO.
 */
int bus_add_device(struct bus* bus, struct device *parent, struct device *dev);

/* Removes a device from its bus, before removal calling the dirvers remove function */
void bus_remove_device(struct device *dev);

/* Initialise all bus and drivers */
void init_buses();

/*
 * For object verification, the driver data and devices fields must be placed first within the
 * structs to allow the bus framework to use the bus specific (struct <bus_name>_driver/device)
 * and generic versions (struct driver/device) interchangeably
 */
#define __VALID_DEVICE_TYPE(dev) HAS_FIELD_OF_TYPE_AT_OFFSET(dev, device, struct device, 0)
#define __VALID_DRIVER_TYPE(drv, bus)                                                   \
        HAS_FIELD_OF_TYPE_AT_OFFSET(drv, device_attach, int(*)(struct bus##_device *),  \
            offsetof(struct driver, device_attach)) &&                                  \
        HAS_FIELD_OF_TYPE_AT_OFFSET(drv, device_remove, int(*)(struct bus##_device *),  \
            offsetof(struct driver, device_remove))                                     \

#endif /* DEVICES_BUS_H */
