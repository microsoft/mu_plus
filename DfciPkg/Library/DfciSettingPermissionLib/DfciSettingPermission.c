/** @file
DfciSettingPermission

Main file for the lib.  Implements the library class routines as well as constructor.  

Copyright (c) 2015, Microsoft Corporation. 

**/
#include "DfciSettingPermission.h"


DFCI_PERMISSION_STORE        *mPermStore = NULL;
DFCI_AUTHENTICATION_PROTOCOL *mAuthenticationProtocol = NULL;
EFI_EVENT                     mAuthInstallEvent;
VOID                         *mAuthInstallEventRegistration = NULL;


EFI_STATUS
EFIAPI
ResetPermissionsToDefault(
IN CONST DFCI_AUTH_TOKEN *AuthToken OPTIONAL
)
{
  EFI_STATUS Status;
  BOOLEAN CanChange = FALSE;
  BOOLEAN CanChangeRecovery = FALSE;

  if (AuthToken != NULL)
  {
    if (mAuthenticationProtocol == NULL)
    {
      DEBUG((DEBUG_ERROR, "%a - Trying to access Auth Protocol too early.\n", __FUNCTION__));
      return EFI_NOT_READY;
    }

    //User is trying to reset.  Check if auth token is valid for this operation.   
    // Permission is based on who can change the Owner Cert and/or who can do recovery.
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
      DEBUG((DEBUG_INFO, "%a - Auth Token doesn't have permission to clear permissions\n", __FUNCTION__));
      return EFI_ACCESS_DENIED;
    }
  }
 
  DEBUG((DEBUG_INFO, "%a - Auth Token good.  Lets clear the permissions.\n", __FUNCTION__));

  // 1. Free existing PermissionStore
  if (mPermStore)
  {
    FreePermissionStore(mPermStore);
    mPermStore = NULL;
  }
  
  // 2. Set it to defaults which is all access to all settings
  Status = InitPermStore(&mPermStore);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Couldn't Init Perm Store %r\n", __FUNCTION__, Status));
    mPermStore = NULL;
    return Status;  //failed to load the lib
  }
  else
  {
    //save the newly initialized permissions
    Status = SaveToFlash(mPermStore);
  }
  return Status;
}



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
IN  CONST DFCI_AUTH_TOKEN       *AuthToken,
OUT BOOLEAN                     *Result
)
{
  EFI_STATUS Status;
  DFCI_PERMISSION_MASK Perm = 0;
  DFCI_IDENTITY_PROPERTIES Properties;
  DFCI_PERMISSION_ENTRY *Temp = NULL;

  if ((AuthToken == NULL) || (Result == NULL) || (SettingId == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (!mPermStore)
  {
    return EFI_NOT_READY;
  }

  if (mAuthenticationProtocol == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Trying to access Auth Protocol too early.\n", __FUNCTION__));
    return EFI_NOT_READY;
  }

  //1. Get Identity from Auth Token
  Status = mAuthenticationProtocol->GetIdentityProperties(mAuthenticationProtocol, AuthToken, &Properties);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Faled to get properties for auth token %r\n", __FUNCTION__, Status));
    return Status;
  }

  
  //2. set to default. 
  Perm = mPermStore->Default;

  //3. Set Perm to specific value if in list
  Temp = FindPermissionEntry(mPermStore, SettingId);
  if (Temp != NULL)
  {
    DEBUG((DEBUG_INFO, "%a - Found Specific Permission for %a\n", __FUNCTION__, SettingId));
    Perm = Temp->Perm;
  }

  //3. Permission and Identity use the same bits so they can be logically anded together
  *Result = (Perm & Properties.Identity)? TRUE: FALSE;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
QueryPermission(
IN  DFCI_SETTING_ID_STRING  SettingId,
OUT DFCI_PERMISSION_MASK   *Permissions
)
{
  DFCI_PERMISSION_MASK Perm = 0;
  DFCI_PERMISSION_ENTRY *Temp = NULL;
  if (Permissions == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  if (!mPermStore)
  {
    return EFI_NOT_READY;
  }

  if (SettingId == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  Perm = mPermStore->Default;

  //3. Set Perm to specific value if in list
  Temp = FindPermissionEntry(mPermStore, SettingId);
  if (Temp != NULL)
  {
    DEBUG((DEBUG_INFO, "%a - Found Specific Permission for %a\n", __FUNCTION__, SettingId));
    Perm = Temp->Perm;
  }

  *Permissions = Perm;
  return EFI_SUCCESS;
}


VOID
EFIAPI
CheckForPermissionUpdate(
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
  {
    DFCI_AUTHENTICATION_PROTOCOL *pro = NULL;
    VOID *AuthPendingProtocol = NULL;
    EFI_STATUS Status;

    Status = gBS->LocateProtocol(
      &gDfciAuthenticationProtocolGuid,
      NULL,
      (VOID **)&pro
      );

    if (EFI_ERROR(Status))
    {
      //this happens at least once 
      //on register
      return;
    }

    if (mAuthenticationProtocol == NULL)
    {
      mAuthenticationProtocol = pro;
    }

    gBS->CloseEvent(Event);  //close the event so we don't get signalled again

    if (!EFI_ERROR(gBS->LocateProtocol(&gDfciAuthenticationProvisioningPendingGuid, NULL, &AuthPendingProtocol)))
    {
      DEBUG((DEBUG_INFO, "%a - Auth Provisioning Pending Protocol Installed.  Skip Checking for Pending Updates\n", __FUNCTION__));
      return;
    }

    CheckForPendingPermissionChanges(); //Check for permission provisioning
}


//
//Constructor
//
EFI_STATUS
EFIAPI
DfciPermissionInit(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;

  //Load Permission Store
  Status = LoadFromFlash(&mPermStore);
  if (EFI_ERROR(Status))
  {
    if (Status != EFI_NOT_FOUND)
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to load Permission Store. %r\n", __FUNCTION__, Status));
    }

    //If load failed - init store 
    Status = InitPermStore(&mPermStore);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Couldn't Init Perm Store %r\n", __FUNCTION__, Status));
      mPermStore = NULL;
      return Status;  //failed to load the lib
    }
    else
    {
      //save the newly initialized permissions
      Status = SaveToFlash(mPermStore);
    }
  }

  //Register notify function for Auth Protocol installed. Auth Protocol will not be installed
  //provisioning is ready to be checked.
  mAuthInstallEvent = EfiCreateProtocolNotifyEvent(
    &gDfciAuthenticationProtocolGuid,
    TPL_CALLBACK,
    CheckForPermissionUpdate,
    NULL,
    &mAuthInstallEventRegistration
    );

  DebugPrintPermissionStore(mPermStore);
  return Status;
}
