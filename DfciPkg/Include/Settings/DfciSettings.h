/** @file
DfciSettings.h

This protocol to provide password hashing.

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

#ifndef __DFCI_SETTINGS_H__
#define __DFCI_SETTINGS_H__

// Settings name convention:

// Definer.SettingName.Type
//
// Some settings types are asymetric in that the types are different for getting and
// updating a setting. The HttpsCert, for example, is set with a binary blob of a
// Certificate. On Get, a string of the thumbprint is returned.

//
//  DFCI control settings
//
#define DFCI_SETTING_ID__OWNER_KEY                  "Dfci.OwnerKey.Enum"
#define DFCI_SETTING_ID__USER_KEY                   "Dfci.UserKey.Enum"
#define DFCI_SETTING_ID__USER1_KEY                  "Dfci.User1Key.Enum"
#define DFCI_SETTING_ID__USER2_KEY                  "Dfci.User2Key.Enum"
#define DFCI_SETTING_ID__ZTD_KEY                    "Dfci.ZtdKey.Enum"

// Dfci.ZtdUnenroll.Enable is used as a permission only.  There is no setting. THis
// permission allows a Ztd signed unenroll packet to unenroll and owner.
#define DFCI_SETTING_ID__ZTD_UNENROLL               "Dfci.ZtdUnenroll.Enable"

// Dfci.Recovery.Enable is used as a permission only.  There is no setting.
// When Dfci.Recovery.Enable permission is assigned to the local user, a menu
// is available to initiate a Recovery Reset of DFCI.  The menu presents an encrypted
// challenge that is given to an owner (QrCode, USB key, or raw text) for decoding.
// This will allow the owner to give the local user a response token to disable DFCI.
#define DFCI_SETTING_ID__DFCI_RECOVERY              "Dfci.Recovery.Enable"
#define DFCI_SETTING_ID__ZTD_RECOVERY               "Dfci.Ztd.Recovery.Enable"

// Dfci.RecoveryUrl.String is a setting that the owner can enable that will allow
// UEFI to get updated DFCI settings should the system not be able to boot.
#define DFCI_SETTING_ID__DFCI_RECOVERY_URL          "Dfci.RecoveryUrl.String"
#define DFCI_SETTING_ID__DFCI_BOOTSTRAP_URL         "Dfci.RecoveryBootstrapUrl.String"
#define DFCI_SETTING_ID__DFCI_HTTPS_CERT            "Dfci.HttpsCert.Binary"
#define DFCI_SETTING_ID__DFCI_REGISTRATION_ID       "Dfci.RegistrationId.String"
#define DFCI_SETTING_ID__DFCI_TENANT_ID             "Dfci.TenantId.String"

//
// Second user settings
//
#define DFCI_SETTING_ID__MDM_FRIENDLY_NAME          "MDM.FriendlyName.String"
#define DFCI_SETTING_ID__MDM_TENANT_NAME            "MDM.TenantName.String"

//
// Group Settings, and their individual settings
//

// AllCameras Group Setting.
// AllCameras Enable allows the cameras to be enabled.  AllCameras disabled
// ensures all cameras are disabled.  When received as a setting, all cameras
// will be disabled, and individual settings will be disabled, and grayed out.
#define DFCI_SETTING_ID__ALL_CAMERAS                "Dfci.OnboardCameras.Enable"

// Cpu and I/O Virtualization Settings
//
#define DFCI_SETTING_ID__ALL_CPU_IO_VIRT            "Dfci.CpuAndIoVirtualization.Enable"

//
// Onboard Audio devices settings
//
#define DFCI_SETTING_ID__ALL_AUDIO                  "Dfci.OnboardAudio.Enable"

//
//  Onboard radios
//
#define DFCI_SETTING_ID__ALL_RADIOS                 "Dfci.OnboardRadios.Enable"

//
// Boot from External Media
//
#define DFCI_SETTING_ID__EXTERNAL_MEDIA             "Dfci.BootExternalMedia.Enable"

//
// Boot from On Board Network
//
#define DFCI_SETTING_ID__ENABLE_NETWORK             "Dfci.BootOnboardNetwork.Enable"


#endif //  __DFCI_SETTINGS_H__
