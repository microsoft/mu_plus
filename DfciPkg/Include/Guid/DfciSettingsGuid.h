/** @file
DfciSettingsGuid.h

Header file for DfciSettings internal variables

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DFCI_SETTINGS_GUID_H__
#define __DFCI_SETTINGS_GUID_H__


#define DFCI_SETTINGS_RECOVERY_URL_NAME     L"DfciUrl"
#define DFCI_SETTINGS_BOOTSTRAP_URL_NAME    L"DfciUrl2"
#define DFCI_SETTINGS_HTTPS_CERT_NAME       L"DfciHttpsCert"
#define DFCI_SETTINGS_REGISTRATION_ID_NAME  L"DfciRegistrationId"
#define DFCI_SETTINGS_TENANT_ID_NAME        L"DfciTenantId"
#define DFCI_SETTINGS_FRIENDLY_NAME         L"MdmFriendlyName"
#define DFCI_SETTINGS_TENANT_NAME           L"MdmTenantName"

#define DFCI_SETTINGS_WPBT_NAME             L"WPBTen"
#define DFCI_SETTINGS_ATTRIBUTES (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE )

#define DFCI_SETTINGS_GUID  \
  { \
    0x9548f732, 0xd482, 0x4a39, { 0x8b, 0x27, 0xcd, 0x99, 0x18, 0x11, 0xba, 0x6a } \
  }

extern EFI_GUID gDfciSettingsGuid;

#endif //  __DFCI_SETTINGS_GUID_H__


