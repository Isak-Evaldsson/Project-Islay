/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
/*
    Simple "8042" PS/2 Controller driver,
    see: https://wiki.osdev.org/%228042%22_PS/2_Controller
*/
#include <arch/i386/io.h>
#include <devices/keyboard/ps2_keyboard.h>
#include <stdbool.h>
#include <stddef.h>
#include <utils.h>

#include "ps2.h"

#define GET_BIT(num, bit) ((num & (1 << bit)) >> bit)

#define PS2_DATA_PORT 0x60
#define PS2_CMD_PORT  0x64

// Commands
#define DISABLE_PORT1     0xad
#define ENABLE_PORT1      0xae
#define DISABLE_PORT2     0xa7
#define ENABLE_PORT2      0xa8
#define READ_CONFIG_BYTE  0x20
#define WRITE_CONFIG_BYTE 0x60

#define WAIT_FOR_READ()                     \
    while ((inb(PS2_CMD_PORT) & 0x01) == 0) \
        ;
#define WAIT_FOR_WRITE()                    \
    while ((inb(PS2_CMD_PORT) & 0x02) != 0) \
        ;

/* Marks successful initiation */
static bool initialised = false;

/*
    Intermidiate buffer for scancode, in order to make the top half irq as small as possible, the
    scancodes are simply stored in a buffer to be later processed within the bottom half

    TODO: Definitely NOT threadsafe, it only works with a single producer and consumer (due to
    loads/stores being atomic in x86), will need to be addressed once going smp, by for example
    using spinlocks, lock-free algorithms or by limiting the interrupt to a single core the
    interrupt to a single core.
*/
#define SCAN_CODE_BUFF_SIZE 100
static unsigned int  read_idx;
static unsigned int  write_idx;
static unsigned char scancode_buffer[SCAN_CODE_BUFF_SIZE];

/*
    Initialises the PS/2 Controller
*/
void ps2_init()
{
    bool dual_port = true;  // Assume dual-port, assumption may be disproved during initialisation
    uint8_t config_byte;

    /*
        This driver currently assumes the following things:
         * The device does not have "USB Legacy Support" which interferes with the PS/2 controller
         * PS/2 port exits, no fancy ACPI going on here
         * No hotplug support
    */

    // Disable PS/2 devices
    outb(PS2_CMD_PORT, DISABLE_PORT1);
    outb(PS2_CMD_PORT, DISABLE_PORT2);

    // Flush output buffer
    inb(PS2_DATA_PORT);

    // Read configuration
    outb(PS2_CMD_PORT, READ_CONFIG_BYTE);
    WAIT_FOR_READ();
    config_byte = inb(PS2_DATA_PORT);

    // Check if port 2 is not disabled, i.e. 0
    if (GET_BIT(config_byte, 5) == 0) {
        // Since it's not disabled even after disabling it, we can assume
        // that there is no second port
        dual_port = false;
        kprintf("No second port\n");
    }

    // Clear bit 0, 1 (i.e. disable interrupts)
    config_byte = config_byte & ~(1 << 0);
    config_byte = config_byte & ~(1 << 1);

    // Write config back to controller
    outb(PS2_CMD_PORT, WRITE_CONFIG_BYTE);
    WAIT_FOR_WRITE();
    outb(PS2_DATA_PORT, config_byte);

    // Controller self-test
    outb(PS2_CMD_PORT, 0xaa);
    WAIT_FOR_READ();
    if (inb(PS2_DATA_PORT) != 0x55) {
        kprintf("PS/2 controller failed self-test\n");
    }

    // Verify if it's a dual-port device
    if (dual_port) {
        outb(PS2_CMD_PORT, ENABLE_PORT2);  // enable second port

        // read configuration
        outb(PS2_CMD_PORT, READ_CONFIG_BYTE);
        WAIT_FOR_READ();
        if (GET_BIT(inb(PS2_DATA_PORT), 5) == 1) {
            // port 2 still disabled after being enabled, its a single port device
            dual_port = false;
        }

        if (dual_port) {
            outb(PS2_CMD_PORT, DISABLE_PORT2);
        }
    }

    // Port self-tests
    outb(PS2_CMD_PORT, 0xab);
    WAIT_FOR_READ();
    if (inb(PS2_DATA_PORT) != 0x00) {
        kprintf("PS/2 controller port 1 failed self-test\n");
    }

    if (dual_port) {
        outb(PS2_CMD_PORT, 0xa9);
        WAIT_FOR_READ();
        if (inb(PS2_DATA_PORT) != 0x00) {
            kprintf("PS/2 controller port 2 failed self-test\n");
        }
    }

    // Enable ports
    // TODO: enable second port if available
    outb(PS2_CMD_PORT, ENABLE_PORT1);

    // Configure controller
    outb(PS2_CMD_PORT, READ_CONFIG_BYTE);
    WAIT_FOR_READ();
    config_byte = inb(PS2_DATA_PORT);

    // enable interrupt on port 1
    config_byte = config_byte | 0x01;

    // enable translation to set 1
    config_byte = config_byte | (1 << 6);

    outb(PS2_CMD_PORT, WRITE_CONFIG_BYTE);
    WAIT_FOR_WRITE();
    outb(PS2_DATA_PORT, config_byte);

    // Reset controller
    WAIT_FOR_WRITE();
    outb(PS2_DATA_PORT, 0xff);
    WAIT_FOR_READ();

    if (inb(PS2_DATA_PORT) != 0xfa) {
        kprintf("Failed to reset device on PS/2 port 1\n");
    }

    kprintf("i8042 PS/2 controller enabled\n");
}

static void ps2_send_kbd_data(unsigned char data)
{
    log("Received data '%x' from ps2 driver", data);
    WAIT_FOR_WRITE();
    outb(PS2_DATA_PORT, data);
}

void ps2_top_irq(struct interrupt_stack_state *state, uint32_t interrupt_number)
{
    (void)state;
    (void)interrupt_number;

    unsigned char scancode = inb(PS2_DATA_PORT);

    if (scancode == 0xaa && !initialised) {
        initialised = true;
        ps2_keyboard_register("i8042", ps2_send_kbd_data);
        return;
    }

    // We always keep a single slot free since, otherwise we can't distinguish from the empty
    // buffer or full buffer case.
    if ((write_idx + 1) % SCAN_CODE_BUFF_SIZE == read_idx) {
        log("scancode buffer overflowing, unable to process any more scancodes");
        return;
    }

    scancode_buffer[write_idx] = scancode;
    mem_barrier_full();  // Ensure that the store happens before incrementing index, otherwise
                         // the consumer may see an incorrect value
    write_idx = (write_idx + 1) % SCAN_CODE_BUFF_SIZE;
}

void ps2_bottom_irq(uint32_t irq_no)
{
    (void)irq_no;
    unsigned char scancode;

    while (read_idx != write_idx) {
        scancode = scancode_buffer[read_idx];
        mem_barrier_full();  // Ensure that the load happens before incrementing the index,
                             // otherwise the producer may overwrite the value before its read
        read_idx = (read_idx + 1) % SCAN_CODE_BUFF_SIZE;

        ps2_keyboard_send(scancode);
    }
}
