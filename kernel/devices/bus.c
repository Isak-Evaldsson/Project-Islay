#include <devices/bus.h>
#include <devices/builtin_bus.h>
#include <tasks/locking.h>
#include <uapi/errno.h>
#include <initobj.h>

// TODO: Replace with some kind of dev_log() function or macro
#define LOG(fmt, ...) __LOG(1, "[BUS]", fmt, ##__VA_ARGS__)

static DEFINE_LIST(bus_list);
static atomic_uint_t buses_initalised = ATOMIC_INIT();

static int register_bus(void *arg)
{
    uint32_t flags;
    struct bus *iter, *bus = arg;

    if (EMPTY_STR(bus->name)){
        LOG("Trying to register unamed bus");
        return -EINVAL;
    }

    if (!bus->match) {
        LOG("Missing match function for '%s' bus", bus->name);
        return -EINVAL;
    }

    if (!bus->valid_driver) {
        LOG("Missing valid driver function for '%s' bus", bus->name);
        return -EINVAL;
    };

    LIST_ITER_STRUCT(&bus_list, iter, struct bus, bus_list) {
        if (!strcmp(iter->name, bus->name)) {
            LOG("Already exits bus named %s", bus->name);
            return -EEXIST;
        }
    }

    list_init(&bus->drivers);
    list_init(&bus->unmatched_devices);
    spinlock_init(&bus->lock);
    list_add_last(&bus_list, &bus->bus_list);
    return 0;
}

static int register_driver(void *arg)
{
    int ret;
    uint32_t flags;
    struct driver *iter, *driver = arg;
    struct bus *bus = driver->data.bus;

    if (!driver || EMPTY_STR(driver->data.name) || !bus) {
        LOG("Bad driver object provided");
        return -EINVAL;
    }

    if (!driver->device_attach || !driver->device_remove) {
        LOG("%s driver is missing attach/remove operations", driver->data.name);
        return -EINVAL;
    }

    ret = bus->valid_driver(driver);
    if (ret) {
        LOG("%s is not a vaild %s driver: %i", driver->data.name, bus->name, ret);
        return ret;
    }

    LIST_ITER_STRUCT(&bus->drivers, iter, struct driver, data.bus_list_entry) {
        if (!strcmp(driver->data.name, iter->data.name)) {
            LOG("Already exists driver named %s on the %s bus", driver->data.name, bus->name);
            return -EEXIST;
        }
    }

    list_init(&driver->data.devices);
    list_add_last(&bus->drivers, &driver->data.bus_list_entry);
    return 0;
}

int bus_add_device(struct bus* bus, struct device *parent, struct device *dev)
{
    int ret;
    uint32_t flags;
    struct driver *driver;
    struct list_entry *entry;

    if (!bus || !parent || !dev) {
        return -EINVAL;
    }

    if (dev->driver) {
        return -EEXIST;
    }

    if (!atomic_load(&buses_initalised)) {
        return -EBUSY;
    }

    dev->parent = parent;
    dev->bus = bus;

    spinlock_lock(&bus->lock, &flags);
    LIST_ITER(&bus->drivers, entry) {
        driver = GET_STRUCT(struct driver, data.bus_list_entry, entry);

        if (bus->match(driver, dev)) {
            if (driver->data.drv_data_size) {
                dev->drv_data = kalloc(driver->data.drv_data_size);
                if (!dev->drv_data) {
                    LOG("Failed to allocate drv_data for '%s' driver", driver->data.name);
                    continue;
                }
            }

            ret = driver->device_attach(dev);
            if (ret) {
                kfree(dev->drv_data);
                LOG("Failed to attach device to '%s' driver: %i", driver->data.name, ret);
                continue; // Give the next driver a chance
            }

            dev->driver = driver;
            list_add_last(&driver->data.devices, &dev->driver_list);
            break;
        }
    }

    if (!dev->driver) {
        list_add_last(&bus->unmatched_devices, &dev->driver_list);
        ret = -ENODEV;
    }

    spinlock_unlock(&bus->lock, flags);
    return ret;
}

void bus_remove_device(struct device *dev)
{
    int ret;
    uint32_t flags;
    struct bus *bus = dev->bus;

    if (!bus)
        return;

    spinlock_lock(&bus->lock, &flags);
    if (dev->driver) {
        kfree(dev->drv_data);
        ret = dev->driver->device_remove(dev);
        if (ret) {
            LOG("Failed to remove device from '%s' driver: %i", dev->driver->data.name, ret);
        }
    }
    list_entry_remove(&dev->driver_list);
    spinlock_unlock(&bus->lock, flags);

    dev->bus = NULL;
    dev->driver = NULL;
}

void init_buses()
{
    // No locking needed inside the registeration functions since buses_initialised is not yet set
    kassert(!call_init_objects(INITOBJ_TYPE_BUS, register_bus));
    kassert(!call_init_objects(INITOBJ_TYPE_DRIVER, register_driver));
    atomic_store(&buses_initalised, true);

    builtin_bus_init();
}
