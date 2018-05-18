/** @file
This file defines the Device Firmware Configuration Interface formset guid and page ID *

Copyright (c) 2018, Microsoft Corporation.

**/

#ifndef __DFCI_MENU_FORMSET_GUID_H__
#define __DFCI_MENU_FORMSET_GUID_H__


#define DFCI_MENU_FORMSET_GUID  \
  { \
    0x3b82283d, 0x7add, 0x4c6a, {0xad, 0x2b, 0x71, 0x9b, 0x8d, 0x7b, 0x77, 0xc9} \
  }

#define DFCI_MENU_FORM_ID           0x1000

//
// Recovery Support - Optionally provided by platform
//
#define DFCI_RECOVERY_FORMSET_GUID  \
  { \
  0xcf9873b2, 0xce63, 0x4f63, {0x9a, 0x5f, 0x16, 0x54, 0xe4, 0x01, 0x61, 0xc6 } \
  }

#define DFCI_RECOVERY_FORM_ID       0x1000

#define DFCI_RECOVERY_QUESTION_ID   0x1200


extern EFI_GUID gDfciMenuFormsetGuid;
extern EFI_GUID gDfciRecoveryFormsetGuid;

#endif

