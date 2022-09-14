/** @file
DfciSettingPermissions.h

Defines the System Settings Permissions Protocol.

This protocol allows modules to get the Permission for a given setting.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DFCI_SETTING_PERMISSIONS_H__
#define __DFCI_SETTING_PERMISSIONS_H__

/**
Define the DFCI_SETTING_PERMISSIONS_PROTOCOL related structures
**/
typedef struct _DFCI_SETTING_PERMISSIONS_PROTOCOL DFCI_SETTING_PERMISSIONS_PROTOCOL;

typedef enum {
  FIRST_ENROLL,            // Transitioning from unenrolled to enrolled.
  ENROLL,                  // Rolling certs (enrolled to enrolled with different cert).
  UNENROLL                 // Transitioning from enrolled to unenrolled.
} IDENTITY_CHANGE_TYPE;

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
(EFIAPI *DFCI_SETTING_PERMISSIONS_GET_PERMISSION)(
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
(EFIAPI *DFCI_SETTING_PERMISSIONS_RESET_PERMISSIONS)(
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
(EFIAPI *DFCI_SETTING_PERMISSIONS_IDENTITY_CHANGE)(
  IN  CONST DFCI_SETTING_PERMISSIONS_PROTOCOL *This,
  IN  CONST DFCI_AUTH_TOKEN                   *AuthToken,
  IN        DFCI_IDENTITY_ID                   CertIdentity,
  IN        IDENTITY_CHANGE_TYPE               ChangeType
  );

//
// Protocol def
//
#pragma pack (push, 1)
struct _DFCI_SETTING_PERMISSIONS_PROTOCOL {
  DFCI_SETTING_PERMISSIONS_GET_PERMISSION       GetPermission;
  DFCI_SETTING_PERMISSIONS_RESET_PERMISSIONS    ResetPermissions;
  DFCI_SETTING_PERMISSIONS_IDENTITY_CHANGE      IdentityChange;
};

#pragma pack (pop)

extern EFI_GUID  gDfciSettingPermissionsProtocolGuid;

#endif // __DFCI_SETTING_PERMISSIONS_H__
