/** @file
DfciSettingAccess.h

Defines the System Settings Access Protocol.

This protocol allows modules to get and set a setting.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DFCI_SETTING_ACCESS_H__
#define __DFCI_SETTING_ACCESS_H__

/**
Define the DFCI_SYSTEM_SETTING_ACCESS related structures
**/
typedef struct _DFCI_SETTING_ACCESS_PROTOCOL DFCI_SETTING_ACCESS_PROTOCOL;

/*
Set a single setting

@param[in] This:       Access Protocol
@param[in] Id:         Setting ID to set
@param[in] AuthToken:  A valid auth token to apply the setting using.  This auth token will be validated
                       to check permissions for changing the setting.
@param[in] Type:       Type that caller expects this setting to be.
@param[in] Value:      A pointer to a datatype defined by the Type for this setting.
@param[in,out] Flags:  Informational Flags passed to the SET and/or Returned as a result of the set

@retval EFI_SUCCESS if setting could be set.  Check flags for other info (reset required, etc)
@retval Error - Setting not set.

*/
typedef
EFI_STATUS
(EFIAPI *DFCI_SETTING_ACCESS_SET)(
  IN  CONST DFCI_SETTING_ACCESS_PROTOCOL   *This,
  IN  DFCI_SETTING_ID_STRING                Id,
  IN  CONST DFCI_AUTH_TOKEN                *AuthToken,
  IN  DFCI_SETTING_TYPE                     Type,
  IN  UINTN                                 ValueSize,
  IN  CONST VOID                           *Value,
  IN OUT DFCI_SETTING_FLAGS                *Flags
  );

/*
Get a single setting

@param[in] This:         Access Protocol
@param[in] Id:           Setting ID to Get
@param[in] AuthToken:    An optional auth token* to use to check permission of setting.  This auth token will be validated
                         to check permissions for changing the setting which will be reported in flags if valid.
@param[in] Type:         Type that caller expects this setting to be.
@param[in,out] ValueSize IN=Size of location to store value
                         OUT=Size of value stored
@param[out] Value:       A pointer to a datatype defined by the Type for this setting.
@param[IN OUT] Flags     Optional Informational flags passed back from the Get operation.  If the Auth Token is valid write access will be set in
                         flags for the given auth.

@retval EFI_SUCCESS if setting could be set.  Check flags for other info (reset required, etc)
@retval Error - couldn't get setting.

*/
typedef
EFI_STATUS
(EFIAPI *DFCI_SETTING_ACCESS_GET)(
  IN  CONST DFCI_SETTING_ACCESS_PROTOCOL *This,
  IN  DFCI_SETTING_ID_STRING              Id,
  IN  CONST DFCI_AUTH_TOKEN              *AuthToken  OPTIONAL,
  IN  DFCI_SETTING_TYPE                   Type,
  IN  OUT UINTN                          *ValueSize,
  OUT     VOID                           *Value,
  IN  OUT DFCI_SETTING_FLAGS             *Flags OPTIONAL
  );

/*
Reset Settings Access

This will clear all internal Settings Access Data
This will reset all settings that have DFCI_SETTING_FLAGS_NO_PREBOOT_UI set

@param[in] This:        Access Protocol
@param[in] AuthToken:   An  auth token to authorize the operation.  Only an auth token with recovery and/or Owner Auth Key permissions
                        can perform a reset.

@retval EFI_SUCCESS   - Settings access clear completed
@retval Error - failed

*/
typedef
EFI_STATUS
(EFIAPI *DFCI_SETTING_ACCESS_RESET)(
  IN  CONST DFCI_SETTING_ACCESS_PROTOCOL *This,
  IN  CONST DFCI_AUTH_TOKEN              *AuthToken
  );

//
// Protocol def
//
#pragma pack (push, 1)
struct _DFCI_SETTING_ACCESS_PROTOCOL {
  DFCI_SETTING_ACCESS_SET      Set;
  DFCI_SETTING_ACCESS_GET      Get;
  DFCI_SETTING_ACCESS_RESET    Reset;
};

#pragma pack (pop)

extern EFI_GUID  gDfciSettingAccessProtocolGuid;

#endif // __DFCI_SETTING_ACCESS_H__
