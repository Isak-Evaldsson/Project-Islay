#include <devices/input.h>
#include <devices/ps2_keyboard.h>
#include <klib/klib.h>
#include <stdbool.h>
#include <stddef.h>

#define KBD_CMD_BUFF_SIZE 16

#define BREAK_CODE 0x80

// TODO: Allow multiple keyboard support
static struct {
    char*                   name;
    keyboard_receive_data_t callback
} device;

// Keyboard state
static bool          key_pressed = false;
static unsigned char key;

// maps set 1 scan codes to input event key codes
static uint16_t set1_to_keycode[] = {
    // TODO: Add all keys
    INVALID_KEY, KEY_ESCAPE, KEY_1, KEY_2,     KEY_3,     KEY_4,         KEY_5,   KEY_6, KEY_7,
    KEY_8,       KEY_9,      KEY_0, KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE, KEY_TAB, KEY_Q, KEY_W,
    KEY_E,       KEY_R,      KEY_T, KEY_T,     KEY_U,     KEY_I,         KEY_O,   KEY_P};

void ps2_keyboard_register(char* device_name, keyboard_receive_data_t fn)
{
    kassert(fn != NULL);

    // Check if trying to register multiple keyboards
    if (device.callback != NULL) {
        kpanic(
            "PS2 driver currently not supporting multiple ps2 keyboard devices\nRegistering "
            "%s, "
            "but %s previously registered",
            device_name, device.name);
    }

    // Registering our device
    device.name     = device_name;
    device.callback = fn;

    input_manager_init();
    kprintf("PS/2 keyboard driver: successfully registered %s\n", device_name);
}

void ps2_keyboard_send(unsigned char scancode)
{
    unsigned char key_code;

    // Entering key pressed state.
    if (!key_pressed) {
        key_pressed = true;
        key         = scancode;

        // leaving pressed state
    } else if ((scancode - BREAK_CODE) == key) {
        key_pressed = false;
    }

    // Check if we receive an invalid scancode
    if (key > (sizeof(set1_to_keycode) / sizeof(set1_to_keycode[0]))) {
        kprintf("PS/2 keyboard: invalid scancode: %u\n", key);
        return;
    }

    key_code = set1_to_keycode[key];
    if (key_code != INVALID_KEY) {
        input_manager_send_event(key_code, key_pressed ? INPUT_PRESSED : INPUT_RELEASE);
    }
}