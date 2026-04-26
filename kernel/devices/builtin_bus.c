#include <devices/builtin_bus.h>
#include <utils.h>
#include <uapi/errno.h>
#include <list.h>

static struct device root_dev;
static DEFINE_LIST(uninitalized);

int builtin_add_device(struct builtin_device *dev, const char *name, void *data_ptr, size_t size)
{
    int ret;

    dev->name = name;
    if (data_ptr) {
        dev->data_ptr = kalloc(size);
        if (!dev->data_ptr)
            return -ENOMEM;

        memcpy(dev->data_ptr, data_ptr, size);
    }

    ret = _add_builtin_device(&root_dev, dev);
    if (ret == -EBUSY) {
        list_add_last(&uninitalized, &builtin_to_device(dev)->driver_list);
        return 0;
    }

    return ret;
}

void builtin_bus_init()
{
    struct builtin_device *dev;
    struct list_entry *entry;

    LIST_ITER_SAFE_REMOVAL(&uninitalized, entry) {
        dev = (struct builtin_device *)GET_STRUCT(struct device, driver_list, entry);

        // Don't want to cause trouble with the bus logic
        list_entry_remove(&builtin_to_device(dev)->driver_list);
        kassert(!_add_builtin_device(&root_dev, dev));
    }
}

static int builtin_match(struct builtin_driver *drv, struct builtin_device *dev)
{
    return !strcmp(dev->name, drv->data.name);
}

static int bultin_valid_driver(struct builtin_driver *drv)
{
    const char *name = drv->data.name;

    if (EMPTY_STR(name))
        return -EINVAL;

    return 0;
}

DEFINE_BUS(builtin, builtin_match, bultin_valid_driver)