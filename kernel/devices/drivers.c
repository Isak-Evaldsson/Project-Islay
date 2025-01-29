/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/

#include "internals.h"

#define LOG(fmt, ...) __LOG(1, "[DRIVER]", fmt, ##__VA_ARGS__)

extern struct driver console;

static struct driver *driver_table[] = {
    NULL,  // Major num 0 reserved for errors
};
static_assert(COUNT_ARRAY_ELEMS(driver_table) < MAJOR_MAX);

void drivers_init()
{
    for (size_t i = 1; i < COUNT_ARRAY_ELEMS(driver_table); i++) {
        struct driver *driver = driver_table[i];

        size_t len = strlen(driver->name);
        if (!len || len > DRIVER_NAME_MAXLEN) {
            LOG("Invalid driver name: '%s', index: %u", len, i);
            continue;
        }

        driver->major      = i;  // mark as registered
        driver->next_minor = 1;
    }
}

struct driver *get_driver(unsigned int major)
{
    if (!major || major >= COUNT_ARRAY_ELEMS(driver_table)) {
        return NULL;
    }
    return driver_table[major];
}
