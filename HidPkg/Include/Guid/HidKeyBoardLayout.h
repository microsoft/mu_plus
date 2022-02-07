/** @file
  HID KeyBoard Layout GUIDs

Copyright (C) Microsoft Corporation. All rights reserved.

Portions Derived from UsbKeyBoardLayout.h
Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __HID_KEYBOARD_LAYOUT_GUID_H__
#define __HID_KEYBOARD_LAYOUT_GUID_H__

//
// GUID for HID keyboard HII package list.
//
#define HID_KEYBOARD_LAYOUT_PACKAGE_GUID \
  { \
    0x57adcfa9, 0xc08a, 0x40b9, {0x86, 0x87, 0x65, 0xe2, 0xec, 0xe0, 0xe5, 0xe1} \
  }

//
// GUID for HID keyboard layout
//
#define HID_KEYBOARD_LAYOUT_KEY_GUID \
  { \
    0xdcafaba8, 0xcde5, 0x40a1, {0x9a, 0xd3, 0x48, 0x76, 0x44, 0x71, 0x7e, 0x47} \
  }

extern EFI_GUID  gHidKeyboardLayoutPackageGuid;
extern EFI_GUID  gHidKeyboardLayoutKeyGuid;

#endif
