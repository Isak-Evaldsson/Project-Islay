/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#include <arch/i686/io.h>
#include <arch/serial.h>

#define PORT 0x3f8

/* check if we can transmit a new char */
static int is_transmit_empty()
{
    return inb(PORT + 5) & 0x20;
}

/* Initialises the serial port, returns 0 on success */
int serial_init()
{
    outb(PORT + 1, 0x00);  // Disable all interrupts
    outb(PORT + 3, 0x80);  // Enable DLAB (set baud rate divisor)
    outb(PORT + 0, 0x03);  // Set divisor to 3 (lo byte) 38400 baud
    outb(PORT + 1, 0x00);  //                  (hi byte)
    outb(PORT + 3, 0x03);  // 8 bits, no parity, one stop bit
    outb(PORT + 2, 0xC7);  // Enable FIFO, clear them, with 14-byte threshold
    outb(PORT + 4, 0x0B);  // IRQs enabled, RTS/DSR set
    outb(PORT + 4, 0x1E);  // Set in loopback mode, test the serial chip
    outb(PORT + 0,
         0xAE);  // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if (inb(PORT + 0) != 0xAE) {
        return 1;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(PORT + 4, 0x0F);
    return 0;
}

/* Writes the supplied char to the serial port */
void serial_put(char a)
{
    while (is_transmit_empty() == 0)
        ;

    outb(PORT, a);
}

/* Writes the supplied string to the serial port */
void serial_write(const char *data, size_t size)
{
    char c;

    for (size_t i = 0; i < size; i++) {
        c = data[i];

        // Inserts missing carriage return
        if (c == '\n' && (i == 0 || data[i - 1] != '\r')) {
            serial_put('\r');
        }
        serial_put(c);
    }
}
