/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#ifndef DEVICES_DISPLAY_TEXT_MODE_DISPLAY_H
#define DEVICES_DISPLAY_TEXT_MODE_DISPLAY_H
/*
    Text mode display - A generic api to implement and interact with text mode based displays
*/

#include <arch/platfrom.h>
#include <devices/device.h>
#include <devices/unicode.h>
#include <stddef.h>
#include <stdint.h>

#define TEXT_MODE_COLS 80
#define TEXT_MODE_ROWS 25

/* macro converting row and col values to text buffer index */
#define TEXT_BUFF_IDX(row, col) (TEXT_MODE_COLS * (row) + (col))

/*
    Object representing a specific ext mode buffer/device
*/
struct text_mode_device {
    struct device                 dev;
    struct text_mode_display_ops* ops;

    // Current state
    size_t index_row;
    size_t index_col;

    // Hardware specific text mode start address
    size_t buffer_start;

    // Memory mapped address to the text mode buffer
    volatile uint16_t* buffer_addr;
};

/*
    Operations required for each specific text mode display implementation
*/
struct text_mode_display_ops {
    // Display the specific buffer associated with the device
    void (*display_buffer)(struct text_mode_device* device);

    // Adjust the cursor position fo the buffer associated with the device
    void (*set_cursor)(struct text_mode_device* device, size_t row, size_t col);
};

/* Register and initialize a text mode device to the text mode driver */
int init_text_mode_dev(struct text_mode_display_ops* ops, struct text_mode_device* device);

/*
    Text mode api to allow interaction with the vga text mode display
*/

/* Operations on a text mode display */
void   text_mode_putc(struct text_mode_device* dev, char c);
size_t text_mode_write(struct text_mode_device* dev, const char* str, size_t n);
void   text_mode_del(struct text_mode_device* dev, size_t n);
void   text_mode_clear(struct text_mode_device* dev);

void text_mode_write_char(struct text_mode_device* dev, ucs2_t c);

/* Switch to the text mode display */
void text_mode_set_active_display(struct text_mode_device* dev);

/* Get the text mode display with the specifed minor number, or null if it does exits */
struct text_mode_device* text_mode_get_display(size_t minor);

/* How many text mode buffers are available to the system */
size_t text_get_number_of_displays();

#if ARCH(i386)
/* Creates vga based text mode devices. Returns 0 on success, -ERRNO on failure */
int create_vga_text_display();
#endif

/* For driver registration */
extern struct driver text_mode_display_driver;

#endif /* DEVICES_DISPLAY_TEXT_MODE_DISPLAY_H */
