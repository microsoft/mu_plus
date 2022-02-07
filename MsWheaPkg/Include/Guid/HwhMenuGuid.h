/** @file
HwhMenuGuid.h

This file defines the Hardware Health formset guid and page ID.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __HWH_MENU_FORMSET_GUID_H__
#define __HWH_MENU_FORMSET_GUID_H__

// 3b82383d-7add-4c6a-ad2b719b8d7b77c9
#define HWH_MENU_FORMSET_GUID  \
  { \
  0x3b82383d, 0x7add, 0x4c6a, {0xad, 0x2b, 0x71, 0x9b, 0x8d, 0x7b, 0x77, 0xc9} \
  }

#define HWH_MENU_FORM_ID  0x3000

extern EFI_GUID  gHwhMenuFormsetGuid;

#endif
