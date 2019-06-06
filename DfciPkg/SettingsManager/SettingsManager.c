/**@file
SettingsManager.c

Implements the SettingAccess Provider

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

#include "SettingsManager.h"

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
  )
{
  DFCI_SETTING_PROVIDER *prov;
  DFCI_SETTING_ID_STRING  GroupId;
  DFCI_GROUP_LIST_ENTRY *Group;
  DFCI_MEMBER_LIST_ENTRY *Member;
  DFCI_SETTING_PROVIDER_LIST_ENTRY *Provider;
  LIST_ENTRY *Link;
  EFI_STATUS ReturnStatus;
  EFI_STATUS Status;
  BOOLEAN AuthStatus = FALSE;
  STATIC BOOLEAN SetRecurse = FALSE;

  //Check parameters
  if ((This == NULL) || (Value == NULL) || (AuthToken == NULL) || (Flags == NULL) | (Id == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //Get provider and verify type
  prov = FindProviderById(Id);

  if (prov == NULL)
  {
    if (SetRecurse) {
      DEBUG((DEBUG_ERROR,"%a: Unexpected recursion.\n"));
      ASSERT(!SetRecurse);
      return EFI_UNSUPPORTED;
    }

    // May be group setting
    Group = FindGroup (Id);
    if (Group == NULL)
    {
      DEBUG((DEBUG_ERROR, "%a - Requested ID (%a) not found.\n", __FUNCTION__, Id));
      return EFI_NOT_FOUND;
    }

    ReturnStatus = EFI_SUCCESS;
    for (Link = GetFirstNode (&Group->MemberHead)
         ; !IsNull (&Group->MemberHead, Link)
         ; Link = GetNextNode (&Group->MemberHead, Link)
         )
    {
      Member = MEMBER_LIST_ENTRY_FROM_MEMBER_LINK (Link);
      SetRecurse = TRUE;
      Status = SystemSettingAccessSet (This,
                                       Member->PList->Provider.Id,
                                       AuthToken,
                                       Type,
                                       ValueSize,
                                       Value,
                                       Flags);
      SetRecurse = FALSE;
      if (EFI_ERROR(Status)) {
        ReturnStatus = Status;
      }
    }

    return ReturnStatus;
  }

  Provider = PROV_LIST_ENTRY_FROM_PROVIDER (prov);
  if (NULL != Provider->Group) {
    GroupId = Provider->Group->GroupId;
  } else {
    GroupId = NULL;
  }

  //Check Auth for the setting Id.
  Status = HasWritePermissions(Id, GroupId, AuthToken, &AuthStatus);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - HasWritePermissions returned an error %r\n", __FUNCTION__, Status));
    return Status;
  }

  //if no write access to group ID return access denied
  if (!AuthStatus)
  {
    DEBUG((DEBUG_INFO, "%a - No Permission to write setting %a\n", __FUNCTION__, Id));
    return EFI_ACCESS_DENIED;
  }

  if (Type != prov->Type)
  {
    DEBUG((DEBUG_ERROR, "Caller supplied type (0x%X) and provider type (0x%X) don't match\n", Type, prov->Type));
    ASSERT(Type == prov->Type);
    return EFI_INVALID_PARAMETER;
  }

  //Set the current setting to the new value.
  Status = prov->SetSettingValue(prov, ValueSize, Value, Flags);
  if (EFI_ERROR(Status))
  {
    if (Status == EFI_BAD_BUFFER_SIZE) {
      DEBUG((DEBUG_ERROR, "%a: Bad size requested for setting provider!\n", __FUNCTION__));
      ASSERT_EFI_ERROR( Status );
    }
    DEBUG((DEBUG_ERROR, "Failed to Set Settings\n"));
    return Status;
  }

  if ((*Flags & DFCI_SETTING_FLAGS_OUT_ALREADY_SET) == 0)
  {
    //Status was good and flags don't indicate that value was already set.
    //need to clear the cache
    ClearCacheOfCurrentSettings();
  }

  return Status;
}

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
  IN  CONST DFCI_AUTH_TOKEN              *AuthToken, OPTIONAL
  IN  DFCI_SETTING_TYPE                   Type,
  IN OUT UINTN                           *ValueSize,
  OUT VOID                               *Value,
  IN OUT DFCI_SETTING_FLAGS              *Flags OPTIONAL
  )
{
  DFCI_SETTING_PROVIDER *prov = NULL;
  DFCI_GROUP_LIST_ENTRY *Group;
  DFCI_SETTING_ID_STRING  GroupId;
  DFCI_MEMBER_LIST_ENTRY *Member;
  DFCI_SETTING_PROVIDER_LIST_ENTRY *Provider;
  LIST_ENTRY *Link;
  EFI_STATUS ReturnStatus;
  EFI_STATUS Status = EFI_SUCCESS;
  STATIC BOOLEAN GetRecurse = FALSE;
  UINT8 LocalValue;
  UINT8 MasterValue;
  UINTN LocalSize;

  //Check parameters
  if ((This == NULL) || (Value == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //Get provider and verify type
  prov = FindProviderById(Id);

  if (prov == NULL)
  {
    if (GetRecurse) {
      DEBUG((DEBUG_ERROR,"%a: Unexpected recursion.\n"));
      ASSERT(!GetRecurse);
      return EFI_UNSUPPORTED;
    }

    // May be group setting
    Group = FindGroup (Id);
    if (Group == NULL)
    {
      DEBUG((DEBUG_ERROR, "%a - Requested ID (%a) not found.\n", __FUNCTION__, Id));
      return EFI_NOT_FOUND;
    }

    //
    // Group Settings are limited to DFCI_SETTING_TYPE_ENABLE
    //
    if (DFCI_SETTING_TYPE_ENABLE != Type) {
      DEBUG((DEBUG_ERROR, "%a - Requested ID (%a) type not Enable.\n", __FUNCTION__, Id));
      return EFI_UNSUPPORTED;
    }

    if (*ValueSize < 1) {
      *ValueSize = sizeof(UINT8);
      return EFI_BUFFER_TOO_SMALL;
    }

    ReturnStatus = EFI_SUCCESS;
    MasterValue = 0x80;  // Some value not ENABLE_TRUE or ENABLE_FALSE
    for (Link = GetFirstNode (&Group->MemberHead)
         ; !IsNull (&Group->MemberHead, Link)
         ; Link = GetNextNode (&Group->MemberHead, Link)
         )
    {
      Member = MEMBER_LIST_ENTRY_FROM_MEMBER_LINK (Link);
      if (Member->PList->Provider.Type != Type) {
        DEBUG((DEBUG_ERROR, "%a: Only type ENABLE is allowed to be a group member\n", __FUNCTION__));
        ReturnStatus = EFI_UNSUPPORTED;
        continue;
      }
      LocalSize = sizeof(LocalValue);
      GetRecurse= TRUE;
      Status = SystemSettingAccessGet (This,
                                       Member->PList->Provider.Id,
                                       AuthToken,
                                       Type,
                                      &LocalSize,
                                      &LocalValue,
                                       Flags);
      GetRecurse = FALSE;
      if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: Unexpected return from AccessGet. Code=%r\n", __FUNCTION__, Status));
        ReturnStatus = Status;
        continue;
      }

      DEBUG((DEBUG_INFO,"Value of %a is %x\n", Member->PList->Provider.Id, (UINTN) LocalValue));

      if (0x80 == MasterValue) {
        MasterValue = LocalValue;
      } else {
        if (MasterValue != LocalValue) {
          MasterValue = ENABLE_INCONSISTENT;
          break;
        }
      }
    }

    // On Success, set *Value and *ValueSize
    // On Buffer Too Small, only set *ValueSize
    // All other errors do not alter *Value or *ValueSize
    switch (ReturnStatus) {
      case EFI_SUCCESS:
        *((UINT8 *)Value) = MasterValue;
      case EFI_BUFFER_TOO_SMALL:
        *ValueSize = sizeof(UINT8);
      break;
    }

    return ReturnStatus;
  }

  Provider = PROV_LIST_ENTRY_FROM_PROVIDER (prov);
  if (NULL != Provider->Group) {
    GroupId = Provider->Group->GroupId;
  } else {
    GroupId = NULL;
  }

  if (Type != prov->Type)
  {
    DEBUG((DEBUG_ERROR, "Caller supplied type (0x%X) and provider type (0x%X) don't match\n", Type, prov->Type));
    ASSERT(Type == prov->Type);
    return EFI_INVALID_PARAMETER;
  }

  if (Flags != NULL)
  {
    //return the provider flags
    *Flags = prov->Flags;
  }

  //
  // Go check the permission
  //
  if ((AuthToken != NULL) && (Flags != NULL))
  {
    BOOLEAN AuthStatus = FALSE;
    Status = HasWritePermissions(Id, GroupId, AuthToken, &AuthStatus);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_INFO, "%a - Failed to get Write Permission for Id %a Status %r\n", __FUNCTION__, Id, Status));
      AuthStatus = FALSE;
    }
    if (AuthStatus)
    {
      *Flags |= DFCI_SETTING_FLAGS_OUT_WRITE_ACCESS;  //add write access if AuthStatus is TRUE
    }
  }
  return prov->GetSettingValue(prov, ValueSize, Value);
}

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
  )
{
  BOOLEAN CanUnenroll = FALSE;
  EFI_STATUS Status;

  //Check parameters
  if ((This == NULL) ||(AuthToken == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = HasUnenrollPermission ( AuthToken, &CanUnenroll);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get recovery permission. Status = %r\n", __FUNCTION__, Status));
    return Status;
  }

  if (!CanUnenroll)
  {
    DEBUG((DEBUG_INFO, "%a - Auth Token doesn't have permission to reset settings\n", __FUNCTION__));
    return EFI_ACCESS_DENIED;
  }

  Status = ResetAllProvidersToDefaultsWithMatchingFlags(DFCI_SETTING_FLAGS_NO_PREBOOT_UI);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to reset all settings to defaults. Status = %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR(Status); //if cleanup fails on production system nothing we can do...keep going
  }

  Status = SMID_ResetInFlash();  //clear the internal storage
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to Reset Settings Internal Data Status = %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR(Status);  //if cleanup fails on production system nothing we can do...keep going
  }

  ClearCacheOfCurrentSettings();  //clear current settings XML
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SystemSettingPermissionGetPermission (
  IN  CONST DFCI_SETTING_PERMISSIONS_PROTOCOL *This,
  IN  DFCI_SETTING_ID_STRING                   Id,
  OUT DFCI_PERMISSION_MASK                    *PermissionMask
  )
{
  if ((This == NULL) || (PermissionMask == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  return QueryPermission(Id, PermissionMask);

}

EFI_STATUS
EFIAPI
SystemSettingPermissionResetPermission (
  IN  CONST DFCI_SETTING_PERMISSIONS_PROTOCOL *This,
  IN  CONST DFCI_AUTH_TOKEN                   *AuthToken
  )
{
  EFI_STATUS Status;
  if ((This == NULL) || (AuthToken == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = ResetPermissionsToDefault(AuthToken);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to Reset Permissions Status = %r\n", __FUNCTION__, Status));
  }

  return Status;
}

EFI_STATUS
EFIAPI
SystemSettingPermissionIdentityChange (
  IN  CONST DFCI_SETTING_PERMISSIONS_PROTOCOL *This,
  IN  CONST DFCI_AUTH_TOKEN                   *AuthToken,
  IN        DFCI_IDENTITY_ID                   CertIdentity,
  IN        BOOLEAN                            Enroll
  )
{
  EFI_STATUS Status;
  if ((This == NULL) || (AuthToken == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = IdentityChange(AuthToken, CertIdentity, Enroll);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to Reset Permissions. Status = %r\n", __FUNCTION__, Status));
  }

  return Status;
}
