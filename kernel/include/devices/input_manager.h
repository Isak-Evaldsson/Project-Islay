/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef DEVICES_INPUT_H
#define DEVICES_INPUT_H

#include <list.h>
#include <stdbool.h>
#include <stdint.h>

/*
    Generalised api for key input handling,
    independent on whatever it's a usb devices or ps2 device etc.
*/

/* Representing system input as an input event object */
typedef struct input_event_t {
    uint16_t      key_code; /* The key input */
    unsigned char status;   /* status flags */
} input_event_t;

/*
    Flags
*/
enum input_event_status_bits {
    INPUT_RELEASED,  // 1 if released, otherwise pressed
    UPPER_CASE,      // 1 if uppercase, otherwise lowercase

    // Key Modifier bits, 1 if set
    MOD_SHIFT,
    MOD_CTRL,
    MOD_ALT,
    MOD_SUPER,
};

/*
    Macros to check the flags
*/
#define CHECK_IF_PRESSED(x)    (!(x & (1 << INPUT_RELEASED)))
#define CHECK_IF_UPPER_CASE(x) (x & (1 << UPPER_CASE))
#define CHECK_IF_SHIFT(x)      (x & (1 << MOD_SHIFT))
#define CHECK_IF_CTRL(x)       (x & (1 << MOD_CTRL))
#define CHECK_IF_ALT(x)        (x & (1 << MOD_ALT))
#define CHECK_IF_SUPER(x)      (x & (1 << MOD_SUPER))
/*
    All key-codes, all printable characters correspond to their ascii equivalents
*/
enum key_codes {
    INVALID_KEY = 0,  // clang-format off

    /* First other section */
    KEY_SPACE     = ' ',    KEY_EXCLAMATION = '!',  KEY_QUOTE   = '"',  
    KEY_HASH      = '#',    KEY_DOLLAR      = '$',  KEY_PERCENT = '%', 
    KEY_AMPERSAND = '&',    KEY_APOSTROPHE  = '\'', KEY_LPAREN  = '(',
    KEY_RPAREN    = ')',    KEY_ASTERISK    = '*',  KEY_PLUS    = '+',
    KEY_COMMA     = ',',    KEY_MINUS       = '-',  KEY_DOT     = '.',
    KEY_FSLASH    = '/',    
    
    /* Numbers */
    KEY_0 = '0',            KEY_1 = '1',            KEY_2 = '2', 
    KEY_3 = '3',            KEY_4 = '4',            KEY_5 = '5',    
    KEY_6 = '6',            KEY_7 = '7',            KEY_8 = '8',
    KEY_9 = '9',

    /* Second other ascii section */
    KEY_COLON    = ':',     KEY_SEMI     = ';',     KEY_LESS     = '<',
    KEY_EQUAL    = '=',     KEY_GREATER  = '>',     KEY_QUESTION = '?',
    KEY_AT       = '@',

    /* NOTE: Upper case is encoded with a modifier instead */

    /* Third other section */
    KEY_LSBRACKET  = '[',   KEY_BSLASH     = '\\',  KEY_RSBRACKET  = ']',
    KEY_CARET      = '^',   KEY_UNDERSCORE = '_',   KEY_BACKTICK   = '`',

    /* Letters */
    KEY_A = 'a',            KEY_B = 'b',            KEY_C = 'c',
    KEY_D = 'd',            KEY_E = 'e',            KEY_F = 'f',
    KEY_G = 'g',            KEY_H = 'h',            KEY_I = 'i',
    KEY_J = 'j',            KEY_K = 'k',            KEY_L = 'l',
    KEY_M = 'm',            KEY_N = 'n',            KEY_O = 'o',
    KEY_P = 'p',            KEY_Q = 'q',            KEY_R = 'r',
    KEY_S = 's',            KEY_T = 't',            KEY_U = 'u',
    KEY_V = 'v',            KEY_W = 'w',            KEY_X = 'x',
    KEY_Y = 'y',            KEY_Z = 'z',

    /* Fourth other section */
    KEY_LCUBRACKET = '{',
    KEY_BAR        = '|',
    KEY_RCUBRACKET = '}',
    KEY_TILDE      = '~',

    /* Special keys, i.e. non-printable */
    KEY_ESCAPE,             KEY_BACKSPACE,          KEY_TAB,
    KEY_ENTER,              KEY_CTRL,               KEY_LSHIFT,
    KEY_RSHIFT,             KEY_PRTSC,              KEY_ALT,
    KEY_CAPS,               KEY_F1,                 KEY_F2,
    KEY_F3,                 KEY_F4,                 KEY_F5,
    KEY_F6,                 KEY_F7,                 KEY_F8,
    KEY_F9,                 KEY_F10,                KEY_F11,
    KEY_F12,                KEY_NUMLOCK,            KEY_SCROLLOCK,
    KEY_HOME,               KEY_UP,                 KEY_PAGEUP,
    KEY_LEFT,               KEY_CENTER,             KEY_RIGHT,
    KEY_END,                KEY_DOWN,               KEY_PAGEDOWN,
    KEY_INSERT,             KEY_DELETE,             KEY_BREAK,

    /* Multi-media keys */
    KEY_MM_PREV,            KEY_MM_NEXT,            KEY_MM_MUTE,
    KEY_MM_CALC,            KEY_MM_PLAY,            KEY_MM_STOP,
    KEY_MM_VOL_DOWN,        KEY_MM_VOL_UP,          KEY_MM_HOME,
    KEY_MM_APPS,            KEY_MM_SEARCH,          KEY_MM_FAVORITES,
    KEY_MM_REFRESH,         KEY_MM_FORWARD,         KEY_MM_BACK,
    KEY_MM_COMPUTER,        KEY_MM_EMAIL,           KEY_MM_SELECT,

    /* ACPI Keys */
    KEY_ACPI_POWER,         KEY_ACPI_SLEEP,        KEY_ACPI_WAKE,

    VALID_KEY_MAX  // clang-format on
};

#define KEY_ASCII_PRINTABLE(key) (key >= KEY_SPACE && key <= KEY_TILDE)
#define KEY_LETTER(key)          (key >= KEY_A && key <= KEY_Z)

/* Initialises the input manager */
void input_manager_init();

/* Allows input drivers to send key events to the input driver */
void input_manager_send_event(uint16_t key_code, unsigned char status);

/*
    To read events report to the input manger, an input subscriber object must be created and
    supplied to the registration functions bellow.

    When actively subscribing, the on_events_receive callback will be called once events are
    available.
*/
struct input_subscriber {
    int (*on_events_received)(input_event_t event);
    struct list_entry list;
};

/*
    Register an input subscriber object to actively subscribe on new input events, when a new event
   is available, the objects associated callback will be called.
*/
int input_manger_subscribe(struct input_subscriber* subscriber);

/* Unsubscribe from inputs events, the callback associated to the subscriber object will no longer
 * be called on new input events. */
void input_manger_unsubscribe(struct input_subscriber* subscriber);

#endif /* DEVICES_INPUT_H */
