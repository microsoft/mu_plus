/**@file
SettingsManager.h

Common header file for Settings Manager

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <PiDxe.h>

#include <XmlTypes.h>
#include <DfciSystemSettingTypes.h>

#include <Guid/DfciInternalVariableGuid.h>
#include <Guid/DfciDeviceIdVariables.h>
#include <Guid/DfciPacketHeader.h>
#include <Guid/DfciSettingsManagerVariables.h>
#include <Guid/WinCertificate.h>

#include <Protocol/DfciApplyPacket.h>
#include <Protocol/DfciAuthentication.h>
#include <Protocol/DfciSettingAccess.h>
#include <Protocol/DfciSettingsProvider.h>
#include <Protocol/DfciSettingPermissions.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DfciDeviceIdSupportLib.h>
#include <Library/DfciGroupLib.h>
#include <Library/DfciSettingPermissionLib.h>
#include <Library/DfciV1SupportLib.h>
#include <Library/DfciXmlDeviceIdSchemaSupportLib.h>
#include <Library/DfciXmlSettingSchemaSupportLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PerformanceLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/XmlTreeLib.h>
#include <Library/XmlTreeQueryLib.h>

#include <Private/DfciGlobalPrivate.h>

#include <Settings/DfciSettings.h>

typedef enum {
  DfciUsbPortEnabled = 0,   //Port Enabled and Usable in preboot and os.  Including Boot
  DfciUsbPortNoBoot,        //Port Enabled and Usable in preboot and os.  BDS will not boot from port         //<<< Not implemented. Can use BDS option for this to block all USB.
  DfciUsbPortHwDisabledExceptAuthorizedRecover,  //Port Disabled in HW except when factory requested R&R  //<<< Not implemented
  DfciUsbPortHwDisabled  = 0xF0,                  //This blocks factory recovery process
  DfciUsbPortStateMax   = 0xFF
} DFCI_VIRTUAL_USB_PORT_STATE;


//
// List of Settings Groups
//
#define DFCI_GROUP_LIST_ENTRY_SIGNATURE SIGNATURE_32('M','S','S','G')
#define GROUP_LIST_ENTRY_FROM_GROUP_LINK(a)    CR (a, DFCI_GROUP_LIST_ENTRY, GroupLink, DFCI_GROUP_LIST_ENTRY_SIGNATURE)

typedef struct {
  UINTN Signature;
  DFCI_SETTING_ID_STRING GroupId;
  LIST_ENTRY GroupLink;             // Link to next DFCI_GROUP_LIST_ENTRY
  LIST_ENTRY MemberHead;         // Head of list of DFCI_MEMBER_LIST_ENTRYs
} DFCI_GROUP_LIST_ENTRY;

//
// List of Settings Providers
//
#define DFCI_SETTING_PROVIDER_LIST_ENTRY_SIGNATURE SIGNATURE_32('M','S','S','P')
#define PROV_LIST_ENTRY_FROM_PROVIDER(a) CR (a, DFCI_SETTING_PROVIDER_LIST_ENTRY, Provider, DFCI_SETTING_PROVIDER_LIST_ENTRY_SIGNATURE)
#define PROV_LIST_ENTRY_FROM_LINK(a)     CR (a, DFCI_SETTING_PROVIDER_LIST_ENTRY, Link, DFCI_SETTING_PROVIDER_LIST_ENTRY_SIGNATURE)

typedef struct {
  UINTN Signature;
  LIST_ENTRY Link;
  DFCI_SETTING_PROVIDER Provider;
} DFCI_SETTING_PROVIDER_LIST_ENTRY;

//
// List of Member Settings in a group
//
#define DFCI_MEMBER_ENTRY_SIGNATURE SIGNATURE_32('M','S','S','M')
#define MEMBER_LIST_ENTRY_FROM_MEMBER_LINK(a)  CR (a, DFCI_MEMBER_LIST_ENTRY, MemberLink, DFCI_MEMBER_ENTRY_SIGNATURE)

typedef struct {
  UINTN Signature;
  LIST_ENTRY MemberLink;
  DFCI_SETTING_PROVIDER_LIST_ENTRY *PList;
} DFCI_MEMBER_LIST_ENTRY;

extern LIST_ENTRY  mProviderList;         // Head of a list of DFCI_SETTING_PROVIDER_LIST_ENTRYs
extern LIST_ENTRY  mGroupList;            // Head of a list of DFCI_GROUP_PROVIDER_LIST_ENTRYs

extern DFCI_SETTING_ACCESS_PROTOCOL             mSystemSettingAccessProtocol;
extern DFCI_APPLY_PACKET_PROTOCOL               mApplySettingsProtocol;

//
// Internal Data struct
//
typedef struct {
  UINT32 CurrentVersion;
  UINT32 LSV;
  EFI_TIME CreatedOn;
  BOOLEAN Modified;
} DFCI_SETTING_INTERNAL_DATA;

EFI_STATUS
EFIAPI
RegisterProvider (
  IN DFCI_SETTING_PROVIDER_SUPPORT_PROTOCOL   *This,
  IN DFCI_SETTING_PROVIDER                    *Provider
  );

DFCI_SETTING_PROVIDER*
FindProviderById (
  DFCI_SETTING_ID_STRING Id
  );

DFCI_GROUP_LIST_ENTRY *
FindGroup (DFCI_SETTING_ID_STRING Id
  );

/**
Function sets the providers to default for
any provider that contains the FilterFlag in its flags
**/
EFI_STATUS
EFIAPI
ResetAllProvidersToDefaultsWithMatchingFlags(
  DFCI_SETTING_FLAGS FilterFlag);

VOID
DebugPrintProviderList();

VOID
DebugPrintGroups();

EFI_STATUS
RegisterSettingToGroup (
  IN DFCI_SETTING_PROVIDER_LIST_ENTRY *PList
  );

VOID
EFIAPI
ClearCacheOfCurrentSettings();

EFI_STATUS
EFIAPI
PopulateCurrentSettingsIfNeeded();

CHAR8*
ProviderValueAsAscii(DFCI_SETTING_PROVIDER *Provider, BOOLEAN Current);

EFI_STATUS
EFIAPI
SetProviderValueFromAscii(
  IN CONST DFCI_SETTING_PROVIDER *Provider,
  IN CONST CHAR8* Value,
  IN CONST DFCI_AUTH_TOKEN *AuthToken,
  IN OUT DFCI_SETTING_FLAGS *Flags
  );

EFI_STATUS
EFIAPI
SetSettingFromAscii(
  IN CONST CHAR8*  Id,
  IN CONST CHAR8*  Value,
  IN CONST DFCI_AUTH_TOKEN *AuthToken,
  IN OUT DFCI_SETTING_FLAGS *Flags);

EFI_STATUS
EFIAPI
ApplyNewSettingsPacket (
    IN CONST DFCI_APPLY_PACKET_PROTOCOL *This,
    IN       DFCI_INTERNAL_PACKET       *ApplyPacket
  );

EFI_STATUS
EFIAPI
SetSettingsResponse(
  IN  CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
  IN        DFCI_INTERNAL_PACKET        *Data
  );

EFI_STATUS
EFIAPI
SettingsLKG_Handler(
    IN  CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
    IN        DFCI_INTERNAL_PACKET        *ApplyPacket,
    IN        UINT8                        Operation
  );

/**
Pass thru function for using the Auth Protocol to get auth and token

@param[in]  SignedData      - Pointer to signed data
@param[in]  SignedDataLen   - Length of signed data
@param[in]  Signature       - Pointer to WIN_CERT that contains the signature
@param[in,out] AuthToken - returned Auth Token.  Caller must allocate.  data only valid if success.

**/
EFI_STATUS
EFIAPI
CheckAuthAndGetToken(
  IN     UINT8           *SignedData,
  IN     UINTN            SignedDataLen,
  IN     WIN_CERTIFICATE *Signature,
  IN OUT DFCI_AUTH_TOKEN  *AuthToken
  );

/**
Pass thru function for using the Auth Protocol to dispose of an auth token
so it can no longer be used in the system.

@param[in]  AuthToken - Pointer to auth token to dispose of
**/
EFI_STATUS
EFIAPI
AuthTokenDispose(
  IN DFCI_AUTH_TOKEN  *AuthToken
  );

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
EFI_STATUS
EFIAPI
SystemSettingAccessSet (
  IN  CONST DFCI_SETTING_ACCESS_PROTOCOL    *This,
  IN  DFCI_SETTING_ID_STRING                 Id,
  IN  CONST DFCI_AUTH_TOKEN                 *AuthToken,
  IN  DFCI_SETTING_TYPE                      Type,
  IN  UINTN                                  ValueSize,
  IN  CONST VOID                            *Value,
  IN OUT DFCI_SETTING_FLAGS                 *Flags
  );


/*
Get a single setting

@param[in] This:        Access Protocol
@param[in] Id:          Setting ID to Get
@param[in] AuthToken:   An optional auth token* to use to check permission of setting.  This auth token will be validated
to check permissions for changing the setting which will be reported in flags if valid.
@param[in] Type:        Type that caller expects this setting to be.
@param[out] Value:      A pointer to a datatype defined by the Type for this setting.
@param[IN OUT] Flags    Optional Informational flags passed back from the Get operation.  If the Auth Token is valid write access will be set in
flags for the given auth.

@retval EFI_SUCCESS if setting could be set.  Check flags for other info (reset required, etc)
@retval Error - couldn't get setting.

*/
EFI_STATUS
EFIAPI
SystemSettingAccessGet (
  IN  CONST DFCI_SETTING_ACCESS_PROTOCOL *This,
  IN  DFCI_SETTING_ID_STRING              Id,
  IN  CONST DFCI_AUTH_TOKEN              *AuthToken OPTIONAL,
  IN  DFCI_SETTING_TYPE                   Type,
  IN  OUT UINTN                          *ValueSize,
  OUT VOID                               *Value,
  IN OUT DFCI_SETTING_FLAGS              *Flags OPTIONAL
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
EFI_STATUS
EFIAPI
SystemSettingsAccessReset (
  IN  CONST DFCI_SETTING_ACCESS_PROTOCOL *This,
  IN  CONST DFCI_AUTH_TOKEN              *AuthToken
  );


EFI_STATUS
EFIAPI
SystemSettingPermissionGetPermission (
  IN  CONST DFCI_SETTING_PERMISSIONS_PROTOCOL *This,
  IN  DFCI_SETTING_ID_STRING                   Id,
  OUT DFCI_PERMISSION_MASK                    *PermissionMask
  );

EFI_STATUS
EFIAPI
SystemSettingPermissionResetPermission (
  IN  CONST DFCI_SETTING_PERMISSIONS_PROTOCOL *This,
  IN  CONST DFCI_AUTH_TOKEN                   *AuthToken OPTIONAL
  );

EFI_STATUS
EFIAPI
SystemSettingPermissionIdentityChange (
  IN  CONST DFCI_SETTING_PERMISSIONS_PROTOCOL *This,
  IN  CONST DFCI_AUTH_TOKEN                   *AuthToken,
  IN        DFCI_IDENTITY_ID                   CertIdentity,
  IN        BOOLEAN                            Enroll
  );

//functions to support internal data store management
EFI_STATUS
EFIAPI
SMID_SaveToFlash(IN DFCI_SETTING_INTERNAL_DATA *InternalData);

EFI_STATUS
EFIAPI
SMID_LoadFromFlash(IN DFCI_SETTING_INTERNAL_DATA **InternalData);

EFI_STATUS
EFIAPI
SMID_InitInternalData(IN DFCI_SETTING_INTERNAL_DATA **InternalData);

EFI_STATUS
EFIAPI
SMID_ResetInFlash();

#endif // SETTINGS_MANAGER_H