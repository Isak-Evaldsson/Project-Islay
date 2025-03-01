/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#ifndef DEVICES_UNICODE_H
#define DEVICES_UNICODE_H

#include <stdint.h>

/* Special not a character codepoint */
#define UCS2_NOCHAR 0xFFFF

/*
    Characters encoded according to the UCS2 standard, as defined in ISO/IEC 10646
*/
typedef uint16_t ucs2_t;

/* Checks if the first byte from an utf8 character is ASCII compatible */
#define UTF8_ASCII_COMPATIBLE(byte) (!(byte & 0x80))

#define IS_VALID_UCS2(c) ()

/*
    TODO: UTF8 <--> UC2 conversion functions
*/

/*
    UTF-8 Codepoints, see https://en.wikipedia.org/wiki/List_of_Unicode_characters
*/
enum c0_codepoints {
    UTF8_NUL = '\0',
    UTF8_SOH,
    UTF8_STX,
    UTF8_ETX,
    UTF8_EOT,
    UTF8_ENQ,
    UTF8_ACK,
    UTF8_BEL,
    UTF8_BS,
    UTF8_HT,
    UTF8_LF,
    UTF8_VT,
    UTF8_FF,
    UTF8_CR,
    UTF8_SO,
    UTF8_SI,
    UTF8_DLE,
    UTF8_DC1,
    UTF8_DC2,
    UTF8_DC3,
    UTF8_DC4,
    UTF8_NAK,
    UTF8_SYN,
    UTF8_ETB,
    UTF8_CAN,
    UTF8_EM,
    UTF8_SUB,
    UTF8_ESC,
    UTF8_FS,
    UTF8_GS,
    UTF8_RS,
    UTF8_US,
    UTF8_DELL = 127,
};

enum basic_latin_codepoints {
    UTF8_SPACE       = ' ',
    UTF8_EXCLAMATION = '!',
    UTF8_QUOTE       = '"',
    UTF8_HASH        = '#',
    UTF8_DOLLAR      = '$',
    UTF8_PERCENT     = '%',
    UTF8_AMPERSAND   = '&',
    UTF8_APOSTROPHE  = '\'',
    UTF8_LPAREN      = '(',
    UTF8_RPAREN      = ')',
    UTF8_ASTERISK    = '*',
    UTF8_PLUS        = '+',
    UTF8_COMMA       = ',',
    UTF8_MINUS       = '-',
    UTF8_DOT         = '.',
    UTF8_FSLASH      = '/',
    UTF8_0           = '0',
    UTF8_1           = '1',
    UTF8_2           = '2',
    UTF8_3           = '3',
    UTF8_4           = '4',
    UTF8_5           = '5',
    UTF8_6           = '6',
    UTF8_7           = '7',
    UTF8_8           = '8',
    UTF8_9           = '9',
    UTF8_COLON       = ':',
    UTF8_SEMI        = ';',
    UTF8_LESS        = '<',
    UTF8_EQUAL       = '=',
    UTF8_GREATER     = '>',
    UTF8_QUESTION    = '?',
    UTF8_AT          = '@',
    UTF8_CAP_A       = 'A',
    UTF8_CAP_B       = 'B',
    UTF8_CAP_C       = 'C',
    UTF8_CAP_D       = 'D',
    UTF8_CAP_E       = 'E',
    UTF8_CAP_F       = 'F',
    UTF8_CAP_G       = 'G',
    UTF8_CAP_H       = 'H',
    UTF8_CAP_I       = 'I',
    UTF8_CAP_J       = 'J',
    UTF8_CAP_K       = 'K',
    UTF8_CAP_L       = 'L',
    UTF8_CAP_M       = 'M',
    UTF8_CAP_N       = 'N',
    UTF8_CAP_O       = 'O',
    UTF8_CAP_P       = 'P',
    UTF8_CAP_Q       = 'Q',
    UTF8_CAP_R       = 'R',
    UTF8_CAP_S       = 'S',
    UTF8_CAP_T       = 'T',
    UTF8_CAP_U       = 'U',
    UTF8_CAP_V       = 'V',
    UTF8_CAP_W       = 'W',
    UTF8_CAP_X       = 'X',
    UTF8_CAP_Y       = 'T',
    UTF8_CAP_Z       = 'Z',
    UTF8_LSBRACKET   = '[',
    UTF8_BSLASH      = '\\',
    UTF8_RSBRACKET   = ']',
    UTF8_CARET       = '^',
    UTF8_UNDERSCORE  = '_',
    UTF8_BACKTICK    = '`',
    UTF8_A           = 'a',
    UTF8_B           = 'b',
    UTF8_C           = 'c',
    UTF8_D           = 'd',
    UTF8_E           = 'e',
    UTF8_F           = 'f',
    UTF8_G           = 'g',
    UTF8_H           = 'h',
    UTF8_I           = 'i',
    UTF8_J           = 'j',
    UTF8_K           = 'k',
    UTF8_L           = 'l',
    UTF8_M           = 'm',
    UTF8_N           = 'n',
    UTF8_O           = 'o',
    UTF8_P           = 'p',
    UTF8_Q           = 'q',
    UTF8_R           = 'r',
    UTF8_S           = 's',
    UTF8_T           = 't',
    UTF8_U           = 'u',
    UTF8_V           = 'v',
    UTF8_W           = 'w',
    UTF8_X           = 'x',
    UTF8_Y           = 'y',
    UTF8_Z           = 'z',
    UTF8_LCUBRACKET  = '{',
    UTF8_BAR         = '|',
    UTF8_RCUBRACKET  = '}',
    UTF8_TILDE       = '~',
};

#endif /* DEVICES_UNICODE_H */
