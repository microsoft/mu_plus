// DfciSettings.h provides the Dfci Policy settings information

/** @file
 *Header file for Dfci Policy settings

 *Copyright (c) 2018, Microsoft Corporation. All rights
 *reserved.<BR> This software and associated documentation (if
 *any) is furnished under a license and may only be used or
 *copied in accordance with the terms of the license. Except as
 *permitted by such license, no part of this software or
 *documentation may be reproduced, stored in a retrieval system,
 *or transmitted in any form or by any means without the express
 *written consent of Microsoft Corporation.


**/

#ifndef __DFCI_SETTINGS_GUID_H__
#define __DFCI_SETTINGS_GUID_H__


#define DFCI_SETTINGS_URL_NAME       L"DfciUrl"
#define DFCI_SETTINGS_URL_CERT_NAME  L"DfciUrlCert"
#define DFCI_SETTINGS_HWID_NAME      L"DfciHwid"
#define DFCI_SETTINGS_ATTRIBUTES (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE )

#define DFCI_SETTINGS_GUID  \
  { \
    0x9548f732, 0xd482, 0x4a39, { 0x8b, 0x27, 0xcd, 0x99, 0x18, 0x11, 0xba, 0x6a } \
  }

extern EFI_GUID gDfciSettingsGuid;

#endif //  __DFCI_SETTINGS_GUID_H__


