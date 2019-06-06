/** @file
DfciSettingPermissionLib.h

Library supports Permissions for Settings based on Identity.  This should
only be linked into the SettingsManager and is not for other modules usage.

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
