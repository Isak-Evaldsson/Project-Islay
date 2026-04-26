/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#include <arch/i686/io.h>
#include <devices/builtin_bus.h>
#include <devices/display/text_mode_display.h>
#include <utils.h>

#if !ARCH(i686)
#error "This driver is only available on i686"
#endif

/*
    Pointer to x86 vga buffer, the physical address B8000 is re-mapped to page 1016 at higher half
    address 0xE0000000 page directory. 0xE0000000 + 1016 * 4096
*/
#define VGA_BUFF_ADDR (0xE03F8000)

/*
    crtc register index, used configure the VGA device, see
    http://www.osdever.net/FreeVGA/vga/crtcreg.htm for more details
*/
#define CRTC_IDX_START_ADDRESS 0x0C
#define CRTC_IDX_CURSOR_LOC    0x0E

#define N_VGA_BUFFERS (8)

struct vga_text_device {
    struct text_mode_device text_mode_dev;
    struct builtin_device dev;
};

static struct vga_text_device vga_devices[N_VGA_BUFFERS];

static void write_crtc_reg(uint8_t index, uint16_t value)
{
    outb(0x3D4, index);
    outb(0x3D5, (value >> 8) & 0xff);
    outb(0x3D4, index + 1);
    outb(0x3D5, value & 0xff);
}

void vga_text_display_buffer(struct text_mode_device *device)
{
    write_crtc_reg(CRTC_IDX_START_ADDRESS, device->buffer_start);
}

void vga_text_set_cursor(struct text_mode_device *device, size_t row, size_t col)
{
    uint16_t pos = TEXT_BUFF_IDX(row, col) + device->buffer_start;

    write_crtc_reg(CRTC_IDX_CURSOR_LOC, pos);
}

static struct text_mode_display_ops vga_text_ops = {
    .display_buffer = vga_text_display_buffer,
    .set_cursor     = vga_text_set_cursor,
};

int create_vga_text_display()
{
    int                      ret;
    struct vga_text_device *device;

    for (size_t i = 0; i < N_VGA_BUFFERS; i++) {
        device = vga_devices + i;
        device->text_mode_dev.buffer_start = i * TEXT_MODE_COLS * TEXT_MODE_ROWS;
        device->text_mode_dev.buffer_addr  =
                (uint16_t *)(VGA_BUFF_ADDR + device->text_mode_dev.buffer_start * 2);

        ret = init_text_mode_dev(&vga_text_ops, &device->text_mode_dev);
        if (ret < 0) {
            return ret;
        }

        ret = builtin_add_device(&device->dev, "tty", &device, sizeof(device));
        if (ret)
            return ret;
    }
    return 0;
}
