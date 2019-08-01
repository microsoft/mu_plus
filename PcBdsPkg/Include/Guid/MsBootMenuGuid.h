/** @file
This file defines the Boot Manager formset guid and page ID * 

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MS_BOOT_MENU_FORMSET_GUID_H__
#define __MS_BOOT_MENU_FORMSET_GUID_H__


#define MS_BOOT_MENU_FORMSET_GUID  \
  { \
  0x4123defc, 0x3eb8, 0x433c, {0x89, 0x19, 0x12, 0x79, 0x00, 0xcc, 0x26, 0x0f } \
  }

#define MS_BOOT_ORDER_FORM_ID       0x1000

extern EFI_GUID gMsBootMenuFormsetGuid;

#endif

