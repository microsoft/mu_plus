// MsBootManagerSettingsLib.h provides a methos of DXE driver read access to boot manager settings

/** @file
 *Header file for Ms Boot Manager Settings settings

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MS_BOOT_MANAGER_SETTINGS_LIB_H_
#define _MS_BOOT_MANAGER_SETTINGS_LIB_H_

/**
Function to Get a Boot Manager Setting.
If the setting has not be previously set this function will return the default.  However it will
not cause the default to be set.

@param Id:      The MsSystemSettingsId for the setting
@param Value:   Ptr to a boolean value for the setting to be returned.  Enabled = True, Disabled = False


@retval: Success - Setting was returned in Value
@retval: EFI_ERROR.  Settings was not returned in Value.
**/
EFI_STATUS
EFIAPI
GetBootManagerSetting
(
  IN  DFCI_SETTING_ID_STRING   Id,
  OUT BOOLEAN                 *Value
);

#endif  // _MS_BOOT_MANAGER_SETTINGS_LIB_H_


