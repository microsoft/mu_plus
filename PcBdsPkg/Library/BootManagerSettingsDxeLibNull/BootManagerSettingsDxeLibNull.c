/** @file
 *DeviceBootManager  - Ms Device specific extensions to BdsDxe.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Settings/BootMenuSettings.h>

/**
Function to Get a Boot Manager Setting.
If the setting has not be previously set this function will return the default.  However it will
not cause the default to be set.

@param Id:      The BootManagerSettingId for the setting
@param Value:   Ptr to a boolean value for the setting to be returned.  Enabled = True, Disabled = False


@retval: Success - Setting was returned in Value
@retval: EFI_ERROR.  Settings was not returned in Value.
**/
EFI_STATUS
EFIAPI
GetBootManagerSetting (
  IN  DFCI_SETTING_ID_STRING  Id,
  OUT BOOLEAN                 *Value
  )
{
  return EFI_NOT_FOUND;
}
