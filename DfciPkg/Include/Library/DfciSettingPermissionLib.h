/** @file
DfciSettingPermissionLib.h

Library supports Permissions for Settings based on Identity.  This should
only be linked into the SettingsManager and is not for other modules usage.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
  IN  DFCI_SETTING_ID_STRING       SettingId,
  IN  DFCI_SETTING_ID_STRING       GroupId     OPTIONAL,
  IN  CONST DFCI_AUTH_TOKEN       *AuthToken,
  OUT BOOLEAN                     *Result
  );

/**
 Check if the current AuthToken has unenroll permission. This
 includes recovery, which is a form of unenroll.
 **/
EFI_STATUS
EFIAPI
HasUnenrollPermission (
  IN  CONST DFCI_AUTH_TOKEN       *AuthToken,
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
  IN  DFCI_SETTING_ID_STRING   SettingId,
  OUT DFCI_PERMISSION_MASK    *Permissions
  );

EFI_STATUS
EFIAPI
IdentityChange(
  IN  CONST DFCI_AUTH_TOKEN     *AuthToken,
  IN        DFCI_IDENTITY_ID     CertIdentity,
  IN        BOOLEAN              Enroll
  );

#endif //__DFCI_SETTING_PERMISSION_LIB_H__
