#ifndef DEVICES_BUILTIN_BUS
#define DEVICES_BUILTIN_BUS

#include <devices/bus.h>

/*
 * The builtin bus handles devices that is a fixed of the platfrom or architecture
 * and theteby lack some dynamic device discovery mechanism.
 */
DEFINE_BUS_DEVICE_STRUCT(builtin,
    const char *name;
    /*
     * To allow the device to pass init/parameter data to its driver.
     * Will get copied and free'd once the device is succesfully attached.
     */
    void *data_ptr;
)
DEFINE_BUS_DRIVER_STRUCT(builtin)
DECLARE_BUS(builtin)

int builtin_add_device(struct builtin_device *dev, const char *name, void *data_ptr, size_t size);
void builtin_bus_init();

#endif /* DEVICES_BUILTIN_BUS */