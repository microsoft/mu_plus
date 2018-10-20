/** @file
 *Header file for Ms Boot Policy settings, providing the Bds Policy settings information

Copyright (c) 2015 - 2018, Microsoft Corporation

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

#ifndef _MS_BOOT_MANAGER_SETTINGS_H_
#define _MS_BOOT_MANAGER_SETTINGS_H_

#define MS_BOOT_MANAGER_SETTINGS_SIGNATURE     SIGNATURE_32 ('S', 'P', 'B', 'M')
#define MS_BOOT_MANAGER_SETTINGS_SIGNATURE_OLD SIGNATURE_32 ('S', 'P', 'M', 'B')

#define MS_BOOT_MANAGER_SETTINGS_NAME  L"MsBootPolicySettings"
#define MS_BOOT_MANAGER_SETTINGS_ATTRIBUTES (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE )

#define MS_BOOT_MANAGER_SETTINGS_VERSON1  0         // Original version
#define MS_BOOT_MANAGER_SETTINGS_VERSON2  1         // USB Boot is valid
#define MS_BOOT_MANAGER_SETTINGS_VERSON3  2         // StartNetwork is valid

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


