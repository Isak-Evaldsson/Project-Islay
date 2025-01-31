/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#include <arch/i386/io.h>
#include <devices/display/text_mode_display.h>
#include <utils.h>

#if !ARCH(i386)
#error "This driver is only available on i386"
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

static struct text_mode_device vga_devices[N_VGA_BUFFERS];

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
    struct text_mode_device *device;
    log("%u", sizeof(vga_text_ops));

    for (size_t i = 0; i < N_VGA_BUFFERS; i++) {
        device = vga_devices + i;

        device->buffer_start = i * TEXT_MODE_COLS * TEXT_MODE_ROWS;
        device->buffer_addr  = (uint16_t *)(VGA_BUFF_ADDR + device->buffer_start * 2);

        ret = init_text_mode_dev(&vga_text_ops, device);
        if (ret < 0) {
            return ret;
        }
    }
    return 0;
}
