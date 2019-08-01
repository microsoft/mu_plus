/** @file
  UEFI FormsBrowser Extensions.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MS_UEFI_CHECKBOX_H__
#define __MS_UEFI_CHECKBOX_H__

// Ordered List extensions
#define EMBEDDED_CHECKBOX              ((UINT8) 0x80)
#define EMBEDDED_DELETE                ((UINT8) 0x40)

// The input/output value of a UINT32 value array when the EMBEDDED extensions are use does the following:
// -- The low 16 bits represent the boot option value for the menu item
// -- On input, the CHECKBOX_VALUE is the LOAD_OPTION_ACTIVE bit.
// -- On output, the CHECKBOX_VALUE is used to set the LOAD_OPTION_ACTIVE bit.
// -- On input only, ALLOW_DELETE_VALUE is used to tell Listbox that an entry is allowed to be deleted.
// -- On output only, the BOOT_VALUE is used to tell the boot menu code to boot the selected item.
#define ORDERED_LIST_CHECKBOX_VALUE_32     ((UINT32)0x01000000)   // Currently, only value type UINT32 is supported
#define ORDERED_LIST_ALLOW_DELETE_VALUE_32 ((UINT32)0x02000000)   // Currently, only value type UINT32 is supported
#define ORDERED_LIST_BOOT_VALUE_32         ((UINT32)0x80000000)   // Currently, only value type UINT32 is supported

#endif
