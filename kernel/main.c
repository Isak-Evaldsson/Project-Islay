/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/gdt.h>
#include <arch/interrupts.h>
#include <arch/serial.h>
#include <arch/tty.h>
#include <devices/device.h>
#include <fs.h>
#include <memory/page_frame_manager.h>
#include <tasks/scheduler.h>
#include <utils.h>

#include "kshell.h"
#include "tests/test.h"

void kernel_main(struct boot_data* boot_data)
{
    kprintf("Starting boot sequence...\n");
    page_frame_manager_init(boot_data);
    drivers_init();

    init_gdt();
    init_interrupts();
    if (arch_initialise_static_devices() < 0) {
        kpanic("Failed to initialise static devices");
    }
    kprintf("Kernel successfully booted at vaddr 0xE0100000 (3.5 GiB + 1 MiB)\n\n");

    int ret = fs_init(boot_data);
    if (ret < 0) {
        kpanic("boot failure, failed to initialise vfs %i", ret);
    }
    scheduler_init();

#ifdef RUN_TESTS
    run_post_boot_tests();
#endif

    // TODO: nice looking boot animation (requires timers)
    kshell();

    // Keep kernel alive, waiting for interrupts
    for (;;) {
        wait_for_interrupt();
    }
}
