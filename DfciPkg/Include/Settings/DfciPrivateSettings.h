/** @file
Settings.h

This protocol to provide password hashing.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DFCI_PRIVATE_SETTINGS_H__
#define __DFCI_PRIVATE_SETTINGS_H__

// Settings name convention:

// Definer.SettingName.Type
//
// Some settings types are asymmetric in that the types are different for getting and
// updating a setting. The HttpsCert, for example, is set with a binary blob of a
// Certificate. On Get, a string of the thumbprint is returned.

//
//  DFCI control settings
//
#define DFCI_PRIVATE_SETTING_ID__OWNER_KEY  "Dfci.OwnerKey.Enum"
#define DFCI_PRIVATE_SETTING_ID__USER_KEY   "Dfci.UserKey.Enum"
#define DFCI_PRIVATE_SETTING_ID__USER1_KEY  "Dfci.User1Key.Enum"
#define DFCI_PRIVATE_SETTING_ID__USER2_KEY  "Dfci.User2Key.Enum"
#define DFCI_PRIVATE_SETTING_ID__ZTD_KEY    "Dfci.ZtdKey.Enum"

// Dfci.ZtdUnenroll.Enable is used as a permission only.  There is no setting. This
// permission allows a Ztd signed unenroll packet to unenroll and owner.
#define DFCI_PRIVATE_SETTING_ID__ZTD_UNENROLL  "Dfci.ZtdUnenroll.Enable"

// Dfci.Recovery.Enable is used as a permission only.  There is no setting.
// When Dfci.Recovery.Enable permission is assigned to the local user, a menu
// is available to initiate a Recovery Reset of DFCI.  The menu presents an encrypted
// challenge that is given to an owner (QrCode, USB key, or raw text) for decoding.
// This will allow the owner to give the local user a response token to disable DFCI.
#define DFCI_PRIVATE_SETTING_ID__DFCI_RECOVERY  "Dfci.Recovery.Enable"
#define DFCI_PRIVATE_SETTING_ID__ZTD_RECOVERY   "Dfci.Ztd.Recovery.Enable"

// Dfci.RecoveryUrl.String is a setting that the owner can enable that will allow
// UEFI to get updated DFCI settings should the system not be able to boot.
#define DFCI_PRIVATE_SETTING_ID__DFCI_RECOVERY_URL     "Dfci.RecoveryUrl.String"
#define DFCI_PRIVATE_SETTING_ID__DFCI_BOOTSTRAP_URL    "Dfci.RecoveryBootstrapUrl.String"
#define DFCI_PRIVATE_SETTING_ID__DFCI_HTTPS_CERT       "Dfci.HttpsCert.Binary"
#define DFCI_PRIVATE_SETTING_ID__DFCI_REGISTRATION_ID  "Dfci.RegistrationId.String"
#define DFCI_PRIVATE_SETTING_ID__DFCI_TENANT_ID        "Dfci.TenantId.String"

//
// Second user settings
//
#define DFCI_PRIVATE_SETTING_ID__MDM_FRIENDLY_NAME  "MDM.FriendlyName.String"
#define DFCI_PRIVATE_SETTING_ID__MDM_TENANT_NAME    "MDM.TenantName.String"

#endif //  __DFCI_PRIVATE_SETTINGS_H__
