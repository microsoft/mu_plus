/** @file
 *Header file for Ms Boot Manager Settings

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __BOOT_MENU_SETTINGS_H__
#define __BOOT_MENU_SETTINGS_H__

/**
This file provides the settings in a compatible manner with DFCI, without
actually using DFCI.  It does this by defining the settings type as a CHAR8
string.

WARNING:

When used with DFCI, this header file MUST be included after:

  #include <DfciSystemSettingTypes.h>

or any other include file that includes DfciSystemSettingTypes.h.

Failure to do so will cause a compile error in DfciSystemSettingTypes.h when it
defines DFCI_SETTINT_ID_STRING.
**/
#ifndef __DFCI_SETTING_TYPES_H__
typedef CONST CHAR8 *DFCI_SETTING_ID_STRING;
#endif

#define DFCI_SETTING_ID__IPV6             "Device.IPv6Pxe.Enable"
#define DFCI_SETTING_ID__BOOT_ORDER_LOCK  "Device.BootOrderLock.Enable"
#define DFCI_SETTING_ID__ENABLE_USB_BOOT  "Device.USBBoot.Enable"
#define DFCI_SETTING_ID__ALT_BOOT         "OEM.AlternateBoot.Enable"
#define DFCI_SETTING_ID__START_NETWORK    "OEM.StartNetwork.Enable"

#endif //  __BOOT_MENU_SETTINGS_H__
