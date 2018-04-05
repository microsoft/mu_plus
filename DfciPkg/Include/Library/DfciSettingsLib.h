// DfciSettingsLib.h provides a method for DXE drivers to access Dfci Settings

/** @file
 *Header file for Dfci Settings

 *Copyright (c) 2015, Microsoft Corporation. All rights
 *reserved.<BR> This software and associated documentation (if
 *any) is furnished under a license and may only be used or
 *copied in accordance with the terms of the license. Except as
 *permitted by such license, no part of this software or
 *documentation may be reproduced, stored in a retrieval system,
 *or transmitted in any form or by any means without the express
 *written consent of Microsoft Corporation.


**/

#ifndef __DFCI_SETTINGS_LIB_H__
#define __DFCI_SETTINGS_LIB_H__

/**
Function to Get a Dfci Setting.
If the setting has not be previously set this function will return the default.  However it will
not cause the default to be set.

@param Id:          The DFCI_SETTING_ID_STRING of the Dfci
@param ValueSize:   IN=Size Of Buffer or 0 to get size, OUT=Size of returned Value
@param Value:       Ptr to a buffer for the setting to be returned.


@retval: Success - Setting was returned in Value
@retval: EFI_ERROR.  Settings was not returned in Value.
**/
EFI_STATUS
EFIAPI
GetDfciSetting
(
  IN      DFCI_SETTING_ID_STRING   Id,
  IN  OUT UINTN                   *ValueSize,
  OUT     VOID                    *Value
);

#endif  // __DFCI_SETTINGS_LIB_H__
