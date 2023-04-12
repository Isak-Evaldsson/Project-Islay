/*
    Simple "8042" PS/2 Controller driver,
    see: https://wiki.osdev.org/%228042%22_PS/2_Controller
*/
#include <arch/i386/interrupts/pic.h>
#include <arch/i386/interrupts/ps2.h>
#include <arch/i386/io.h>
#include <devices/ps2_keyboard.h>
#include <stdbool.h>
#include <stddef.h>

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

/* Marks successfull initiation */
static bool initialised = false;

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

static void ps2_receive_data(unsigned char data)
{
    kprintf("Received data '%x' from ps2 driver \n", data);
}

/*
    Interrupt handler to be run when the PS/2 device sends an interrupt through the PIC
*/
void ps2_interrupt_handler()
{
    unsigned char scancode = inb(PS2_DATA_PORT);

    if (scancode == 0xaa && !initialised) {
        initialised = true;
        ps2_keyboard_register("i8042", ps2_receive_data);
        return;
    }

    ps2_keyboard_send(scancode);
}
