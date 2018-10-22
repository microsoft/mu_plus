/** @file

  Implements key mapping and key labels for supported languages (US-EN only at the moment).

  Copyright (c) 2015 - 2018, Microsoft Corporation.

  All rights reserved.
  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#ifndef _OSK_KEY_MAPPING_H_
#define _OSK_KEY_MAPPING_H_

#define UNICODE_CHAR_ERASE_TO_THE_LEFT                              0x232B
#define UNICODE_CHAR_RETURN_SYMBOL                                  0x23CE
#define UNICODE_CHAR_WHITE_SQUARE_CONTAINING_BLACK_SMALL_SQUARE     0x25A3
#define UNICODE_CHAR_MULTIPLICATION_X                               0x2715
#define UNICODE_CHAR_SQUARE_WITH_BOTTOM_HALF_BLACK                  0x2B13

// Arrow key labels
//
CHAR16 mUpArrowKeyLabel[]       = { ARROW_UP,    0 };
CHAR16 mDownArrowKeyLabel[]     = { ARROW_DOWN,  0 };
CHAR16 mRightArrowKeyLabel[]    = { ARROW_RIGHT, 0 };
CHAR16 mLeftArrowKeyLabel[]     = { ARROW_LEFT,  0 };

// Enter and Backspace key labels
//
CHAR16 mBackSpaceKeyLabel[]     = { UNICODE_CHAR_ERASE_TO_THE_LEFT, 0 };
CHAR16 mEnterKeyLabel[]         = { UNICODE_CHAR_RETURN_SYMBOL,     0 };

// Keyboard Dock/Undock and Close "button" labels
//
CHAR16 mDockButtonLabel[]       = { UNICODE_CHAR_SQUARE_WITH_BOTTOM_HALF_BLACK,              0 };
CHAR16 mUndockButtonLabel[]     = { UNICODE_CHAR_WHITE_SQUARE_CONTAINING_BLACK_SMALL_SQUARE, 0 };
CHAR16 mCloseButtonLabel[]      = { UNICODE_CHAR_MULTIPLICATION_X,                           0 };


// [US-EN: Standard]
//
// Standard (i.e., no key modifier text mode) US-English OSK keyboard mapping table
//
OSK_KEY_MAPPING mOSK_StdMode_US_EN[NUMBER_OF_KEYS] =
{
//    EFI Key           Unicode Value           Scan Code   Key Label               Label W x H
// --------------------------------------------------------------------------------------------
    { EfiKeyD1,         'q',                    SCAN_NULL,  L"q",                   0, 0 },
    { EfiKeyD2,         'w',                    SCAN_NULL,  L"w",                   0, 0 },
    { EfiKeyD3,         'e',                    SCAN_NULL,  L"e",                   0, 0 },
    { EfiKeyD4,         'r',                    SCAN_NULL,  L"r",                   0, 0 },
    { EfiKeyD5,         't',                    SCAN_NULL,  L"t",                   0, 0 },
    { EfiKeyD6,         'y',                    SCAN_NULL,  L"y",                   0, 0 },
    { EfiKeyD7,         'u',                    SCAN_NULL,  L"u",                   0, 0 },
    { EfiKeyD8,         'i',                    SCAN_NULL,  L"i",                   0, 0 },
    { EfiKeyD9,         'o',                    SCAN_NULL,  L"o",                   0, 0 },
    { EfiKeyD10,        'p',                    SCAN_NULL,  L"p",                   0, 0 },
    { EfiKeyBackSpace,  CHAR_BACKSPACE,         SCAN_NULL,  mBackSpaceKeyLabel,     0, 0 },
    { EfiKeyC1,         'a',                    SCAN_NULL,  L"a",                   0, 0 },
    { EfiKeyC2,         's',                    SCAN_NULL,  L"s",                   0, 0 },
    { EfiKeyC3,         'd',                    SCAN_NULL,  L"d",                   0, 0 },
    { EfiKeyC4,         'f',                    SCAN_NULL,  L"f",                   0, 0 },
    { EfiKeyC5,         'g',                    SCAN_NULL,  L"g",                   0, 0 },
    { EfiKeyC6,         'h',                    SCAN_NULL,  L"h",                   0, 0 },
    { EfiKeyC7,         'j',                    SCAN_NULL,  L"j",                   0, 0 },
    { EfiKeyC8,         'k',                    SCAN_NULL,  L"k",                   0, 0 },
    { EfiKeyC9,         'l',                    SCAN_NULL,  L"l",                   0, 0 },
    { EfiKeyC11,        '\'',                   SCAN_NULL,  L"'",                   0, 0 },
    { EfiKeyEnter,      CHAR_CARRIAGE_RETURN,   SCAN_NULL,  mEnterKeyLabel,         0, 0 },
    { EfiKeyLShift,     CHAR_NULL,              SCAN_NULL,  L"^",                   0, 0 },
    { EfiKeyB1,         'z',                    SCAN_NULL,  L"z",                   0, 0 },
    { EfiKeyB2,         'x',                    SCAN_NULL,  L"x",                   0, 0 },
    { EfiKeyB3,         'c',                    SCAN_NULL,  L"c",                   0, 0 },
    { EfiKeyB4,         'v',                    SCAN_NULL,  L"v",                   0, 0 },
    { EfiKeyB5,         'b',                    SCAN_NULL,  L"b",                   0, 0 },
    { EfiKeyB6,         'n',                    SCAN_NULL,  L"n",                   0, 0 },
    { EfiKeyB7,         'm',                    SCAN_NULL,  L"m",                   0, 0 },
    { EfiKeyB8,         ',',                    SCAN_NULL,  L",",                   0, 0 },
    { EfiKeyB9,         '.',                    SCAN_NULL,  L".",                   0, 0 },
    { EfiKeyB10,        '?',                    SCAN_NULL,  L"?",                   0, 0 },
    { EfiKeyRShift,     CHAR_NULL,              SCAN_NULL,  L"^",                   0, 0 },
    { EfiKeyA0,         CHAR_NULL,              SCAN_NULL,  L"&123",                0, 0 },
    { EfiKeyA2,         CHAR_NULL,              SCAN_NULL,  L"Fn",                  0, 0 },
    { EfiKeyUpArrow,    CHAR_NULL,              SCAN_UP,    mUpArrowKeyLabel,       0, 0 },
    { EfiKeySpaceBar,   ' ',                    SCAN_NULL,  L"",                    0, 0 },
    { EfiKeyDownArrow,  CHAR_NULL,              SCAN_DOWN,  mDownArrowKeyLabel,     0, 0 },
    { EfiKeyLeftArrow,  CHAR_NULL,              SCAN_LEFT,  mLeftArrowKeyLabel,     0, 0 },
    { EfiKeyRightArrow, CHAR_NULL,              SCAN_RIGHT, mRightArrowKeyLabel,    0, 0 }
};


// [US-EN: Shift Mode]
//
// Shift-Only Mode US-English OSK keyboard mapping table
//
OSK_KEY_MAPPING mOSK_ShiftMode_US_EN[NUMBER_OF_KEYS] =
{
//    EFI Key           Unicode Value           Scan Code   Key Label               Label W x H
// --------------------------------------------------------------------------------------------
    { EfiKeyD1,         'Q',                    SCAN_NULL,  L"Q",                   0, 0 },
    { EfiKeyD2,         'W',                    SCAN_NULL,  L"W",                   0, 0 },
    { EfiKeyD3,         'E',                    SCAN_NULL,  L"E",                   0, 0 },
    { EfiKeyD4,         'R',                    SCAN_NULL,  L"R",                   0, 0 },
    { EfiKeyD5,         'T',                    SCAN_NULL,  L"T",                   0, 0 },
    { EfiKeyD6,         'Y',                    SCAN_NULL,  L"Y",                   0, 0 },
    { EfiKeyD7,         'U',                    SCAN_NULL,  L"U",                   0, 0 },
    { EfiKeyD8,         'I',                    SCAN_NULL,  L"I",                   0, 0 },
    { EfiKeyD9,         'O',                    SCAN_NULL,  L"O",                   0, 0 },
    { EfiKeyD10,        'P',                    SCAN_NULL,  L"P",                   0, 0 },
    { EfiKeyBackSpace,  CHAR_BACKSPACE,         SCAN_NULL,  mBackSpaceKeyLabel,     0, 0 },
    { EfiKeyC1,         'A',                    SCAN_NULL,  L"A",                   0, 0 },
    { EfiKeyC2,         'S',                    SCAN_NULL,  L"S",                   0, 0 },
    { EfiKeyC3,         'D',                    SCAN_NULL,  L"D",                   0, 0 },
    { EfiKeyC4,         'F',                    SCAN_NULL,  L"F",                   0, 0 },
    { EfiKeyC5,         'G',                    SCAN_NULL,  L"G",                   0, 0 },
    { EfiKeyC6,         'H',                    SCAN_NULL,  L"H",                   0, 0 },
    { EfiKeyC7,         'J',                    SCAN_NULL,  L"J",                   0, 0 },
    { EfiKeyC8,         'K',                    SCAN_NULL,  L"K",                   0, 0 },
    { EfiKeyC9,         'L',                    SCAN_NULL,  L"L",                   0, 0 },
    { EfiKeyC11,        '\'',                   SCAN_NULL,  L"'",                   0, 0 },
    { EfiKeyEnter,      CHAR_CARRIAGE_RETURN,   SCAN_NULL,  mEnterKeyLabel,         0, 0 },
    { EfiKeyLShift,     CHAR_NULL,              SCAN_NULL,  L"^",                   0, 0 },
    { EfiKeyB1,         'Z',                    SCAN_NULL,  L"Z",                   0, 0 },
    { EfiKeyB2,         'X',                    SCAN_NULL,  L"X",                   0, 0 },
    { EfiKeyB3,         'C',                    SCAN_NULL,  L"C",                   0, 0 },
    { EfiKeyB4,         'V',                    SCAN_NULL,  L"V",                   0, 0 },
    { EfiKeyB5,         'B',                    SCAN_NULL,  L"B",                   0, 0 },
    { EfiKeyB6,         'N',                    SCAN_NULL,  L"N",                   0, 0 },
    { EfiKeyB7,         'M',                    SCAN_NULL,  L"M",                   0, 0 },
    { EfiKeyB8,         ';',                    SCAN_NULL,  L";",                   0, 0 },
    { EfiKeyB9,         ':',                    SCAN_NULL,  L":",                   0, 0 },
    { EfiKeyB10,        '!',                    SCAN_NULL,  L"!",                   0, 0 },
    { EfiKeyRShift,     CHAR_NULL,              SCAN_NULL,  L"^",                   0, 0 },
    { EfiKeyA0,         CHAR_NULL,              SCAN_NULL,  L"&123",                0, 0 },
    { EfiKeyA2,         CHAR_NULL,              SCAN_NULL,  L"Fn",                  0, 0 },
    { EfiKeyUpArrow,    CHAR_NULL,              SCAN_UP,    mUpArrowKeyLabel,       0, 0 },
    { EfiKeySpaceBar,   ' ',                    SCAN_NULL,  L"",                    0, 0 },
    { EfiKeyDownArrow,  CHAR_NULL,              SCAN_DOWN,  mDownArrowKeyLabel,     0, 0 },
    { EfiKeyLeftArrow,  CHAR_NULL,              SCAN_LEFT,  mLeftArrowKeyLabel,     0, 0 },
    { EfiKeyRightArrow, CHAR_NULL,              SCAN_RIGHT, mRightArrowKeyLabel,    0, 0 }
};


// [US-EN: Numeric & Symbols Mode]
//
// NumericSymbols-Only Mode US-English OSK keyboard mapping table
//
//
OSK_KEY_MAPPING mOSK_NumSymMode_US_EN[NUMBER_OF_KEYS] =
{
//    EFI Key           Unicode Value           Scan Code   Key Label               Label W x H
// --------------------------------------------------------------------------------------------
    { EfiKeyD1,         '!',                    SCAN_NULL,  L"!",                   0, 0 },
    { EfiKeyD2,         '@',                    SCAN_NULL,  L"@",                   0, 0 },
    { EfiKeyD3,         '#',                    SCAN_NULL,  L"#",                   0, 0 },
    { EfiKeyD4,         '$',                    SCAN_NULL,  L"$",                   0, 0 },
    { EfiKeyD5,         '%',                    SCAN_NULL,  L"%",                   0, 0 },
    { EfiKeyD6,         '^',                    SCAN_NULL,  L"^",                   0, 0 },
    { EfiKeyD7,         '&',                    SCAN_NULL,  L"&",                   0, 0 },
    { EfiKeyD8,         '*',                    SCAN_NULL,  L"*",                   0, 0 },
    { EfiKeyD9,         '(',                    SCAN_NULL,  L"(",                   0, 0 },
    { EfiKeyD10,        ')',                    SCAN_NULL,  L")",                   0, 0 },
    { EfiKeyBackSpace,  CHAR_BACKSPACE,         SCAN_NULL,  mBackSpaceKeyLabel,     0, 0 },
    { EfiKeyC1,         '1',                    SCAN_NULL,  L"1",                   0, 0 },
    { EfiKeyC2,         '2',                    SCAN_NULL,  L"2",                   0, 0 },
    { EfiKeyC3,         '3',                    SCAN_NULL,  L"3",                   0, 0 },
    { EfiKeyC4,         '4',                    SCAN_NULL,  L"4",                   0, 0 },
    { EfiKeyC5,         '5',                    SCAN_NULL,  L"5",                   0, 0 },
    { EfiKeyC6,         '6',                    SCAN_NULL,  L"6",                   0, 0 },
    { EfiKeyC7,         '7',                    SCAN_NULL,  L"7",                   0, 0 },
    { EfiKeyC8,         '8',                    SCAN_NULL,  L"8",                   0, 0 },
    { EfiKeyC9,         '9',                    SCAN_NULL,  L"9",                   0, 0 },
    { EfiKeyC11,        '0',                    SCAN_NULL,  L"0",                   0, 0 },
    { EfiKeyEnter,      CHAR_CARRIAGE_RETURN,   SCAN_NULL,  mEnterKeyLabel,         0, 0 },
    { EfiKeyLShift,     CHAR_NULL,              SCAN_NULL,  L"^",                   0, 0 },
    { EfiKeyB1,         '_',                    SCAN_NULL,  L"_",                   0, 0 },
    { EfiKeyB2,         '-',                    SCAN_NULL,  L"-",                   0, 0 },
    { EfiKeyB3,         '+',                    SCAN_NULL,  L"+",                   0, 0 },
    { EfiKeyB4,         '=',                    SCAN_NULL,  L"=",                   0, 0 },
    { EfiKeyB5,         '/',                    SCAN_NULL,  L"/",                   0, 0 },
    { EfiKeyB6,         '\\',                   SCAN_NULL,  L"\\",                  0, 0 },
    { EfiKeyB7,         '~',                    SCAN_NULL,  L"~",                   0, 0 },
    { EfiKeyB8,         ',',                    SCAN_NULL,  L",",                   0, 0 },
    { EfiKeyB9,         '.',                    SCAN_NULL,  L".",                   0, 0 },
    { EfiKeyB10,        '?',                    SCAN_NULL,  L"?",                   0, 0 },
    { EfiKeyRShift,     CHAR_NULL,              SCAN_NULL,  L"^",                   0, 0 },
    { EfiKeyA0,         CHAR_NULL,              SCAN_NULL,  L"&123",                0, 0 },
    { EfiKeyA2,         CHAR_NULL,              SCAN_NULL,  L"Fn",                  0, 0 },
    { EfiKeyUpArrow,    CHAR_NULL,              SCAN_UP,    mUpArrowKeyLabel,       0, 0 },
    { EfiKeySpaceBar,   ' ',                    SCAN_NULL,  L"",                    0, 0 },
    { EfiKeyDownArrow,  CHAR_NULL,              SCAN_DOWN,  mDownArrowKeyLabel,     0, 0 },
    { EfiKeyLeftArrow,  CHAR_NULL,              SCAN_LEFT,  mLeftArrowKeyLabel,     0, 0 },
    { EfiKeyRightArrow, CHAR_NULL,              SCAN_RIGHT, mRightArrowKeyLabel,    0, 0 }
};


// [US-EN: Function & Special Mode]
//
// Function & Special Key Mode US-English OSK keyboard mapping table
//
//
OSK_KEY_MAPPING mOSK_FnctMode_US_EN[NUMBER_OF_KEYS] =
{
//    EFI Key           Unicode Value           Scan Code       Key Label               Label W x H
// ------------------------------------------------------------------------------------------------
    { EfiKeyD1,         CHAR_NULL,              SCAN_ESC,       L"Esc",                 0, 0 },
    { EfiKeyD2,         CHAR_NULL,              SCAN_HOME,      L"Home",                0, 0 },
    { EfiKeyD3,         CHAR_NULL,              SCAN_END,       L"End",                 0, 0 },
    { EfiKeyD4,         CHAR_NULL,              SCAN_INSERT,    L"Ins",                 0, 0 },
    { EfiKeyD5,         CHAR_NULL,              SCAN_DELETE,    L"Del",                 0, 0 },
    { EfiKeyD6,         CHAR_NULL,              SCAN_PAGE_UP,   L"PgUp",                0, 0 },
    { EfiKeyD7,         CHAR_NULL,              SCAN_PAGE_DOWN, L"PgDn",                0, 0 },
    { EfiKeyD8,         CHAR_NULL,              SCAN_NULL,      L"",                    0, 0 },     // Unused
    { EfiKeyD9,         CHAR_NULL,              SCAN_NULL,      L"",                    0, 0 },     // Unused
    { EfiKeyD10,        CHAR_NULL,              SCAN_NULL,      L"",                    0, 0 },     // Unused
    { EfiKeyBackSpace,  CHAR_BACKSPACE,         SCAN_NULL,      mBackSpaceKeyLabel,     0, 0 },
    { EfiKeyC1,         CHAR_NULL,              SCAN_F1,        L"F1",                  0, 0 },
    { EfiKeyC2,         CHAR_NULL,              SCAN_F2,        L"F2",                  0, 0 },
    { EfiKeyC3,         CHAR_NULL,              SCAN_F3,        L"F3",                  0, 0 },
    { EfiKeyC4,         CHAR_NULL,              SCAN_F4,        L"F4",                  0, 0 },
    { EfiKeyC5,         CHAR_NULL,              SCAN_F5,        L"F5",                  0, 0 },
    { EfiKeyC6,         CHAR_NULL,              SCAN_F6,        L"F6",                  0, 0 },
    { EfiKeyC7,         CHAR_NULL,              SCAN_F7,        L"F7",                  0, 0 },
    { EfiKeyC8,         CHAR_NULL,              SCAN_F8,        L"F8",                  0, 0 },
    { EfiKeyC9,         CHAR_NULL,              SCAN_F9,        L"F9",                  0, 0 },
    { EfiKeyC11,        CHAR_NULL,              SCAN_F10,       L"F10",                 0, 0 },
    { EfiKeyEnter,      CHAR_CARRIAGE_RETURN,   SCAN_NULL,      mEnterKeyLabel,         0, 0 },
    { EfiKeyLShift,     CHAR_NULL,              SCAN_NULL,      L"^",                   0, 0 },
    { EfiKeyB1,         CHAR_NULL,              SCAN_F11,       L"F11",                 0, 0 },
    { EfiKeyB2,         CHAR_NULL,              SCAN_F12,       L"F12",                 0, 0 },
    { EfiKeyB3,         CHAR_NULL,              SCAN_NULL,      L"",                    0, 0 },     // Unused
    { EfiKeyB4,         CHAR_NULL,              SCAN_NULL,      L"",                    0, 0 },     // Unused
    { EfiKeyB5,         CHAR_NULL,              SCAN_NULL,      L"",                    0, 0 },     // Unused
    { EfiKeyB6,         CHAR_NULL,              SCAN_NULL,      L"",                    0, 0 },     // Unused
    { EfiKeyB7,         CHAR_NULL,              SCAN_NULL,      L"",                    0, 0 },     // Unused
    { EfiKeyB8,         CHAR_NULL,              SCAN_NULL,      L"",                    0, 0 },     // Unused
    { EfiKeyB9,         CHAR_NULL,              SCAN_NULL,      L"",                    0, 0 },     // Unused
    { EfiKeyB10,        CHAR_NULL,              SCAN_NULL,      L"",                    0, 0 },     // Unused
    { EfiKeyRShift,     CHAR_NULL,              SCAN_NULL,      L"^",                   0, 0 },
    { EfiKeyA0,         CHAR_NULL,              SCAN_NULL,      L"&123",                0, 0 },
    { EfiKeyA2,         CHAR_NULL,              SCAN_NULL,      L"Fn",                  0, 0 },
    { EfiKeyUpArrow,    CHAR_NULL,              SCAN_UP,        mUpArrowKeyLabel,       0, 0 },
    { EfiKeySpaceBar,   ' ',                    SCAN_NULL,      L"",                    0, 0 },
    { EfiKeyDownArrow,  CHAR_NULL,              SCAN_DOWN,      mDownArrowKeyLabel,     0, 0 },
    { EfiKeyLeftArrow,  CHAR_NULL,              SCAN_LEFT,      mLeftArrowKeyLabel,     0, 0 },
    { EfiKeyRightArrow, CHAR_NULL,              SCAN_RIGHT,     mRightArrowKeyLabel,    0, 0 }
};


#endif  // _OSK_KEY_MAPPING_H_
