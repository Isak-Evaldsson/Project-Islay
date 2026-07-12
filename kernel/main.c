/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/gdt.h>
#include <arch/interrupts.h>
#include <arch/serial.h>
#include <devices/bus.h>
#include <devices/device.h>
#include <fs.h>
#include <memory/memory_map.h>
#include <tasks/scheduler.h>
#include <utils.h>

#include "kshell.h"
#include "tests/test.h"

void kernel_main(struct boot_data* boot_data)
{
    int ret;

    kprintf("Starting boot sequence...\n");
    ret = memory_map_init_page_frame_allocator();
    if (ret)
        kpanic("Failing to init the page frame manager: %i\n", ret);

    init_gdt();
    init_interrupts();
    scheduler_init();
    init_buses();
    if (arch_initialise_static_devices() < 0) {
        kpanic("Failed to initialise static devices");
    }

    kprintf("Kernel successfully booted at vaddr 0xE0100000 (3.5 GiB + 1 MiB)\n\n");

    ret = fs_init(boot_data);
    if (ret < 0) {
        kpanic("boot failure, failed to initialise vfs %i", ret);
    }
#ifdef RUN_TESTS
    run_post_boot_tests();
#endif

    kshell();

    // Keep kernel alive, waiting for interrupts
    for (;;) {
        wait_for_interrupt();
    }
}
