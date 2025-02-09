/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#include <arch/arch.h>
#include <devices/display/text_mode_display.h>
#include <utils.h>

int arch_initialise_static_devices()
{
    int ret;
    ret = create_vga_text_display();
    if (ret < 0) {
        log("Failed to create vga device %i", ret);
        return ret;
    }

    return 0;
}
