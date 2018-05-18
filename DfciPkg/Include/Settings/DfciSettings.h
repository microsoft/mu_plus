// Dfci Settings

/** @file
 *Header file for Dfci Settings

 *Copyright (c) 2018, Microsoft Corporation. All rights
 *reserved.<BR> This software and associated documentation (if
 *any) is furnished under a license and may only be used or
 *copied in accordance with the terms of the license. Except as
 *permitted by such license, no part of this software or
 *documentation may be reproduced, stored in a retrieval system,
 *or transmitted in any form or by any means without the express
 *written consent of Microsoft Corporation.


**/

#ifndef __DFCI_SETTINGS_H__
#define __DFCI_SETTINGS_H__

// Settings name convention:

// Definer.SettingName.Type
//
// For the case where Get and Set are different types:
// Definer.SettingName.TypeGet_TypeSet
//
// The HttpsCert, for example, is set with a binary blob of a Certificate. On Get,
// a string made up of identifiers in the certificate is returned.


#define DFCI_SETTING_ID__OWNER_KEY      "Dfci.OwnerKey.Enum"
#define DFCI_SETTING_ID__USER_KEY       "Dfci.UserKey.Enum"
#define DFCI_SETTING_ID__USER1_KEY      "Dfci.User1Key.Enum"
#define DFCI_SETTING_ID__USER2_KEY      "Dfci.User2Key.Enum"

//This is used for permission only - Identity can perform recovery challenge/response operation
#define DFCI_SETTING_ID__DFCI_RECOVERY  "Dfci.SemmRecovery.Enable"

#define DFCI_SETTING_ID__DFCI_URL       "Dfci.RecoveryUrl.String"
#define DFCI_SETTING_ID__DFCI_URL_CERT  "Dfci.HttpsCert.Cert"
#define DFCI_SETTING_ID__DFCI_HWID      "Dfci.Hwid.String"


#endif //  __DFCI_SETTINGS_H__
