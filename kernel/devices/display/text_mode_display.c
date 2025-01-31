/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#include <devices/display/text_mode_display.h>

#include "../internals.h"

#define TEXT_MODE_COLOR (0x0f) /* Black background with white text */

#define TEXT_MODE_CHAR(c) (((uint16_t)TEXT_MODE_COLOR << 8) | (c))

#define LOG(fmt, ...) __LOG(1, "[TEXT_MODE_DISPLAY]", fmt, ##__VA_ARGS__)

static struct text_mode_device* current_dev;
static size_t                   n_buffers;

static void scroll(struct text_mode_device* dev)
{
    // Shift all lines in the vga buffer one step up
    for (size_t row = 1; row < TEXT_MODE_ROWS; row++) {
        memmove(dev->buffer_addr + TEXT_BUFF_IDX(row - 1, 0),
                dev->buffer_addr + TEXT_BUFF_IDX(row, 0),
                sizeof(*dev->buffer_addr) * TEXT_MODE_COLS);
    }

    // Clear last line
    for (size_t col = 0; col < TEXT_MODE_COLS; col++) {
        dev->buffer_addr[TEXT_BUFF_IDX(TEXT_MODE_ROWS - 1, col)] = TEXT_MODE_CHAR(' ');
    }
}

size_t text_get_number_of_displays()
{
    return n_buffers;
}

static void write_char(struct text_mode_device* dev, char c)
{
    if (c == '\n') {
        dev->index_col = 0;
        dev->index_row++;
    } else {
        dev->buffer_addr[TEXT_BUFF_IDX(dev->index_row, dev->index_col)] = TEXT_MODE_CHAR(c);
        dev->index_col++;
    }

    // Handles too long rows
    if (dev->index_col >= TEXT_MODE_COLS) {
        dev->index_col = 0;
        dev->index_row++;
    }

    // Handles terminal scrolling
    if (dev->index_row >= TEXT_MODE_ROWS) {
        scroll(dev);

        // Set current line to last line
        dev->index_col = 0;
        dev->index_row = TEXT_MODE_ROWS - 1;
    }
}

void text_mode_putc(struct text_mode_device* dev, char c)
{
    write_char(dev, c);
    current_dev->ops->set_cursor(dev, dev->index_row, dev->index_col);
}

size_t text_mode_write(struct text_mode_device* dev, const char* str, size_t n)
{
    size_t i;

    for (i = 0; str[i] != '\0' && n > 0; i++, n--) {
        write_char(dev, str[i]);
    }
    dev->ops->set_cursor(dev, dev->index_row, dev->index_col);
    return i;
}

void text_mode_del(struct text_mode_device* dev, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (dev->index_col >= 1) {
            dev->index_col--;
            dev->buffer_addr[TEXT_BUFF_IDX(dev->index_row, dev->index_col)] = TEXT_MODE_CHAR(' ');

            dev->ops->set_cursor(dev, dev->index_row, dev->index_col);
        }
    }
}

void text_mode_clear(struct text_mode_device* dev)
{
    dev->index_col = 0;
    dev->index_row = 0;
    dev->ops->set_cursor(dev, 0, 0);

    for (size_t col = 0; col < TEXT_MODE_COLS; col++) {
        for (size_t row = 0; row < TEXT_MODE_ROWS; row++) {
            dev->buffer_addr[TEXT_BUFF_IDX(row, col)] = TEXT_MODE_CHAR(' ');
        }
    }
}

void text_mode_set_active_display(struct text_mode_device* dev)
{
    struct text_mode_device* new_dev;

    if (current_dev == dev) {
        return;
    }

    new_dev = dev;
    new_dev->ops->display_buffer(new_dev);
    new_dev->ops->set_cursor(new_dev, new_dev->index_row, new_dev->index_col);
    current_dev = new_dev;
}

struct text_mode_device* text_mode_get_display(size_t minor)
{
    LOG("minor %u", minor);
    struct device* dev;

    if (minor > n_buffers) {
        return NULL;
    }

    dev = get_device(&text_mode_display_driver, minor);
    return (dev != NULL) ? GET_STRUCT(struct text_mode_device, dev, dev) : NULL;
}

int init_text_mode_dev(struct text_mode_display_ops* ops, struct text_mode_device* device)
{
    int ret;

    if (!ops->display_buffer || !ops->set_cursor) {
        LOG("Missing required method");
        return -EINVAL;
    }

    if (!device->buffer_addr) {
        LOG("No buffer address defined");
        return -EINVAL;
    }

    device->ops       = ops;
    device->index_col = 0;
    device->index_row = 0;

    ret = register_device(&text_mode_display_driver, &device->dev);
    if (ret < 0) {
        return ret;
    }

    text_mode_clear(device);
    n_buffers++;
    return 0;
}

struct driver text_mode_display_driver = {
    .name = "text_mode_display",
};
