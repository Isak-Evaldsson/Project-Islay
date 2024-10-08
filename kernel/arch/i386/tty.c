/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/tty.h>
#include <stdint.h>

#include "io.h"

// VGA buffer configuration
#define VGA_COLS 80
#define VGA_ROWS 25

// macro converting row and col values to vga buffer index
#define vga_index(row, col) (VGA_COLS * (row) + (col))

// Pointer to x86 vga buffer, the physical address B8000 is re-mapped to page 1023 at higher
// half address 0xE0000000 page directory. 0xE0000000 + 1023 * 4096
volatile uint16_t *vga_buffer = (uint16_t *)0xE03FF000;

const int TERM_WIDTH  = VGA_COLS;
const int TERM_HEIGHT = VGA_ROWS;

// Current terminal postion
int     term_col   = 0;
int     term_row   = 0;
uint8_t term_color = 0x0F;  // Black background, white text

// Clears terminal and sets correct background color
void term_init()
{
    for (size_t col = 0; col < VGA_COLS; col++) {
        for (size_t row = 0; row < VGA_ROWS; row++) {
            const size_t index = (VGA_COLS * row) + col;
            vga_buffer[index]  = (term_color << 8) | ' ';
        }
    }
}

void term_put(char c)
{
    if (c == '\n') {
        term_col = 0;
        term_row++;
    } else {
        vga_buffer[vga_index(term_row, term_col)] = ((uint16_t)term_color << 8) | c;
        term_col++;
    }

    // Handles too long rows
    if (term_col >= VGA_COLS) {
        term_col = 0;
        term_row++;
    }

    // Handles terminal scrolling
    if (term_row >= VGA_ROWS) {
        // Shift all lines in the vga buffer one step up
        for (size_t i = 1; i < VGA_ROWS; i++) {
            for (size_t j = 0; j < VGA_COLS; j++) {
                vga_buffer[vga_index(i - 1, j)] = vga_buffer[vga_index(i, j)];
            }
        }

        // Clears last line
        for (size_t i = 0; i < VGA_ROWS; i++) {
            vga_buffer[vga_index(VGA_ROWS - 1, i)] = (term_color << 8) | ' ';
        }

        // Set current line to last line
        term_col = 0;
        term_row = VGA_ROWS - 1;
    }
}

void term_writestring(const char *str)
{
    for (size_t i = 0; str[i] != '\0'; i++)
        term_put(str[i]);
}

void term_write(const char *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
        term_put(data[i]);
}

void term_clear()
{
    term_col = 0;
    term_row = 0;

    for (size_t col = 0; col < VGA_COLS; col++) {
        for (size_t row = 0; row < VGA_ROWS; row++) {
            const size_t index = (VGA_COLS * row) + col;
            vga_buffer[index]  = (term_color << 8) | ' ';
        }
    }
}
