#ifndef DEVICES_INPUT_H
#define DEVICES_INPUT_H

#include <stdbool.h>
#include <stdint.h>

/*
    Generalised api for key input handling,
    independent on whatever it's a usb devices or ps2 device etc.
*/

typedef struct input_event_t {
    uint16_t      key_code; /* The key input */
    unsigned char status;   /* flags, bit 0 indicates pressed or not  */
} input_event_t;

/*
    Flags
*/
#define INPUT_PRESSED 0x00
#define INPUT_RELEASE 0x01

/*
    Macros to check the flags
*/
#define CHECK_IF_PRESSED(x) (x & 0x01)

/*
    All key-codes
*/
typedef enum {
    INVALID_KEY = 0,
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_ESCAPE,
    KEY_MINUS,
    KEY_EQUAL,
    KEY_BACKSPACE,
    KEY_TAB,
};

/* Initialises the input manager */
void input_manager_init();

/* Allows input drivers to send key events to the input driver */
void input_manager_send_event(uint16_t key_code, unsigned char status);

/*
    Read event queue, used by the kernel to receive key events.
    Returns true if there's an event available.
*/
bool input_manager_get_event(input_event_t* event);

/*
    If fetches an event from the queue if available, else it waits for a new event to be pushed to
    the queue.
*/
void input_manager_wait_for_event(input_event_t* event);

#endif /* DEVICES_INPUT_H */
