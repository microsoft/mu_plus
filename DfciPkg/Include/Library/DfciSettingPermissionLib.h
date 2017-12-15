/** @file
DfciSettingPermissionLib - Library supports Permissions for Settings based 
on Identity.  This should only be linked into the SettingsManager and is not
for other modules usage.  


Copyright (C) 2014 Microsoft Corporation. All Rights Reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

**/

#ifndef __DFCI_SETTING_PERMISSION_LIB_H__
#define __DFCI_SETTING_PERMISSION_LIB_H__

/**
Return if the User Identified by AuthToken
has write permission to the setting identified
by the SettingId.  

If error in processing request return bad status code
otherwise Result will be updated with 
TRUE  - user has auth to write
FALSE - user has read only access
**/
EFI_STATUS
EFIAPI
HasWritePermissions(
  IN  DFCI_SETTING_ID_ENUM        SettingId,
  IN  CONST DFCI_AUTH_TOKEN         *AuthToken,
  OUT BOOLEAN                     *Result
  );


/**
Clear all current permission settings 
and restore to an all open system permissions

No Auth is needed for this to support recovery case
If Auth supplied then it must be an Auth with Permission
to change the Owner Key

**/
EFI_STATUS
EFIAPI
ResetPermissionsToDefault(
IN CONST DFCI_AUTH_TOKEN *AuthToken OPTIONAL
);


EFI_STATUS
EFIAPI
QueryPermission(
  IN  DFCI_SETTING_ID_ENUM   SettingId,
  OUT DFCI_PERMISSION_MASK    *Permissions
  );

#endif //__DFCI_SETTING_PERMISSION_LIB_H__
