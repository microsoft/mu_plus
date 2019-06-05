/**@file
  DfciSettingPermission.c

Main file for the library.  Implements the library class routines as well as constructor.

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

#include "DfciSettingPermission.h"

DFCI_PERMISSION_STORE        *mPermStore = NULL;
DFCI_AUTHENTICATION_PROTOCOL *mAuthenticationProtocol = NULL;

// Apply Permissions Protocol
DFCI_APPLY_PACKET_PROTOCOL mApplyPermissionsProtocol = {
       DFCI_APPLY_PACKET_SIGNATURE,
       DFCI_APPLY_PACKET_VERSION,
       {
           0,
           0,
           0
       },
       ApplyNewPermissionsPacket,
       SetPermissionsResponse,
       LKG_Handler
};

EFI_STATUS
EFIAPI
ResetPermissionsToDefault(
IN CONST DFCI_AUTH_TOKEN *AuthToken OPTIONAL
)
{
  EFI_STATUS Status;
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
    Status = HasRecoveryPermission(AuthToken, &CanChangeRecovery);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to get Recovery Permission. Status = %r\n", __FUNCTION__, Status));
      return Status;
    }

    if (!CanChangeRecovery)
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
IN  DFCI_SETTING_ID_STRING       GroupId     OPTIONAL,
IN  CONST DFCI_AUTH_TOKEN       *AuthToken,
OUT BOOLEAN                     *Result
)
{
  EFI_STATUS Status;
  DFCI_PERMISSION_MASK PMask = 0;
  DFCI_IDENTITY_PROPERTIES Properties;
  DFCI_PERMISSION_ENTRY *Temp = NULL;
  BOOLEAN GroupAuthStatus = FALSE;

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

  if (NULL != GroupId)
  {
    //
    // Auth for members of a group depend on there being an explicit permission.
    // If there is no explicit permission, normal write permissions are checked.  If
    // there is an explicit permission entry for the group, the group permission
    // supersedes the individual permission.r
    //
    Temp = FindPermissionEntry(mPermStore, GroupId, NULL, NULL);
    if (NULL != Temp)
    {
      // Check Auth for the group id (Recursive Call)
      Status = HasWritePermissions(GroupId, NULL, AuthToken, &GroupAuthStatus);
      if (!EFI_ERROR(Status))
      {
        if (!GroupAuthStatus) {
          *Result = GroupAuthStatus;
          return EFI_SUCCESS;
        }
      }
      else
      {
        DEBUG((DEBUG_ERROR, "%a - HasWritePermissions returned an error %r\n", __FUNCTION__, Status));
        // Fall through to setting permission
      }
    }
  }

  //1. Get Identity from Auth Token
  Status = mAuthenticationProtocol->GetIdentityProperties(mAuthenticationProtocol, AuthToken, &Properties);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get properties for auth token %r\n", __FUNCTION__, Status));
    return Status;
  }

  //2. set to default.
  PMask = mPermStore->DefaultPMask;

  //3. Set PMask to specific value if in list
  Temp = FindPermissionEntry(mPermStore, SettingId, NULL, NULL);
  if (Temp != NULL)
  {
    DEBUG((DEBUG_INFO, "%a - Found Specific Permission for %a (0x%x), (0x%x)\n", __FUNCTION__, SettingId, Temp->PMask, Properties.Identity));
    PMask = Temp->PMask;
  } else {
      DEBUG((DEBUG_INFO, "%a - Using default permission %a (0x%x), (0x%x)\n", __FUNCTION__, SettingId, PMask, Properties.Identity));
  }

  //3. Permission and Identity use the same bits so they can be logically anded together
  *Result = (PMask & Properties.Identity)? TRUE: FALSE;

  return EFI_SUCCESS;
}


/**
 Check if the current AuthToken has recovery permissions
 **/
EFI_STATUS
EFIAPI
HasRecoveryPermission (
  IN  CONST DFCI_AUTH_TOKEN       *AuthToken,
  OUT BOOLEAN                     *Result
  ) {

  BOOLEAN CanChangeRecovery = FALSE;
  EFI_STATUS    Status;

  //Check parameters
  if (AuthToken == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  // There are two recovery permissions.  Dfci Recovery, which is used by SEMM, and Ztd Recovery.  The
  // Permission manager own these permissions and one or the other is in control based on the enrolled owner.  If
  // the owner certificate was enrolled and signed by the Ztd key, the DFCI recovery is disabled and ZTD recovery
  // is enabled.

  Status = HasWritePermissions(DFCI_SETTING_ID__DFCI_RECOVERY, NULL, AuthToken, &CanChangeRecovery);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get Write Permission for DFCI Recovery. Status = %r\n", __FUNCTION__, Status));
    return Status;
  }

  // If CanChangeRecovery is false, check for ZTD_RECOVERY
  if (!CanChangeRecovery)
  {
    Status = HasWritePermissions(DFCI_SETTING_ID__ZTD_RECOVERY, NULL, AuthToken, &CanChangeRecovery);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to get Write Permission for ZTD Recovery. Status = %r\n", __FUNCTION__, Status));
      return Status;
    }
  }
  DEBUG((DEBUG_INFO, "%a - Recovery Permission Policy=%d\n", __FUNCTION__, CanChangeRecovery));

  *Result = CanChangeRecovery;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
QueryPermission(
IN  DFCI_SETTING_ID_STRING  SettingId,
OUT DFCI_PERMISSION_MASK   *Permissions
)
{
  DFCI_PERMISSION_MASK PMask = 0;
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

  PMask = mPermStore->DefaultPMask;

  //3. Set PMask to specific value if in list
  Temp = FindPermissionEntry(mPermStore, SettingId, NULL, NULL);
  if (Temp != NULL)
  {
    PMask = Temp->PMask;
    DEBUG((DEBUG_INFO, "%a - Found Specific Permission for %a (0x%x)\n", __FUNCTION__, SettingId, PMask));
  } else {
    DEBUG((DEBUG_INFO, "%a - Using default permission %a (0x%x)\n", __FUNCTION__, SettingId, PMask));
  }

  *Permissions = PMask;
  return EFI_SUCCESS;
}

/**
 *
 * IdentityChange notification
 *
 * @param AuthToken
 * @param CertIdentity
 * @param Enroll
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
IdentityChange (
    IN  CONST DFCI_AUTH_TOKEN     *AuthToken,
    IN        DFCI_IDENTITY_ID     CertIdentity,
    IN        BOOLEAN              Enroll
) {
    EFI_STATUS                   Status;
    DFCI_IDENTITY_PROPERTIES     Properties;


    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (AuthToken == NULL)
    {
      DEBUG((DEBUG_ERROR, "%a: AuthToken is NULL.\n", __FUNCTION__));
      return EFI_INVALID_PARAMETER;
    }

    // 1. If the action is not Enroll, do nothing as owner unenroll has alreay reset permissions.
    if (!Enroll)
    {
      return EFI_SUCCESS;
    }

    if (mAuthenticationProtocol == NULL)
    {
      DEBUG((DEBUG_ERROR, "%a: Trying to access Auth Protocol too early.\n", __FUNCTION__));
      return EFI_NOT_READY;
    }

    // 2. Get Identity from Auth Token
    Status = mAuthenticationProtocol->GetIdentityProperties(mAuthenticationProtocol, AuthToken, &Properties);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a: Faled to get properties for auth token %r\n", __FUNCTION__, Status));
      return EFI_ACCESS_DENIED;
    }
    DEBUG((DEBUG_INFO, "%a: Signer=0x%2.2x, Identity=0x%2.2x, Enroll=%d\n", __FUNCTION__, Properties.Identity, CertIdentity, Enroll));

    // 3. See if Owner is being enrolled
    if (CertIdentity == DFCI_IDENTITY_SIGNER_OWNER)
    {
      //  Disallow any future ZTD signing while an owner is applied.
      Status  = AddRequiredPermissionEntry (mPermStore, DFCI_SETTING_ID__ZTD_KEY,      DFCI_IDENTITY_INVALID,    DFCI_PERMISSION_MASK__NONE);
    }

    // 4. When an Owner is enrolled and the signer is ZTD:
    if (Properties.Identity == DFCI_IDENTITY_SIGNER_ZTD)
    {
      //    a. Allow ZTD to UnEnroll.
      //    b. Allow ZTD to use hard reset Recovery
      //    c. Remove SEMM recovery permission
      Status |= AddRequiredPermissionEntry (mPermStore, DFCI_SETTING_ID__ZTD_RECOVERY, DFCI_IDENTITY_SIGNER_ZTD, DFCI_PERMISSION_MASK__NONE);
      Status |= AddRequiredPermissionEntry (mPermStore, DFCI_SETTING_ID__ZTD_UNENROLL, DFCI_IDENTITY_SIGNER_ZTD, DFCI_PERMISSION_MASK__NONE);
      Status |= AddRequiredPermissionEntry (mPermStore, DFCI_SETTING_ID__DFCI_RECOVERY, DFCI_PERMISSION_MASK__NONE, DFCI_PERMISSION_MASK__NONE);
      return EFI_SUCCESS;
    }

    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a: Failed to reset required permissions. Status = %r\n", __FUNCTION__, Status));
      return Status;
    }

    DEBUG((DEBUG_INFO, "%a: Updated permissions\n", __FUNCTION__));

    return Status;
}

/**
 * Get AuthenticationProtocol when published
 *
 * @param Event
 * @param Context
 *
 * @return VOID EFIAPI
 */
VOID
EFIAPI
CheckForAuthenticationProtocol (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
  {
  EFI_STATUS Status;
  DFCI_IDENTITY_MASK  IdMask;           // Identities installed

  if (mAuthenticationProtocol == NULL) {
    Status = gBS->LocateProtocol(&gDfciAuthenticationProtocolGuid, NULL, (VOID **)&mAuthenticationProtocol);
    if (EFI_ERROR(Status))
    {
      //this happens at least once
      //on register
      return;
    }
    DEBUG((DEBUG_INFO,"Located Authentication Protocol after Notify. Code=%r\n",Status));

    Status =  mAuthenticationProtocol->GetEnrolledIdentities ( mAuthenticationProtocol, &IdMask);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "%a: Failed to get owner ids. %r\n", __FUNCTION__, Status));
    } else {

      Status = EFI_NOT_FOUND;
      if (IS_OWNER_IDENTITY_ENROLLED(IdMask))
      {
        //Load Permission Store
        Status = LoadFromFlash(&mPermStore);
      }
      else
      {
        DEBUG((DEBUG_ERROR,"No Owner Identity installed, re-initializing Permissions. Code=%r\n",Status));
      }
      if (EFI_ERROR(Status))
      {
        if (Status != EFI_NOT_FOUND)
        {
          DEBUG((DEBUG_ERROR, "%a - Failed to load Permission Store. %r\n", __FUNCTION__, Status));
        }

        //If load failed, or no Owner Identity was installed, init store
        Status = InitPermStore(&mPermStore);
        if (EFI_ERROR(Status))
        {
          DEBUG((DEBUG_ERROR, "%a - Couldn't Init PMask Store %r\n", __FUNCTION__, Status));
          mPermStore = NULL;
        }
        else
        {
          //save the newly initialized permissions
          Status = SaveToFlash(mPermStore);
        }
      }
      if (mPermStore != NULL)
      {
        DebugPrintPermissionStore(mPermStore);

        PopulateCurrentPermissions(FALSE);   // If no CurrentPermissions, publish the default.
      }
    }
  }
  gBS->CloseEvent(Event);  //close the event so we don't get signalled again
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
  EFI_EVENT  InitEvent;
  VOID      *InitRegistration;

  InitEvent = EfiCreateProtocolNotifyEvent(
    &gDfciAuthenticationProtocolGuid,
    TPL_CALLBACK,
    CheckForAuthenticationProtocol,
    NULL,
    &InitRegistration
    );

  //Install Permission Apply Protocol
  Status = gBS->InstallMultipleProtocolInterfaces(
    &ImageHandle,
    &gDfciApplyPermissionsProtocolGuid,
    &mApplyPermissionsProtocol,
    NULL
    );

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to Install DFCI Permissions Protocol. %r\n", Status));
  }

  return Status;
}
