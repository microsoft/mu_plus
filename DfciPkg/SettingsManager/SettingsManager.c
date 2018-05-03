
#include "SettingsManager.h"
#include <Library/PrintLib.h>



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
  DFCI_SETTING_PROVIDER *prov = NULL;
  EFI_STATUS Status;
  BOOLEAN AuthStatus = FALSE;
  //Check parameters
  if ((This == NULL) || (Value == NULL) || (AuthToken == NULL) || (Flags == NULL) | (Id == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //Get provider and verify type
  prov = FindProviderById(Id);

  if (prov == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Requested ID (%a) not found.\n", __FUNCTION__, Id));
    return EFI_NOT_FOUND;
  }

  if (Type != prov->Type)
  {
    DEBUG((DEBUG_ERROR, "Caller supplied type (0x%X) and provider type (0x%X) don't match\n", Type, prov->Type));
    ASSERT(Type == prov->Type);
    return EFI_INVALID_PARAMETER;
  }

  //Check Auth
  Status = HasWritePermissions(Id, AuthToken, &AuthStatus);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - HasWritePermissions returned an error %r\n", __FUNCTION__, Status));
    return Status;
  }

  //if no write access return access denied
  if (!AuthStatus)
  {
    DEBUG((DEBUG_INFO, "%a - No Permission to write setting %a\n", __FUNCTION__, Id));
    return EFI_ACCESS_DENIED;
  }
  //Set
  Status = prov->SetSettingValue(prov, ValueSize, Value, Flags);
  if (EFI_ERROR(Status))
  {
    if (Status == EFI_BAD_BUFFER_SIZE) {
      DEBUG(( DEBUG_ERROR, __FUNCTION__" - Bad size requested for setting provider!\n" ));
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
  EFI_STATUS Status = EFI_SUCCESS;
  //Check parameters
  if ((This == NULL) || (Value == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //Get provider and verify type
  prov = FindProviderById(Id);

  if (prov == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Requested ID (%a) not found.\n", __FUNCTION__, Id));
    return EFI_NOT_FOUND;
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
  // Go check the permssion
  //
  if ((AuthToken != NULL) && (Flags != NULL))
  {
    BOOLEAN AuthStatus = FALSE;
    Status = HasWritePermissions(Id, AuthToken, &AuthStatus);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to get Write Permission for Id %a Status %r\n", __FUNCTION__, Id, Status));
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
  BOOLEAN CanChange = FALSE;
  BOOLEAN CanChangeRecovery = FALSE;
  EFI_STATUS Status;

  //Check parameters
  if ((This == NULL) ||(AuthToken == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //Check permission for DFCI Recovery or Owner Key
  Status = HasWritePermissions(DFCI_SETTING_ID__OWNER_KEY, AuthToken, &CanChange);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get Write Permission for Owner Key. Status = %r\n", __FUNCTION__, Status));
    return Status;
  }

  Status = HasWritePermissions(DFCI_SETTING_ID__DFCI_RECOVERY, AuthToken, &CanChangeRecovery);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get Write Permission for DFCI Recovery. Status = %r\n", __FUNCTION__, Status));
    return Status;
  }

  if ((CanChange == FALSE) && (CanChangeRecovery == FALSE))
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
