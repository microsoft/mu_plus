/** @file
 *Header file for Ms Boot Manager Settings

Copyright (c) 2018, Microsoft Corporation

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

#ifndef __BOOT_MENU_SETTINGS_H__
#define __BOOT_MENU_SETTINGS_H__

#define DFCI_SETTING_ID__IPV6                       "Device.IPv6Pxe.Enable"
#define DFCI_SETTING_ID__BOOT_ORDER_LOCK            "Device.BootOrderLock.Enable"
#define DFCI_SETTING_ID__ENABLE_USB_BOOT            "Device.USBBoot.Enable"

#define DFCI_SETTING_ID__ALT_BOOT                   "OEM.AlternateBoot.Enable"
#define DFCI_SETTING_ID__START_NETWORK              "OEM.StartNetwork.Enable"

#endif //  __BOOT_MENU_SETTINGS_H__
