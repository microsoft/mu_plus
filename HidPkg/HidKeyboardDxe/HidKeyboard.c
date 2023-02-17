/** @file HidKeyboard.c

Copyright (C) Microsoft Corporation. All rights reserved.

Portions derived from UsbKbDxe:
Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  HID Keyboard Driver that manages HID keyboard and produces
  Simple Text Input Protocol and Simple Text Input Ex Protocol.

**/

#include "HidKeyboard.h"

HID_KEYBOARD_LAYOUT_PACK_BIN  mHidKeyboardLayoutBin = {
  sizeof (HID_KEYBOARD_LAYOUT_PACK_BIN),   // Binary size

  //
  // EFI_HII_PACKAGE_HEADER
  //
  {
    sizeof (HID_KEYBOARD_LAYOUT_PACK_BIN) - sizeof (UINT32),
    EFI_HII_PACKAGE_KEYBOARD_LAYOUT
  },
  1,                                                                                                                               // LayoutCount
  sizeof (HID_KEYBOARD_LAYOUT_PACK_BIN) - sizeof (UINT32) - sizeof (EFI_HII_PACKAGE_HEADER) - sizeof (UINT16),                     // LayoutLength
  HID_KEYBOARD_LAYOUT_KEY_GUID,                                                                                                    // KeyGuid
  sizeof (UINT16) + sizeof (EFI_GUID) + sizeof (UINT32) + sizeof (UINT8) + (HID_KEYBOARD_KEY_COUNT * sizeof (EFI_KEY_DESCRIPTOR)), // LayoutDescriptorStringOffset
  HID_KEYBOARD_KEY_COUNT,                                                                                                          // DescriptorCount
  {
    //
    // EFI_KEY_DESCRIPTOR (total number is HID_KEYBOARD_KEY_COUNT)
    //
    { EfiKeyC1,         'a',  'A',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyB5,         'b',  'B',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyB3,         'c',  'C',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyC3,         'd',  'D',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD3,         'e',  'E',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyC4,         'f',  'F',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyC5,         'g',  'G',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyC6,         'h',  'H',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD8,         'i',  'I',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyC7,         'j',  'J',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyC8,         'k',  'K',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyC9,         'l',  'L',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyB7,         'm',  'M',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyB6,         'n',  'N',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD9,         'o',  'O',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD10,        'p',  'P',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD1,         'q',  'Q',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD4,         'r',  'R',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyC2,         's',  'S',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD5,         't',  'T',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD7,         'u',  'U',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyB4,         'v',  'V',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD2,         'w',  'W',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyB2,         'x',  'X',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD6,         'y',  'Y',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyB1,         'z',  'Z',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyE1,         '1',  '!',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE2,         '2',  '@',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE3,         '3',  '#',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE4,         '4',  '$',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE5,         '5',  '%',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE6,         '6',  '^',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE7,         '7',  '&',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE8,         '8',  '*',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE9,         '9',  '(',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE10,        '0',  ')',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyEnter,      0x0d, 0x0d, 0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyEsc,        0x1b, 0x1b, 0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyBackSpace,  0x08, 0x08, 0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyTab,        0x09, 0x09, 0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeySpaceBar,   ' ',  ' ',  0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyE11,        '-',  '_',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE12,        '=',  '+',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyD11,        '[',  '{',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyD12,        ']',  '}',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyD13,        '\\', '|',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyC12,        '\\', '|',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyC10,        ';',  ':',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyC11,        '\'', '"',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE0,         '`',  '~',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyB8,         ',',  '<',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyB9,         '.',  '>',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyB10,        '/',  '?',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyCapsLock,   0x00, 0x00, 0,   0,   EFI_CAPS_LOCK_MODIFIER,           0                                                          },
    { EfiKeyF1,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_ONE_MODIFIER,    0                                                          },
    { EfiKeyF2,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_TWO_MODIFIER,    0                                                          },
    { EfiKeyF3,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_THREE_MODIFIER,  0                                                          },
    { EfiKeyF4,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_FOUR_MODIFIER,   0                                                          },
    { EfiKeyF5,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_FIVE_MODIFIER,   0                                                          },
    { EfiKeyF6,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_SIX_MODIFIER,    0                                                          },
    { EfiKeyF7,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_SEVEN_MODIFIER,  0                                                          },
    { EfiKeyF8,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_EIGHT_MODIFIER,  0                                                          },
    { EfiKeyF9,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_NINE_MODIFIER,   0                                                          },
    { EfiKeyF10,        0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_TEN_MODIFIER,    0                                                          },
    { EfiKeyF11,        0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_ELEVEN_MODIFIER, 0                                                          },
    { EfiKeyF12,        0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_TWELVE_MODIFIER, 0                                                          },
    { EfiKeyPrint,      0x00, 0x00, 0,   0,   EFI_PRINT_MODIFIER,               0                                                          },
    { EfiKeySLck,       0x00, 0x00, 0,   0,   EFI_SCROLL_LOCK_MODIFIER,         0                                                          },
    { EfiKeyPause,      0x00, 0x00, 0,   0,   EFI_PAUSE_MODIFIER,               0                                                          },
    { EfiKeyIns,        0x00, 0x00, 0,   0,   EFI_INSERT_MODIFIER,              0                                                          },
    { EfiKeyHome,       0x00, 0x00, 0,   0,   EFI_HOME_MODIFIER,                0                                                          },
    { EfiKeyPgUp,       0x00, 0x00, 0,   0,   EFI_PAGE_UP_MODIFIER,             0                                                          },
    { EfiKeyDel,        0x00, 0x00, 0,   0,   EFI_DELETE_MODIFIER,              0                                                          },
    { EfiKeyEnd,        0x00, 0x00, 0,   0,   EFI_END_MODIFIER,                 0                                                          },
    { EfiKeyPgDn,       0x00, 0x00, 0,   0,   EFI_PAGE_DOWN_MODIFIER,           0                                                          },
    { EfiKeyRightArrow, 0x00, 0x00, 0,   0,   EFI_RIGHT_ARROW_MODIFIER,         0                                                          },
    { EfiKeyLeftArrow,  0x00, 0x00, 0,   0,   EFI_LEFT_ARROW_MODIFIER,          0                                                          },
    { EfiKeyDownArrow,  0x00, 0x00, 0,   0,   EFI_DOWN_ARROW_MODIFIER,          0                                                          },
    { EfiKeyUpArrow,    0x00, 0x00, 0,   0,   EFI_UP_ARROW_MODIFIER,            0                                                          },
    { EfiKeyNLck,       0x00, 0x00, 0,   0,   EFI_NUM_LOCK_MODIFIER,            0                                                          },
    { EfiKeySlash,      '/',  '/',  0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyAsterisk,   '*',  '*',  0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyMinus,      '-',  '-',  0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyPlus,       '+',  '+',  0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyEnter,      0x0d, 0x0d, 0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyOne,        '1',  '1',  0,   0,   EFI_END_MODIFIER,                 EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyTwo,        '2',  '2',  0,   0,   EFI_DOWN_ARROW_MODIFIER,          EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyThree,      '3',  '3',  0,   0,   EFI_PAGE_DOWN_MODIFIER,           EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyFour,       '4',  '4',  0,   0,   EFI_LEFT_ARROW_MODIFIER,          EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyFive,       '5',  '5',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeySix,        '6',  '6',  0,   0,   EFI_RIGHT_ARROW_MODIFIER,         EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeySeven,      '7',  '7',  0,   0,   EFI_HOME_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyEight,      '8',  '8',  0,   0,   EFI_UP_ARROW_MODIFIER,            EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyNine,       '9',  '9',  0,   0,   EFI_PAGE_UP_MODIFIER,             EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyZero,       '0',  '0',  0,   0,   EFI_INSERT_MODIFIER,              EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyPeriod,     '.',  '.',  0,   0,   EFI_DELETE_MODIFIER,              EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyA4,         0x00, 0x00, 0,   0,   EFI_MENU_MODIFIER,                0                                                          },
    { EfiKeyLCtrl,      0,    0,    0,   0,   EFI_LEFT_CONTROL_MODIFIER,        0                                                          },
    { EfiKeyLShift,     0,    0,    0,   0,   EFI_LEFT_SHIFT_MODIFIER,          0                                                          },
    { EfiKeyLAlt,       0,    0,    0,   0,   EFI_LEFT_ALT_MODIFIER,            0                                                          },
    { EfiKeyA0,         0,    0,    0,   0,   EFI_LEFT_LOGO_MODIFIER,           0                                                          },
    { EfiKeyRCtrl,      0,    0,    0,   0,   EFI_RIGHT_CONTROL_MODIFIER,       0                                                          },
    { EfiKeyRShift,     0,    0,    0,   0,   EFI_RIGHT_SHIFT_MODIFIER,         0                                                          },
    { EfiKeyA2,         0,    0,    0,   0,   EFI_RIGHT_ALT_MODIFIER,           0                                                          },
    { EfiKeyA3,         0,    0,    0,   0,   EFI_RIGHT_LOGO_MODIFIER,          0                                                          },
  },
  1,                                                                                                                                        // DescriptionCount
  { 'e',              'n',  '-',  'U', 'S' },                                                                                               // RFC4646 language code
  ' ',                                                                                                                                      // Space
  { 'E',              'n',  'g',  'l', 'i', 's',                              'h', ' ', 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd', '\0'     }, // DescriptionString[]
};

//
// EFI_KEY to HID Keycode conversion table
// EFI_KEY is defined in UEFI spec.
// HID Keycode is defined in HID HID Firmware spec.
//
UINT8  EfiKeyToHidKeyCodeConvertionTable[] = {
  0xe0,  //  EfiKeyLCtrl
  0xe3,  //  EfiKeyA0
  0xe2,  //  EfiKeyLAlt
  0x2c,  //  EfiKeySpaceBar
  0xe6,  //  EfiKeyA2
  0xe7,  //  EfiKeyA3
  0x65,  //  EfiKeyA4
  0xe4,  //  EfiKeyRCtrl
  0x50,  //  EfiKeyLeftArrow
  0x51,  //  EfiKeyDownArrow
  0x4F,  //  EfiKeyRightArrow
  0x62,  //  EfiKeyZero
  0x63,  //  EfiKeyPeriod
  0x28,  //  EfiKeyEnter
  0xe1,  //  EfiKeyLShift
  0x64,  //  EfiKeyB0
  0x1D,  //  EfiKeyB1
  0x1B,  //  EfiKeyB2
  0x06,  //  EfiKeyB3
  0x19,  //  EfiKeyB4
  0x05,  //  EfiKeyB5
  0x11,  //  EfiKeyB6
  0x10,  //  EfiKeyB7
  0x36,  //  EfiKeyB8
  0x37,  //  EfiKeyB9
  0x38,  //  EfiKeyB10
  0xe5,  //  EfiKeyRShift
  0x52,  //  EfiKeyUpArrow
  0x59,  //  EfiKeyOne
  0x5A,  //  EfiKeyTwo
  0x5B,  //  EfiKeyThree
  0x39,  //  EfiKeyCapsLock
  0x04,  //  EfiKeyC1
  0x16,  //  EfiKeyC2
  0x07,  //  EfiKeyC3
  0x09,  //  EfiKeyC4
  0x0A,  //  EfiKeyC5
  0x0B,  //  EfiKeyC6
  0x0D,  //  EfiKeyC7
  0x0E,  //  EfiKeyC8
  0x0F,  //  EfiKeyC9
  0x33,  //  EfiKeyC10
  0x34,  //  EfiKeyC11
  0x32,  //  EfiKeyC12
  0x5C,  //  EfiKeyFour
  0x5D,  //  EfiKeyFive
  0x5E,  //  EfiKeySix
  0x57,  //  EfiKeyPlus
  0x2B,  //  EfiKeyTab
  0x14,  //  EfiKeyD1
  0x1A,  //  EfiKeyD2
  0x08,  //  EfiKeyD3
  0x15,  //  EfiKeyD4
  0x17,  //  EfiKeyD5
  0x1C,  //  EfiKeyD6
  0x18,  //  EfiKeyD7
  0x0C,  //  EfiKeyD8
  0x12,  //  EfiKeyD9
  0x13,  //  EfiKeyD10
  0x2F,  //  EfiKeyD11
  0x30,  //  EfiKeyD12
  0x31,  //  EfiKeyD13
  0x4C,  //  EfiKeyDel
  0x4D,  //  EfiKeyEnd
  0x4E,  //  EfiKeyPgDn
  0x5F,  //  EfiKeySeven
  0x60,  //  EfiKeyEight
  0x61,  //  EfiKeyNine
  0x35,  //  EfiKeyE0
  0x1E,  //  EfiKeyE1
  0x1F,  //  EfiKeyE2
  0x20,  //  EfiKeyE3
  0x21,  //  EfiKeyE4
  0x22,  //  EfiKeyE5
  0x23,  //  EfiKeyE6
  0x24,  //  EfiKeyE7
  0x25,  //  EfiKeyE8
  0x26,  //  EfiKeyE9
  0x27,  //  EfiKeyE10
  0x2D,  //  EfiKeyE11
  0x2E,  //  EfiKeyE12
  0x2A,  //  EfiKeyBackSpace
  0x49,  //  EfiKeyIns
  0x4A,  //  EfiKeyHome
  0x4B,  //  EfiKeyPgUp
  0x53,  //  EfiKeyNLck
  0x54,  //  EfiKeySlash
  0x55,  //  EfiKeyAsterisk
  0x56,  //  EfiKeyMinus
  0x29,  //  EfiKeyEsc
  0x3A,  //  EfiKeyF1
  0x3B,  //  EfiKeyF2
  0x3C,  //  EfiKeyF3
  0x3D,  //  EfiKeyF4
  0x3E,  //  EfiKeyF5
  0x3F,  //  EfiKeyF6
  0x40,  //  EfiKeyF7
  0x41,  //  EfiKeyF8
  0x42,  //  EfiKeyF9
  0x43,  //  EfiKeyF10
  0x44,  //  EfiKeyF11
  0x45,  //  EfiKeyF12
  0x46,  //  EfiKeyPrint
  0x47,  //  EfiKeySLck
  0x48   //  EfiKeyPause
};

//
// Keyboard modifier value to EFI Scan Code conversion table
// EFI Scan Code and the modifier values are defined in UEFI spec.
//
UINT8  ModifierValueToEfiScanCodeConvertionTable[] = {
  SCAN_NULL,       // EFI_NULL_MODIFIER
  SCAN_NULL,       // EFI_LEFT_CONTROL_MODIFIER
  SCAN_NULL,       // EFI_RIGHT_CONTROL_MODIFIER
  SCAN_NULL,       // EFI_LEFT_ALT_MODIFIER
  SCAN_NULL,       // EFI_RIGHT_ALT_MODIFIER
  SCAN_NULL,       // EFI_ALT_GR_MODIFIER
  SCAN_INSERT,     // EFI_INSERT_MODIFIER
  SCAN_DELETE,     // EFI_DELETE_MODIFIER
  SCAN_PAGE_DOWN,  // EFI_PAGE_DOWN_MODIFIER
  SCAN_PAGE_UP,    // EFI_PAGE_UP_MODIFIER
  SCAN_HOME,       // EFI_HOME_MODIFIER
  SCAN_END,        // EFI_END_MODIFIER
  SCAN_NULL,       // EFI_LEFT_SHIFT_MODIFIER
  SCAN_NULL,       // EFI_RIGHT_SHIFT_MODIFIER
  SCAN_NULL,       // EFI_CAPS_LOCK_MODIFIER
  SCAN_NULL,       // EFI_NUM_LOCK_MODIFIER
  SCAN_LEFT,       // EFI_LEFT_ARROW_MODIFIER
  SCAN_RIGHT,      // EFI_RIGHT_ARROW_MODIFIER
  SCAN_DOWN,       // EFI_DOWN_ARROW_MODIFIER
  SCAN_UP,         // EFI_UP_ARROW_MODIFIER
  SCAN_NULL,       // EFI_NS_KEY_MODIFIER
  SCAN_NULL,       // EFI_NS_KEY_DEPENDENCY_MODIFIER
  SCAN_F1,         // EFI_FUNCTION_KEY_ONE_MODIFIER
  SCAN_F2,         // EFI_FUNCTION_KEY_TWO_MODIFIER
  SCAN_F3,         // EFI_FUNCTION_KEY_THREE_MODIFIER
  SCAN_F4,         // EFI_FUNCTION_KEY_FOUR_MODIFIER
  SCAN_F5,         // EFI_FUNCTION_KEY_FIVE_MODIFIER
  SCAN_F6,         // EFI_FUNCTION_KEY_SIX_MODIFIER
  SCAN_F7,         // EFI_FUNCTION_KEY_SEVEN_MODIFIER
  SCAN_F8,         // EFI_FUNCTION_KEY_EIGHT_MODIFIER
  SCAN_F9,         // EFI_FUNCTION_KEY_NINE_MODIFIER
  SCAN_F10,        // EFI_FUNCTION_KEY_TEN_MODIFIER
  SCAN_F11,        // EFI_FUNCTION_KEY_ELEVEN_MODIFIER
  SCAN_F12,        // EFI_FUNCTION_KEY_TWELVE_MODIFIER
  //
  // For Partial Keystroke support
  //
  SCAN_NULL,       // EFI_PRINT_MODIFIER
  SCAN_NULL,       // EFI_SYS_REQUEST_MODIFIER
  SCAN_NULL,       // EFI_SCROLL_LOCK_MODIFIER
  SCAN_PAUSE,      // EFI_PAUSE_MODIFIER
  SCAN_NULL,       // EFI_BREAK_MODIFIER
  SCAN_NULL,       // EFI_LEFT_LOGO_MODIFIER
  SCAN_NULL,       // EFI_RIGHT_LOGO_MODIFER
  SCAN_NULL,       // EFI_MENU_MODIFER
};

/**
  Initialize Key Convention Table by using default keyboard layout.

  @param  HidKeyboardDevice    The HID_KB_DEV instance.

  @retval EFI_SUCCESS          The default keyboard layout was installed successfully
  @retval Others               Failure to install default keyboard layout.
**/
EFI_STATUS
InstallDefaultKeyboardLayout (
  IN OUT HID_KB_DEV  *HidKeyboardDevice
  )
{
  EFI_STATUS                 Status;
  EFI_HII_DATABASE_PROTOCOL  *HiiDatabase;
  EFI_HII_HANDLE             HiiHandle;

  //
  // Locate Hii database protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **)&HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install Keyboard Layout package to HII database
  //
  HiiHandle = HiiAddPackages (
                &gHidKeyboardLayoutPackageGuid,
                HidKeyboardDevice->ControllerHandle,
                &mHidKeyboardLayoutBin,
                NULL
                );
  if (HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Set current keyboard layout
  //
  Status = HiiDatabase->SetKeyboardLayout (HiiDatabase, &gHidKeyboardLayoutKeyGuid);

  return Status;
}

/**
  Get current keyboard layout from HII database.

  @return Pointer to HII Keyboard Layout.
          NULL means failure occurred while trying to get keyboard layout.

**/
EFI_HII_KEYBOARD_LAYOUT *
GetCurrentKeyboardLayout (
  VOID
  )
{
  EFI_STATUS                 Status;
  EFI_HII_DATABASE_PROTOCOL  *HiiDatabase;
  EFI_HII_KEYBOARD_LAYOUT    *KeyboardLayout;
  UINT16                     Length;

  //
  // Locate HII Database Protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **)&HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Get current keyboard layout from HII database
  //
  Length         = 0;
  KeyboardLayout = NULL;
  Status         = HiiDatabase->GetKeyboardLayout (
                                  HiiDatabase,
                                  NULL,
                                  &Length,
                                  KeyboardLayout
                                  );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    KeyboardLayout = AllocatePool (Length);
    if (KeyboardLayout == NULL) {
      ASSERT (KeyboardLayout != NULL);
      return NULL;
    }

    Status = HiiDatabase->GetKeyboardLayout (
                            HiiDatabase,
                            NULL,
                            &Length,
                            KeyboardLayout
                            );
    if (EFI_ERROR (Status)) {
      FreePool (KeyboardLayout);
      KeyboardLayout = NULL;
    }
  }

  return KeyboardLayout;
}

/**
  Find Key Descriptor in Key Convertion Table given its HID keycode.

  @param  HidKeyboardDevice   The HID_KB_DEV instance.
  @param  KeyCode             HID Keycode.

  @return The Key Descriptor in Key Convertion Table.
          NULL means not found.

**/
EFI_KEY_DESCRIPTOR *
GetKeyDescriptor (
  IN HID_KB_DEV  *HidKeyboardDevice,
  IN UINT8       KeyCode
  )
{
  UINT8  Index;

  //
  // Make sure KeyCode is in the range of [0x4, 0x65] or [0xe0, 0xe7]
  //
  if ((!HIDKBD_VALID_KEYCODE (KeyCode)) || ((KeyCode > 0x65) && (KeyCode < 0xe0)) || (KeyCode > 0xe7)) {
    return NULL;
  }

  //
  // Calculate the index of Key Descriptor in Key Convertion Table
  //
  if (KeyCode <= 0x65) {
    Index = (UINT8)(KeyCode - 4);
  } else {
    Index = (UINT8)(KeyCode - 0xe0 + NUMBER_OF_VALID_NON_MODIFIER_HID_KEYCODE);
  }

  return &HidKeyboardDevice->KeyConvertionTable[Index];
}

/**
  Find Non-Spacing key for given Key descriptor.

  @param  HidKeyboardDevice    The HID_KB_DEV instance.
  @param  KeyDescriptor        Key descriptor.

  @return The Non-Spacing key corresponding to KeyDescriptor
          NULL means not found.

**/
HID_NS_KEY *
FindHidNsKey (
  IN HID_KB_DEV          *HidKeyboardDevice,
  IN EFI_KEY_DESCRIPTOR  *KeyDescriptor
  )
{
  LIST_ENTRY  *Link;
  LIST_ENTRY  *NsKeyList;
  HID_NS_KEY  *HidNsKey;

  NsKeyList = &HidKeyboardDevice->NsKeyList;
  Link      = GetFirstNode (NsKeyList);
  while (!IsNull (NsKeyList, Link)) {
    HidNsKey = HID_NS_KEY_FORM_FROM_LINK (Link);

    if (HidNsKey->NsKey[0].Key == KeyDescriptor->Key) {
      return HidNsKey;
    }

    Link = GetNextNode (NsKeyList, Link);
  }

  return NULL;
}

/**
  Find physical key definition for a given key descriptor.

  For a specified non-spacing key, there are a list of physical
  keys following it. This function traverses the list of
  physical keys and tries to find the physical key matching
  the KeyDescriptor.

  @param  HidNsKey          The non-spacing key information.
  @param  KeyDescriptor     The key descriptor.

  @return The physical key definition.
          If no physical key is found, parameter KeyDescriptor is returned.

**/
EFI_KEY_DESCRIPTOR *
FindPhysicalKey (
  IN HID_NS_KEY          *HidNsKey,
  IN EFI_KEY_DESCRIPTOR  *KeyDescriptor
  )
{
  UINTN               Index;
  EFI_KEY_DESCRIPTOR  *PhysicalKey;

  PhysicalKey = &HidNsKey->NsKey[1];
  for (Index = 0; Index < HidNsKey->KeyCount; Index++) {
    if (KeyDescriptor->Key == PhysicalKey->Key) {
      return PhysicalKey;
    }

    PhysicalKey++;
  }

  //
  // No children definition matched, return original key
  //
  return KeyDescriptor;
}

/**
  The notification function for EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID.

  This function is registered to event of EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID
  group type, which will be triggered by EFI_HII_DATABASE_PROTOCOL.SetKeyboardLayout().
  It tries to get curent keyboard layout from HII database.

  @param  Event        Event being signaled.
  @param  Context      Points to HID_KB_DEV instance.

**/
VOID
EFIAPI
SetKeyboardLayoutEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  HID_KB_DEV               *HidKeyboardDevice;
  EFI_HII_KEYBOARD_LAYOUT  *KeyboardLayout;
  EFI_KEY_DESCRIPTOR       TempKey;
  EFI_KEY_DESCRIPTOR       *KeyDescriptor;
  EFI_KEY_DESCRIPTOR       *TableEntry;
  EFI_KEY_DESCRIPTOR       *NsKey;
  HID_NS_KEY               *HidNsKey;
  UINTN                    Index;
  UINTN                    Index2;
  UINTN                    KeyCount;
  UINT8                    KeyCode;

  HidKeyboardDevice = (HID_KB_DEV *)Context;
  if (HidKeyboardDevice->Signature != HID_KB_DEV_SIGNATURE) {
    return;
  }

  //
  // Try to get current keyboard layout from HII database
  //
  KeyboardLayout = GetCurrentKeyboardLayout ();
  if (KeyboardLayout == NULL) {
    return;
  }

  //
  // Re-allocate resource for KeyConvertionTable
  //
  ReleaseKeyboardLayoutResources (HidKeyboardDevice);
  HidKeyboardDevice->KeyConvertionTable = AllocateZeroPool ((NUMBER_OF_VALID_HID_KEYCODE)*sizeof (EFI_KEY_DESCRIPTOR));
  ASSERT (HidKeyboardDevice->KeyConvertionTable != NULL);

  //
  // Traverse the list of key descriptors following the header of EFI_HII_KEYBOARD_LAYOUT
  //
  KeyDescriptor = (EFI_KEY_DESCRIPTOR *)(((UINT8 *)KeyboardLayout) + sizeof (EFI_HII_KEYBOARD_LAYOUT));
  for (Index = 0; Index < KeyboardLayout->DescriptorCount; Index++) {
    //
    // Copy from HII keyboard layout package binary for alignment
    //
    CopyMem (&TempKey, KeyDescriptor, sizeof (EFI_KEY_DESCRIPTOR));

    //
    // Fill the key into KeyConvertionTable, whose index is calculated from HID keycode.
    //
    KeyCode    = EfiKeyToHidKeyCodeConvertionTable[(UINT8)(TempKey.Key)];
    TableEntry = GetKeyDescriptor (HidKeyboardDevice, KeyCode);
    if (TableEntry == NULL) {
      ReleaseKeyboardLayoutResources (HidKeyboardDevice);
      FreePool (KeyboardLayout);
      return;
    }

    CopyMem (TableEntry, KeyDescriptor, sizeof (EFI_KEY_DESCRIPTOR));

    //
    // For non-spacing key, create the list with a non-spacing key followed by physical keys.
    //
    if (TempKey.Modifier == EFI_NS_KEY_MODIFIER) {
      HidNsKey = AllocateZeroPool (sizeof (HID_NS_KEY));
      ASSERT (HidNsKey != NULL);

      //
      // Search for sequential children physical key definitions
      //
      KeyCount = 0;
      NsKey    = KeyDescriptor + 1;
      for (Index2 = (UINT8)Index + 1; Index2 < KeyboardLayout->DescriptorCount; Index2++) {
        CopyMem (&TempKey, NsKey, sizeof (EFI_KEY_DESCRIPTOR));
        if (TempKey.Modifier == EFI_NS_KEY_DEPENDENCY_MODIFIER) {
          KeyCount++;
        } else {
          break;
        }

        NsKey++;
      }

      HidNsKey->Signature = HID_NS_KEY_SIGNATURE;
      HidNsKey->KeyCount  = KeyCount;
      HidNsKey->NsKey     = AllocateCopyPool (
                              (KeyCount + 1) * sizeof (EFI_KEY_DESCRIPTOR),
                              KeyDescriptor
                              );
      InsertTailList (&HidKeyboardDevice->NsKeyList, &HidNsKey->Link);

      //
      // Skip over the child physical keys
      //
      Index         += KeyCount;
      KeyDescriptor += KeyCount;
    }

    KeyDescriptor++;
  }

  //
  // There are two EfiKeyEnter, duplicate its key descriptor
  //
  TableEntry    = GetKeyDescriptor (HidKeyboardDevice, 0x58);
  KeyDescriptor = GetKeyDescriptor (HidKeyboardDevice, 0x28);
  CopyMem (TableEntry, KeyDescriptor, sizeof (EFI_KEY_DESCRIPTOR));

  FreePool (KeyboardLayout);
}

/**
  Destroy resources for keyboard layout.

  @param  HidKeyboardDevice    The HID_KB_DEV instance.

**/
VOID
ReleaseKeyboardLayoutResources (
  IN OUT HID_KB_DEV  *HidKeyboardDevice
  )
{
  HID_NS_KEY  *HidNsKey;
  LIST_ENTRY  *Link;

  if (HidKeyboardDevice->KeyConvertionTable != NULL) {
    FreePool (HidKeyboardDevice->KeyConvertionTable);
  }

  HidKeyboardDevice->KeyConvertionTable = NULL;

  while (!IsListEmpty (&HidKeyboardDevice->NsKeyList)) {
    Link     = GetFirstNode (&HidKeyboardDevice->NsKeyList);
    HidNsKey = HID_NS_KEY_FORM_FROM_LINK (Link);
    RemoveEntryList (&HidNsKey->Link);

    FreePool (HidNsKey->NsKey);
    FreePool (HidNsKey);
  }
}

/**
  Initialize HID keyboard layout.

  This function initializes Key Convertion Table for the HID
  keyboard device. It first tries to retrieve layout from HII
  database. If failed and default layout is enabled, then it
  just uses the default layout.

  @param  HidKeyboardDevice      The HID_KB_DEV instance.

  @retval EFI_SUCCESS            Initialization succeeded.
  @retval EFI_NOT_READY          Keyboard layout cannot be retrieve from HII
                                 database, and default layout is disabled.
  @retval Other                  Fail to register event to EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID group.

**/
EFI_STATUS
InitKeyboardLayout (
  OUT HID_KB_DEV  *HidKeyboardDevice
  )
{
  EFI_HII_KEYBOARD_LAYOUT  *KeyboardLayout;
  EFI_STATUS               Status;

  HidKeyboardDevice->KeyConvertionTable = AllocateZeroPool ((NUMBER_OF_VALID_HID_KEYCODE)*sizeof (EFI_KEY_DESCRIPTOR));
  ASSERT (HidKeyboardDevice->KeyConvertionTable != NULL);

  InitializeListHead (&HidKeyboardDevice->NsKeyList);
  HidKeyboardDevice->CurrentNsKey        = NULL;
  HidKeyboardDevice->KeyboardLayoutEvent = NULL;

  //
  // Register event to EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID group,
  // which will be triggered by EFI_HII_DATABASE_PROTOCOL.SetKeyboardLayout().
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SetKeyboardLayoutEvent,
                  HidKeyboardDevice,
                  &gEfiHiiKeyBoardLayoutGuid,
                  &HidKeyboardDevice->KeyboardLayoutEvent
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  KeyboardLayout = GetCurrentKeyboardLayout ();
  if (KeyboardLayout != NULL) {
    //
    // If current keyboard layout is successfully retrieved from HII database,
    // force to initialize the keyboard layout.
    //
    gBS->SignalEvent (HidKeyboardDevice->KeyboardLayoutEvent);
  } else {
    if (FeaturePcdGet (PcdDisableDefaultKeyboardLayoutInHidKbDriver)) {
      //
      // If no keyboard layout can be retrieved from HII database, and default layout
      // is disabled, then return EFI_NOT_READY.
      //
      return EFI_NOT_READY;
    }

    //
    // If no keyboard layout can be retrieved from HII database, and default layout
    // is enabled, then load the default keyboard layout.
    //
    InstallDefaultKeyboardLayout (HidKeyboardDevice);
  }

  return EFI_SUCCESS;
}

/**
  Initialize HID keyboard device and all private data structures.

  @param  HidKeyboardDevice  The HID_KB_DEV instance.

  @retval EFI_SUCCESS        Initialization is successful.
  @retval EFI_DEVICE_ERROR   Keyboard initialization failed.

**/
EFI_STATUS
InitHidKeyboard (
  IN OUT HID_KB_DEV  *HidKeyboardDevice
  )
{
  InitQueue (&HidKeyboardDevice->HidKeyQueue, sizeof (HID_KEY));
  InitQueue (&HidKeyboardDevice->EfiKeyQueue, sizeof (EFI_KEY_DATA));
  InitQueue (&HidKeyboardDevice->EfiKeyQueueForNotify, sizeof (EFI_KEY_DATA));

  HidKeyboardDevice->CtrlOn    = FALSE;
  HidKeyboardDevice->AltOn     = FALSE;
  HidKeyboardDevice->ShiftOn   = FALSE;
  HidKeyboardDevice->NumLockOn = FALSE;
  HidKeyboardDevice->CapsOn    = FALSE;
  HidKeyboardDevice->ScrollOn  = FALSE;

  HidKeyboardDevice->LeftCtrlOn   = FALSE;
  HidKeyboardDevice->LeftAltOn    = FALSE;
  HidKeyboardDevice->LeftShiftOn  = FALSE;
  HidKeyboardDevice->LeftLogoOn   = FALSE;
  HidKeyboardDevice->RightCtrlOn  = FALSE;
  HidKeyboardDevice->RightAltOn   = FALSE;
  HidKeyboardDevice->RightShiftOn = FALSE;
  HidKeyboardDevice->RightLogoOn  = FALSE;
  HidKeyboardDevice->MenuKeyOn    = FALSE;
  HidKeyboardDevice->SysReqOn     = FALSE;

  HidKeyboardDevice->AltGrOn = FALSE;

  HidKeyboardDevice->CurrentNsKey = NULL;

  //
  // Sync the initial state of lights on keyboard.
  //
  SetKeyLED (HidKeyboardDevice);

  if (HidKeyboardDevice->LastReport != NULL) {
    FreePool (HidKeyboardDevice->LastReport);
  }

  HidKeyboardDevice->LastReport     = NULL;
  HidKeyboardDevice->LastReportSize = 0;

  //
  // Create event for repeat keys' generation.
  //
  if (HidKeyboardDevice->RepeatTimer != NULL) {
    gBS->CloseEvent (HidKeyboardDevice->RepeatTimer);
    HidKeyboardDevice->RepeatTimer = NULL;
  }

  gBS->CreateEvent (
         EVT_TIMER | EVT_NOTIFY_SIGNAL,
         TPL_CALLBACK,
         HidKeyboardRepeatHandler,
         HidKeyboardDevice,
         &HidKeyboardDevice->RepeatTimer
         );

  return EFI_SUCCESS;
}

/**
  Top-level function for handling key report form HID layer.

  @param  Interface - defines the format of the hid report.
  @param  *HidInputReportBuffer Pointer to the buffer containing
                                key strokes received.
  @param  HidInputReportBufferSize - gives the size of the input report buffer.
  @param  *Context              Context that we need which
                                should be the pointer to
                                HID_KB_DEV struct.

  @retval VOID                  None

**/
VOID
EFIAPI
HIDProcessKeyStrokesCallback (
  IN KEYBOARD_HID_INTERFACE  Interface,
  IN UINT8                   *HidInputReportBuffer,
  IN UINTN                   HidInputReportBufferSize,
  IN VOID                    *Context
  )
{
  EFI_STATUS    Status;
  HID_KB_DEV    *HidKeyboardDevice;
  HID_KEY       HIDKey;
  EFI_KEY_DATA  KeyData;

  if (Interface != BootKeyboard) {
    DEBUG ((DEBUG_ERROR, "[%a] - Unsupported HID report interface %d\n", __FUNCTION__, Interface));
    return;
  }

  HidKeyboardDevice = (HID_KB_DEV *)Context;

  if ((HidKeyboardDevice == NULL) || (HidInputReportBuffer == NULL)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Invalid input pointer.\n", __FUNCTION__));
    ASSERT ((HidKeyboardDevice != NULL) && (HidInputReportBuffer != NULL));
    return;
  }

  // Process the HID keystrokes and enqueue them for further processing.
  ProcessKeyStroke (HidInputReportBuffer, HidInputReportBufferSize, HidKeyboardDevice);

  while (!IsQueueEmpty (&HidKeyboardDevice->HidKeyQueue)) {
    //
    // Pops one key off.
    //
    Dequeue (&HidKeyboardDevice->HidKeyQueue, &HIDKey, sizeof (HID_KEY));

    // Now process modifiers
    Status = HIDProcessModifierKey (HidKeyboardDevice, &HIDKey);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a] - Error processing modifier key: %r\n", __FUNCTION__, Status));
    }

    if (HIDKey.Down) {
      if (HIDKeyCodeToEfiInputKey (HidKeyboardDevice, HIDKey.KeyCode, &KeyData) == EFI_SUCCESS) {
        Enqueue (&HidKeyboardDevice->EfiKeyQueue, &KeyData, sizeof (KeyData));
      }
    }
  }
}

/**
  Initial processing of the HID key report. Processes and queues individual keys
  in the key report.

  @param  *HidInputReportBuffer     - Pointer to the buffer containing HID key report.
  @param  HidInputReportBufferSize  - gives the size of the input report buffer.
  @param  *HidKeyboardDevice        - pointer to HID_KB_DEV struct.

  @retval VOID                  None

**/
VOID
ProcessKeyStroke (
  IN UINT8       *HidInputReportBuffer,
  IN UINTN       HidInputReportBufferSize,
  IN HID_KB_DEV  *HidKeyboardDevice
  )
{
  HID_KEY                    HIDKey;
  UINT8                      ModifierIndex;
  UINT8                      Mask;
  UINT8                      KeyCode;
  UINT8                      LastKeyCode;
  UINT8                      NewRepeatKey = 0;
  BOOLEAN                    KeyPress;
  BOOLEAN                    KeyRelease;
  EFI_KEY_DESCRIPTOR         *KeyDescriptor;
  KEYBOARD_HID_INPUT_BUFFER  *LastReport;
  UINTN                      LastReportKeyCount;
  KEYBOARD_HID_INPUT_BUFFER  *CurrentReport;
  UINTN                      CurrentReportKeyCount;

  if ((HidKeyboardDevice == NULL) || (HidInputReportBuffer == NULL)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Invalid input pointer.\n", __FUNCTION__));
    ASSERT ((HidKeyboardDevice != NULL) && (HidInputReportBuffer != NULL));
    return;
  }

  if (HidInputReportBufferSize < INPUT_REPORT_HEADER_SIZE) {
    DEBUG ((DEBUG_ERROR, "[%a] - HID input report buffer is too small to process.\n", __FUNCTION__));
    return;
  }

  // if Last response doesn't exist, create it.
  if (HidKeyboardDevice->LastReport == NULL) {
    HidKeyboardDevice->LastReportSize = INPUT_REPORT_HEADER_SIZE;
    HidKeyboardDevice->LastReport     = AllocateZeroPool (INPUT_REPORT_HEADER_SIZE);
    if (HidKeyboardDevice->LastReport == NULL) {
      DEBUG ((DEBUG_ERROR, "[%a] - Could not allocate empty last report.\n", __FUNCTION__));
      return;
    }
  }

  LastReport            = HidKeyboardDevice->LastReport;
  LastReportKeyCount    = HidKeyboardDevice->LastReportSize - INPUT_REPORT_HEADER_SIZE;
  CurrentReport         = (KEYBOARD_HID_INPUT_BUFFER *)HidInputReportBuffer;
  CurrentReportKeyCount = HidInputReportBufferSize - INPUT_REPORT_HEADER_SIZE;

  //
  // Handle modifier key's pressing or releasing situation.
  // According to USB HID Firmware spec, Byte 0 uses following map of Modifier keys:
  // Bit0: Left Control,  Keycode: 0xe0
  // Bit1: Left Shift,    Keycode: 0xe1
  // Bit2: Left Alt,      Keycode: 0xe2
  // Bit3: Left GUI,      Keycode: 0xe3
  // Bit4: Right Control, Keycode: 0xe4
  // Bit5: Right Shift,   Keycode: 0xe5
  // Bit6: Right Alt,     Keycode: 0xe6
  // Bit7: Right GUI,     Keycode: 0xe7
  //
  for (ModifierIndex = 0; ModifierIndex < 8; ModifierIndex++) {
    Mask = (UINT8)(1 << ModifierIndex);
    if ((CurrentReport->ModifierKeys & Mask) != (LastReport->ModifierKeys & Mask)) {
      //
      // If current modifier key is up, then CurModifierMap & Mask = 0;
      // otherwise it is a non-zero value.
      // Insert the changed modifier key into key buffer.
      //
      HIDKey.KeyCode = (UINT8)(0xe0 + ModifierIndex);
      HIDKey.Down    = (BOOLEAN)((CurrentReport->ModifierKeys & Mask) != 0);
      Enqueue (&HidKeyboardDevice->HidKeyQueue, &HIDKey, sizeof (HID_KEY));
    }
  }

  //
  // Handle normal key's releasing situation
  // Bytes 3 to n are for normal keycodes
  //
  KeyRelease = FALSE;
  for (LastKeyCode = 0; (UINTN)LastKeyCode < LastReportKeyCount; LastKeyCode++) {
    if (!HIDKBD_VALID_KEYCODE (LastReport->KeyCode[LastKeyCode])) {
      continue;
    }

    //
    // For any key in old keycode buffer, if it is not in current keycode buffer,
    // then it is released. Otherwise, it is not released.
    //
    KeyRelease = TRUE;
    for (KeyCode = 0; (UINTN)KeyCode < CurrentReportKeyCount; KeyCode++) {
      if (!HIDKBD_VALID_KEYCODE (CurrentReport->KeyCode[KeyCode])) {
        continue;
      }

      if (LastReport->KeyCode[LastKeyCode] == CurrentReport->KeyCode[KeyCode]) {
        KeyRelease = FALSE;
        break;
      }
    }

    if (KeyRelease) {
      HIDKey.KeyCode = LastReport->KeyCode[LastKeyCode];
      HIDKey.Down    = FALSE;
      //
      // The original repeat key is released.
      //
      if (LastReport->KeyCode[LastKeyCode] == HidKeyboardDevice->RepeatKey) {
        DEBUG ((DEBUG_VERBOSE, "HIDKeyboard: Resetting key repeat\n"));
        HidKeyboardDevice->RepeatKey = 0;
      }
    }
  }

  //
  // If original repeat key is released, cancel the repeat timer
  //
  if (HidKeyboardDevice->RepeatKey == 0) {
    DEBUG ((DEBUG_VERBOSE, "HIDKeyboard: Releasing Key Repeat Timer\n"));
    gBS->SetTimer (
           HidKeyboardDevice->RepeatTimer,
           TimerCancel,
           HIDKBD_REPEAT_RATE
           );
  }

  //
  // Handle normal key's pressing situation
  //
  KeyPress = FALSE;
  for (KeyCode = 0; (UINTN)KeyCode < CurrentReportKeyCount; KeyCode++) {
    if (!HIDKBD_VALID_KEYCODE (CurrentReport->KeyCode[KeyCode])) {
      continue;
    }

    //
    // For any key in current keycode buffer, if it is not in old keycode buffer,
    // then it is pressed. Otherwise, it is not pressed.
    //
    KeyPress = TRUE;
    for (LastKeyCode = 0; (UINTN)LastKeyCode < LastReportKeyCount; LastKeyCode++) {
      if (!HIDKBD_VALID_KEYCODE (LastReport->KeyCode[LastKeyCode])) {
        continue;
      }

      if (CurrentReport->KeyCode[KeyCode] == LastReport->KeyCode[LastKeyCode]) {
        KeyPress = FALSE;
        break;
      }
    }

    if (KeyPress) {
      HIDKey.KeyCode = CurrentReport->KeyCode[KeyCode];
      HIDKey.Down    = TRUE;
      DEBUG ((DEBUG_VERBOSE, "HIDKeyboard: Enqueuing Key = %d, on KeyPress\n", HIDKey.KeyCode));
      Enqueue (&HidKeyboardDevice->HidKeyQueue, &HIDKey, sizeof (HID_KEY));

      //
      // Handle repeat key
      //
      KeyDescriptor = GetKeyDescriptor (HidKeyboardDevice, CurrentReport->KeyCode[KeyCode]);
      if (KeyDescriptor == NULL) {
        return;
      }

      if ((KeyDescriptor->Modifier == EFI_NUM_LOCK_MODIFIER) || (KeyDescriptor->Modifier == EFI_CAPS_LOCK_MODIFIER)) {
        //
        // For NumLock or CapsLock pressed, there is no need to handle repeat key for them.
        //
        HidKeyboardDevice->RepeatKey = 0;
      } else {
        //
        // Prepare new repeat key, and clear the original one.
        //
        NewRepeatKey                 = CurrentReport->KeyCode[KeyCode];
        HidKeyboardDevice->RepeatKey = 0;
      }
    }
  }

  //
  // Copy the current report buffer as the last report buffer
  //
  FreePool (LastReport);
  HidKeyboardDevice->LastReport = AllocateCopyPool (HidInputReportBufferSize, HidInputReportBuffer);
  if (HidKeyboardDevice->LastReport == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - could not allocate last report buffer\n", __FUNCTION__));
    HidKeyboardDevice->LastReportSize = 0;
  } else {
    HidKeyboardDevice->LastReportSize = HidInputReportBufferSize;
  }

  //
  // If there is new key pressed, update the RepeatKey value, and set the
  // timer to repeat the delay timer
  //
  if (NewRepeatKey != 0) {
    //
    // Sets trigger time to "Repeat Delay Time",
    // to trigger the repeat timer when the key is hold long
    // enough time.
    //
    gBS->SetTimer (
           HidKeyboardDevice->RepeatTimer,
           TimerRelative,
           HIDKBD_REPEAT_DELAY
           );
    DEBUG ((DEBUG_VERBOSE, "HIDKeyboard: Setting Key repeat timer\n"));
    DEBUG ((DEBUG_VERBOSE, "HIDKeyboard: New Repeat Key = %d, on KeyPress\n", NewRepeatKey));
    HidKeyboardDevice->RepeatKey = NewRepeatKey;
  }
}

/**
  This function parses the Modifier Key Code and sets the
  appropriate flags for Key Stroke processing.

  This function parses the modifier keycode and updates state of
  modifier key in HID_KB_DEV instance, and returns status.

  @param  HidKeyboardDevice    The HID_KB_DEV instance.

  @retval EFI_SUCCESS          Keycode successfully parsed.
  @retval EFI_NOT_READY        Keyboard buffer is not ready for a valid keycode

**/
EFI_STATUS
HIDProcessModifierKey (
  IN HID_KB_DEV  *HidKeyboardDevice,
  IN HID_KEY     *HIDKey
  )
{
  EFI_KEY_DESCRIPTOR  *KeyDescriptor;

  KeyDescriptor = GetKeyDescriptor (HidKeyboardDevice, HIDKey->KeyCode);
  if (KeyDescriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!HIDKey->Down) {
    //
    // Key is released.
    //
    switch (KeyDescriptor->Modifier) {
      //
      // Ctrl release
      //
      case EFI_LEFT_CONTROL_MODIFIER:
        HidKeyboardDevice->LeftCtrlOn = FALSE;
        HidKeyboardDevice->CtrlOn     = FALSE;
        break;
      case EFI_RIGHT_CONTROL_MODIFIER:
        HidKeyboardDevice->RightCtrlOn = FALSE;
        HidKeyboardDevice->CtrlOn      = FALSE;
        break;

      //
      // Shift release
      //
      case EFI_LEFT_SHIFT_MODIFIER:
        HidKeyboardDevice->LeftShiftOn = FALSE;
        HidKeyboardDevice->ShiftOn     = FALSE;
        break;
      case EFI_RIGHT_SHIFT_MODIFIER:
        HidKeyboardDevice->RightShiftOn = FALSE;
        HidKeyboardDevice->ShiftOn      = FALSE;
        break;

      //
      // Alt release
      //
      case EFI_LEFT_ALT_MODIFIER:
        HidKeyboardDevice->LeftAltOn = FALSE;
        HidKeyboardDevice->AltOn     = FALSE;
        break;
      case EFI_RIGHT_ALT_MODIFIER:
        HidKeyboardDevice->RightAltOn = FALSE;
        HidKeyboardDevice->AltOn      = FALSE;
        break;

      //
      // Left Logo release
      //
      case EFI_LEFT_LOGO_MODIFIER:
        HidKeyboardDevice->LeftLogoOn = FALSE;
        break;

      //
      // Right Logo release
      //
      case EFI_RIGHT_LOGO_MODIFIER:
        HidKeyboardDevice->RightLogoOn = FALSE;
        break;

      //
      // Menu key release
      //
      case EFI_MENU_MODIFIER:
        HidKeyboardDevice->MenuKeyOn = FALSE;
        break;

      //
      // SysReq release
      //
      case EFI_PRINT_MODIFIER:
      case EFI_SYS_REQUEST_MODIFIER:
        HidKeyboardDevice->SysReqOn = FALSE;
        break;

      //
      // AltGr release
      //
      case EFI_ALT_GR_MODIFIER:
        HidKeyboardDevice->AltGrOn = FALSE;
        break;

      default:
        break;
    }
  }
  //
  // Analyzes key pressing situation
  //
  else if (HIDKey->Down) {
    switch (KeyDescriptor->Modifier) {
      //
      // Ctrl press
      //
      case EFI_LEFT_CONTROL_MODIFIER:
        HidKeyboardDevice->LeftCtrlOn = TRUE;
        HidKeyboardDevice->CtrlOn     = TRUE;
        break;
      case EFI_RIGHT_CONTROL_MODIFIER:
        HidKeyboardDevice->RightCtrlOn = TRUE;
        HidKeyboardDevice->CtrlOn      = TRUE;
        break;

      //
      // Shift press
      //
      case EFI_LEFT_SHIFT_MODIFIER:
        HidKeyboardDevice->LeftShiftOn = TRUE;
        HidKeyboardDevice->ShiftOn     = TRUE;
        break;
      case EFI_RIGHT_SHIFT_MODIFIER:
        HidKeyboardDevice->RightShiftOn = TRUE;
        HidKeyboardDevice->ShiftOn      = TRUE;
        break;

      //
      // Alt press
      //
      case EFI_LEFT_ALT_MODIFIER:
        HidKeyboardDevice->LeftAltOn = TRUE;
        HidKeyboardDevice->AltOn     = TRUE;
        break;
      case EFI_RIGHT_ALT_MODIFIER:
        HidKeyboardDevice->RightAltOn = TRUE;
        HidKeyboardDevice->AltOn      = TRUE;
        break;

      //
      // Left Logo press
      //
      case EFI_LEFT_LOGO_MODIFIER:
        HidKeyboardDevice->LeftLogoOn = TRUE;
        break;

      //
      // Right Logo press
      //
      case EFI_RIGHT_LOGO_MODIFIER:
        HidKeyboardDevice->RightLogoOn = TRUE;
        break;

      //
      // Menu key press
      //
      case EFI_MENU_MODIFIER:
        HidKeyboardDevice->MenuKeyOn = TRUE;
        break;

      //
      // SysReq press
      //
      case EFI_PRINT_MODIFIER:
      case EFI_SYS_REQUEST_MODIFIER:
        HidKeyboardDevice->SysReqOn = TRUE;
        break;

      //
      // AltGr press
      //
      case EFI_ALT_GR_MODIFIER:
        HidKeyboardDevice->AltGrOn = TRUE;
        break;

      case EFI_NUM_LOCK_MODIFIER:
        //
        // Toggle NumLock
        //
        HidKeyboardDevice->NumLockOn = (BOOLEAN)(!(HidKeyboardDevice->NumLockOn));
        SetKeyLED (HidKeyboardDevice);
        break;

      case EFI_CAPS_LOCK_MODIFIER:
        //
        // Toggle CapsLock
        //
        HidKeyboardDevice->CapsOn = (BOOLEAN)(!(HidKeyboardDevice->CapsOn));
        SetKeyLED (HidKeyboardDevice);
        break;

      case EFI_SCROLL_LOCK_MODIFIER:
        //
        // Toggle ScrollLock
        //
        HidKeyboardDevice->ScrollOn = (BOOLEAN)(!(HidKeyboardDevice->ScrollOn));
        SetKeyLED (HidKeyboardDevice);
        break;

      default:
        break;
    }
  }

  //
  // When encountering Ctrl + Alt + Del, then warm reset.
  //
  if (KeyDescriptor->Modifier == EFI_DELETE_MODIFIER) {
    if ((HidKeyboardDevice->CtrlOn) && (HidKeyboardDevice->AltOn)) {
      gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
    }
  }

  return EFI_SUCCESS;
}

/**
  Initialize the key state.

  @param  UsbKeyboardDevice     The USB_KB_DEV instance.
  @param  KeyState              A pointer to receive the key state information.
**/
VOID
InitializeKeyState (
  IN  HID_KB_DEV     *HidKeyboardDevice,
  OUT EFI_KEY_STATE  *KeyState
  )
{
  KeyState->KeyShiftState  = EFI_SHIFT_STATE_VALID;
  KeyState->KeyToggleState = EFI_TOGGLE_STATE_VALID;

  if (HidKeyboardDevice->LeftCtrlOn) {
    KeyState->KeyShiftState |= EFI_LEFT_CONTROL_PRESSED;
  }

  if (HidKeyboardDevice->RightCtrlOn) {
    KeyState->KeyShiftState |= EFI_RIGHT_CONTROL_PRESSED;
  }

  if (HidKeyboardDevice->LeftAltOn) {
    KeyState->KeyShiftState |= EFI_LEFT_ALT_PRESSED;
  }

  if (HidKeyboardDevice->RightAltOn) {
    KeyState->KeyShiftState |= EFI_RIGHT_ALT_PRESSED;
  }

  if (HidKeyboardDevice->LeftShiftOn) {
    KeyState->KeyShiftState |= EFI_LEFT_SHIFT_PRESSED;
  }

  if (HidKeyboardDevice->RightShiftOn) {
    KeyState->KeyShiftState |= EFI_RIGHT_SHIFT_PRESSED;
  }

  if (HidKeyboardDevice->LeftLogoOn) {
    KeyState->KeyShiftState |= EFI_LEFT_LOGO_PRESSED;
  }

  if (HidKeyboardDevice->RightLogoOn) {
    KeyState->KeyShiftState |= EFI_RIGHT_LOGO_PRESSED;
  }

  if (HidKeyboardDevice->MenuKeyOn) {
    KeyState->KeyShiftState |= EFI_MENU_KEY_PRESSED;
  }

  if (HidKeyboardDevice->SysReqOn) {
    KeyState->KeyShiftState |= EFI_SYS_REQ_PRESSED;
  }

  if (HidKeyboardDevice->ScrollOn) {
    KeyState->KeyToggleState |= EFI_SCROLL_LOCK_ACTIVE;
  }

  if (HidKeyboardDevice->NumLockOn) {
    KeyState->KeyToggleState |= EFI_NUM_LOCK_ACTIVE;
  }

  if (HidKeyboardDevice->CapsOn) {
    KeyState->KeyToggleState |= EFI_CAPS_LOCK_ACTIVE;
  }

  if (HidKeyboardDevice->IsSupportPartialKey) {
    KeyState->KeyToggleState |= EFI_KEY_STATE_EXPOSED;
  }
}

/**
  Converts HID Keycode ranging from 0x4 to 0x65 to EFI_INPUT_KEY.

  @param  HidKeyboardDevice     The HID_KB_DEV instance.
  @param  KeyCode               Indicates the key code that will be interpreted.
  @param  KeyData               A pointer to a buffer that is filled in with
                                the keystroke information for the key that
                                was pressed.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER KeyCode is not in the range of 0x4 to 0x65.
  @retval EFI_INVALID_PARAMETER Translated EFI_INPUT_KEY has zero for both ScanCode and UnicodeChar.
  @retval EFI_NOT_READY         KeyCode represents a dead key with EFI_NS_KEY_MODIFIER
  @retval EFI_DEVICE_ERROR      Keyboard layout is invalid.

**/
EFI_STATUS
HIDKeyCodeToEfiInputKey (
  IN  HID_KB_DEV    *HidKeyboardDevice,
  IN  UINT8         KeyCode,
  OUT EFI_KEY_DATA  *KeyData
  )
{
  EFI_KEY_DESCRIPTOR             *KeyDescriptor;
  LIST_ENTRY                     *Link;
  LIST_ENTRY                     *NotifyList;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY  *CurrentNotify;

  //
  // KeyCode must in the range of  [0x4, 0x65] or [0xe0, 0xe7].
  //
  KeyDescriptor = GetKeyDescriptor (HidKeyboardDevice, KeyCode);
  if (KeyDescriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (KeyDescriptor->Modifier == EFI_NS_KEY_MODIFIER) {
    //
    // If this is a dead key with EFI_NS_KEY_MODIFIER, then record it and return.
    //
    HidKeyboardDevice->CurrentNsKey = FindHidNsKey (HidKeyboardDevice, KeyDescriptor);
    return EFI_NOT_READY;
  }

  if (HidKeyboardDevice->CurrentNsKey != NULL) {
    //
    // If this keystroke follows a non-spacing key, then find the descriptor for corresponding
    // physical key.
    //
    KeyDescriptor                   = FindPhysicalKey (HidKeyboardDevice->CurrentNsKey, KeyDescriptor);
    HidKeyboardDevice->CurrentNsKey = NULL;
  }

  //
  // Make sure modifier of Key Descriptor is in the valid range according to UEFI spec.
  //
  if (KeyDescriptor->Modifier >= (sizeof (ModifierValueToEfiScanCodeConvertionTable) / sizeof (UINT8))) {
    return EFI_DEVICE_ERROR;
  }

  KeyData->Key.ScanCode    = ModifierValueToEfiScanCodeConvertionTable[KeyDescriptor->Modifier];
  KeyData->Key.UnicodeChar = KeyDescriptor->Unicode;

  if ((KeyDescriptor->AffectedAttribute & EFI_AFFECTED_BY_STANDARD_SHIFT) != 0) {
    if (HidKeyboardDevice->ShiftOn) {
      KeyData->Key.UnicodeChar = KeyDescriptor->ShiftedUnicode;

      //
      // Need not return associated shift state if a class of printable characters that
      // are normally adjusted by shift modifiers. e.g. Shift Key + 'f' key = 'F'
      //
      if ((KeyDescriptor->Unicode != CHAR_NULL) && (KeyDescriptor->ShiftedUnicode != CHAR_NULL) &&
          (KeyDescriptor->Unicode != KeyDescriptor->ShiftedUnicode))
      {
        HidKeyboardDevice->LeftShiftOn  = FALSE;
        HidKeyboardDevice->RightShiftOn = FALSE;
      }

      if (HidKeyboardDevice->AltGrOn) {
        KeyData->Key.UnicodeChar = KeyDescriptor->ShiftedAltGrUnicode;
      }
    } else {
      //
      // Shift off
      //
      KeyData->Key.UnicodeChar = KeyDescriptor->Unicode;

      if (HidKeyboardDevice->AltGrOn) {
        KeyData->Key.UnicodeChar = KeyDescriptor->AltGrUnicode;
      }
    }
  }

  if ((KeyDescriptor->AffectedAttribute & EFI_AFFECTED_BY_CAPS_LOCK) != 0) {
    if (HidKeyboardDevice->CapsOn) {
      if (KeyData->Key.UnicodeChar == KeyDescriptor->Unicode) {
        KeyData->Key.UnicodeChar = KeyDescriptor->ShiftedUnicode;
      } else if (KeyData->Key.UnicodeChar == KeyDescriptor->ShiftedUnicode) {
        KeyData->Key.UnicodeChar = KeyDescriptor->Unicode;
      }
    }
  }

  if ((KeyDescriptor->AffectedAttribute & EFI_AFFECTED_BY_NUM_LOCK) != 0) {
    //
    // For key affected by NumLock, if NumLock is on and Shift is not pressed, then it means
    // normal key, instead of original control key. So the ScanCode should be cleaned.
    // Otherwise, it means control key, so preserve the EFI Scan Code and clear the unicode keycode.
    //
    if ((HidKeyboardDevice->NumLockOn) && (!(HidKeyboardDevice->ShiftOn))) {
      KeyData->Key.ScanCode = SCAN_NULL;
    } else {
      KeyData->Key.UnicodeChar = CHAR_NULL;
    }
  }

  //
  // Translate Unicode 0x1B (ESC) to EFI Scan Code
  //
  if ((KeyData->Key.UnicodeChar == 0x1B) && (KeyData->Key.ScanCode == SCAN_NULL)) {
    KeyData->Key.ScanCode    = SCAN_ESC;
    KeyData->Key.UnicodeChar = CHAR_NULL;
  }

  //
  // Not valid for key without both unicode key code and EFI Scan Code.
  //
  if ((KeyData->Key.UnicodeChar == 0) && (KeyData->Key.ScanCode == SCAN_NULL)) {
    if (!HidKeyboardDevice->IsSupportPartialKey) {
      return EFI_NOT_READY;
    }
  }

  //
  // Save Shift/Toggle state
  //
  InitializeKeyState (HidKeyboardDevice, &KeyData->KeyState);

  //
  // Signal KeyNotify process event if this key pressed matches any key registered.
  //
  NotifyList = &HidKeyboardDevice->NotifyList;
  for (Link = GetFirstNode (NotifyList); !IsNull (NotifyList, Link); Link = GetNextNode (NotifyList, Link)) {
    CurrentNotify = CR (Link, KEYBOARD_CONSOLE_IN_EX_NOTIFY, NotifyEntry, HID_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE);
    if (IsKeyRegistered (&CurrentNotify->KeyData, KeyData)) {
      //
      // The key notification function needs to run at TPL_CALLBACK.
      // It will be invoked in KeyNotifyProcessHandler() which runs at TPL_CALLBACK.
      //
      Enqueue (&HidKeyboardDevice->EfiKeyQueueForNotify, KeyData, sizeof (*KeyData));
      gBS->SignalEvent (HidKeyboardDevice->KeyNotifyProcessEvent);
      break;
    }
  }

  return EFI_SUCCESS;
}

/**
  Create the queue.

  @param  Queue     Points to the queue.
  @param  ItemSize  Size of the single item.

**/
VOID
InitQueue (
  IN OUT  HID_SIMPLE_QUEUE  *Queue,
  IN      UINTN             ItemSize
  )
{
  UINTN  Index;

  Queue->ItemSize = ItemSize;
  Queue->Head     = 0;
  Queue->Tail     = 0;

  if (Queue->Buffer[0] != NULL) {
    FreePool (Queue->Buffer[0]);
  }

  Queue->Buffer[0] = AllocatePool (sizeof (Queue->Buffer) / sizeof (Queue->Buffer[0]) * ItemSize);
  ASSERT (Queue->Buffer[0] != NULL);

  for (Index = 1; Index < sizeof (Queue->Buffer) / sizeof (Queue->Buffer[0]); Index++) {
    Queue->Buffer[Index] = ((UINT8 *)Queue->Buffer[Index - 1]) + ItemSize;
  }
}

/**
  Destroy the queue

  @param Queue    Points to the queue.
**/
VOID
DestroyQueue (
  IN OUT HID_SIMPLE_QUEUE  *Queue
  )
{
  FreePool (Queue->Buffer[0]);
}

/**
  Check whether the queue is empty.

  @param  Queue     Points to the queue.

  @retval TRUE      Queue is empty.
  @retval FALSE     Queue is not empty.

**/
BOOLEAN
IsQueueEmpty (
  IN  HID_SIMPLE_QUEUE  *Queue
  )
{
  //
  // Meet FIFO empty condition
  //
  return (BOOLEAN)(Queue->Head == Queue->Tail);
}

/**
  Check whether the queue is full.

  @param  Queue     Points to the queue.

  @retval TRUE      Queue is full.
  @retval FALSE     Queue is not full.

**/
BOOLEAN
IsQueueFull (
  IN  HID_SIMPLE_QUEUE  *Queue
  )
{
  return (BOOLEAN)(((Queue->Tail + 1) % (MAX_KEY_ALLOWED + 1)) == Queue->Head);
}

/**
  Enqueue the item to the queue.

  @param  Queue     Points to the queue.
  @param  Item      Points to the item to be enqueued.
  @param  ItemSize  Size of the item.
**/
VOID
Enqueue (
  IN OUT  HID_SIMPLE_QUEUE  *Queue,
  IN      VOID              *Item,
  IN      UINTN             ItemSize
  )
{
  ASSERT (ItemSize == Queue->ItemSize);
  //
  // If keyboard buffer is full, throw the
  // first key out of the keyboard buffer.
  //
  if (IsQueueFull (Queue)) {
    Queue->Head = (Queue->Head + 1) % (MAX_KEY_ALLOWED + 1);
  }

  CopyMem (Queue->Buffer[Queue->Tail], Item, ItemSize);

  //
  // Adjust the tail pointer of the FIFO keyboard buffer.
  //
  Queue->Tail = (Queue->Tail + 1) % (MAX_KEY_ALLOWED + 1);
}

/**
  Dequeue a item from the queue.

  @param  Queue     Points to the queue.
  @param  Item      Receives the item.
  @param  ItemSize  Size of the item.

  @retval EFI_SUCCESS        Item was successfully dequeued.
  @retval EFI_DEVICE_ERROR   The queue is empty.

**/
EFI_STATUS
Dequeue (
  IN OUT  HID_SIMPLE_QUEUE  *Queue,
  OUT  VOID                 *Item,
  IN   UINTN                ItemSize
  )
{
  ASSERT (Queue->ItemSize == ItemSize);

  if (IsQueueEmpty (Queue)) {
    return EFI_DEVICE_ERROR;
  }

  CopyMem (Item, Queue->Buffer[Queue->Head], ItemSize);

  //
  // Adjust the head pointer of the FIFO keyboard buffer.
  //
  Queue->Head = (Queue->Head + 1) % (MAX_KEY_ALLOWED + 1);

  return EFI_SUCCESS;
}

/**
Sets HID keyboard LED state.

@param  HidKeyboardDevice  The HID_KB_DEV instance.

**/
VOID
SetKeyLED (
  IN  HID_KB_DEV  *HidKeyboardDevice
  )
{
  KEYBOARD_HID_OUTPUT_BUFFER  HidOutput;

  ASSERT (NULL != HidKeyboardDevice->KeyboardProtocol);

  ZeroMem (&HidOutput, sizeof (KEYBOARD_HID_OUTPUT_BUFFER));

  // Presently, only CapsLock is supported by this driver.
  if (HidKeyboardDevice->CapsOn) {
    HidOutput.CapsLock = 1;
  }

  if (HidKeyboardDevice->NumLockOn) {
    HidOutput.NumLock = 1;
  }

  if (HidKeyboardDevice->ScrollOn) {
    HidOutput.ScrollLock = 1;
  }

  HidKeyboardDevice->KeyboardProtocol->SetOutputReport (
                                         HidKeyboardDevice->KeyboardProtocol,
                                         BootKeyboard,
                                         (UINT8 *)&HidOutput,
                                         sizeof (KEYBOARD_HID_OUTPUT_BUFFER)
                                         );
}

/**
  Handler for Repeat Key event.

  This function is the handler for Repeat Key event triggered
  by timer.
  After a repeatable key is pressed, the event would be triggered
  with interval of HIDKBD_REPEAT_DELAY. Once the event is triggered,
  following trigger will come with interval of HIDKBD_REPEAT_RATE.

  @param  Event              The Repeat Key event.
  @param  Context            Points to the HID_KB_DEV instance.

**/
VOID
EFIAPI
HidKeyboardRepeatHandler (
  IN    EFI_EVENT  Event,
  IN    VOID       *Context
  )
{
  HID_KB_DEV    *HidKeyboardDevice;
  HID_KEY       HIDKey;
  EFI_KEY_DATA  KeyData;

  HidKeyboardDevice = (HID_KB_DEV *)Context;

  //
  // Do nothing when there is no repeat key.
  //
  if (HidKeyboardDevice->RepeatKey != 0) {
    //
    // Inserts the repeat key into keyboard buffer,
    //
    HIDKey.KeyCode = HidKeyboardDevice->RepeatKey;
    HIDKey.Down    = TRUE;
    if (HIDKeyCodeToEfiInputKey (HidKeyboardDevice, HIDKey.KeyCode, &KeyData) == EFI_SUCCESS) {
      Enqueue (&HidKeyboardDevice->EfiKeyQueue, &KeyData, sizeof (KeyData));
    }

    //
    // Set repeat rate for next repeat key generation.
    //
    gBS->SetTimer (
           HidKeyboardDevice->RepeatTimer,
           TimerRelative,
           HIDKBD_REPEAT_RATE
           );
  }
}
