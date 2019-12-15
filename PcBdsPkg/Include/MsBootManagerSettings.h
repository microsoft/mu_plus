/** @file
 *Header file for Ms Boot Policy settings, providing the Bds Policy settings information

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MS_BOOT_MANAGER_SETTINGS_H_
#define _MS_BOOT_MANAGER_SETTINGS_H_

#define MS_BOOT_MANAGER_SETTINGS_SIGNATURE     SIGNATURE_32 ('S', 'P', 'B', 'M')
#define MS_BOOT_MANAGER_SETTINGS_SIGNATURE_OLD SIGNATURE_32 ('S', 'P', 'M', 'B')

#define MS_BOOT_MANAGER_SETTINGS_NAME  L"MsBootPolicySettings"
#define MS_BOOT_MANAGER_SETTINGS_ATTRIBUTES (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE )

#define MS_BOOT_MANAGER_SETTINGS_VERSION1  0         // Original version
#define MS_BOOT_MANAGER_SETTINGS_VERSION2  1         // USB Boot is valid
#define MS_BOOT_MANAGER_SETTINGS_VERSION3  2         // StartNetwork is valid

typedef struct {
    // Currently a 16 byte structure.  When initialized, the unused values are zero.
    UINT32  Signature;
    UINT8   IPv6;           // IPv6 Network Boot: 1 = Enabled,  0 = Disabled
    UINT8   AltBoot;        // Enable Alt Boot:   1 = Enabled,  0 = Disabled
    UINT8   BootOrderLock;  // BootOrder          1 = Locked    0 = Unlocked
    UINT8   EnableUsbBoot;  // EnableUsbBoot      1 = BootUSB   0 = Don't Boot USB
    UINT8   StartNetwork;   // StartNetwork       1 = Enabled   0 = Don't enable
    UINT8   Reserved[6];
    UINT8   Version;        //
} MS_BOOT_MANAGER_SETTINGS;

extern EFI_GUID gMsBootManagerSettingsGuid;

#endif //  _MS_BOOT_MANAGER_SETTINGS_H_


