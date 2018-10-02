/** @file
DfciSettingPermissions.h

Defines the System Settings Permissions Protocol.

This protocol allows modules to get the Permission for a given setting.

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

#ifndef __DFCI_SETTING_PERMISSIONS_H__
#define __DFCI_SETTING_PERMISSIONS_H__  


/**
Define the DFCI_SETTING_PERMISSIONS_PROTOCOL related structures
**/
typedef struct _DFCI_SETTING_PERMISSIONS_PROTOCOL DFCI_SETTING_PERMISSIONS_PROTOCOL;

/*
Get the Permission Mask for a given setting

@param[in]  This:           Permission Protocol
@param[in]  Id:             Setting ID to Get Permission for
@param[OUT] PermissionMask  Permission Mask for the requested setting.     

@retval EFI_SUCCESS - PermissionMask Parameter value is set to the value in System Permissions
@retval Error -  Error and PermissionMask Parameter is not updated. 

*/
typedef
EFI_STATUS
(EFIAPI *DFCI_SETTING_PERMISSIONS_GET_PERMISSION) (
  IN  CONST DFCI_SETTING_PERMISSIONS_PROTOCOL *This,
  IN  DFCI_SETTING_ID_STRING                   Id,
  OUT DFCI_PERMISSION_MASK                    *PermissionMask
);

/*
Clear All the System Permissions

@param[in]  This:           Permission Protocol
@param[in]  AuthToken       Token used for authority.

@retval EFI_SUCCESS - System permissions cleared
@retval Error -  Error

*/
typedef
EFI_STATUS
(EFIAPI *DFCI_SETTING_PERMISSIONS_RESET_PERMISSIONS) (
  IN  CONST DFCI_SETTING_PERMISSIONS_PROTOCOL *This,
  IN  CONST DFCI_AUTH_TOKEN                   *AuthToken
  );

/*
Inform Permissions that the Owner Key Changed

@param[in]  This:           Permission Protocol
@param[in]  AuthToken       Token used for authority.

@retval EFI_SUCCESS - System permissions cleared
@retval Error -  Error

*/
typedef
EFI_STATUS
(EFIAPI *DFCI_SETTING_PERMISSIONS_IDENTITY_CHANGE) (
  IN  CONST DFCI_SETTING_PERMISSIONS_PROTOCOL *This,
  IN  CONST DFCI_AUTH_TOKEN                   *AuthToken,
  IN        DFCI_IDENTITY_ID                   CertIdentity,
  IN        BOOLEAN                            Enroll
  );

//
// Protocol def
//
#pragma pack (push, 1)
struct _DFCI_SETTING_PERMISSIONS_PROTOCOL {
  DFCI_SETTING_PERMISSIONS_GET_PERMISSION     GetPermission;
  DFCI_SETTING_PERMISSIONS_RESET_PERMISSIONS  ResetPermissions;
  DFCI_SETTING_PERMISSIONS_IDENTITY_CHANGE    IdentityChange;
};
#pragma pack (pop)


extern EFI_GUID     gDfciSettingPermissionsProtocolGuid;

#endif // __DFCI_SETTING_PERMISSIONS_H__
