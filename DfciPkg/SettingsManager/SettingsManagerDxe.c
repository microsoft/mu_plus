/*++ @file
    
    Copyright (C) 2014 Microsoft Corporation. All Rights Reserved. 

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    PARTICULAR PURPOSE.

--*/

#include "SettingsManager.h"

DFCI_SETTING_ACCESS_PROTOCOL            mSystemSettingAccessProtocol = { SystemSettingAccessSet, SystemSettingAccessGet, SystemSettingsAccessReset };
DFCI_SETTING_PROVIDER_SUPPORT_PROTOCOL  mProviderProtocol = { RegisterProvider };
DFCI_SETTING_PERMISSIONS_PROTOCOL       mPermissionProtocol = { SystemSettingPermissionGetPermission, SystemSettingPermissionResetPermission };
EFI_EVENT                               mSmDxeEvent;
EFI_EVENT                               mSmReadyToBootEvent;

DFCI_AUTHENTICATION_PROTOCOL *mAuthProtocol = NULL;

/**
Notify function for running and acting on the requests (input, debug, etc)

@param[in]  Event   The Event that is being processed.
@param[in]  Context The Event Context.

**/
VOID
EFIAPI
SettingManagerDxeEventNotify(
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  VOID *AuthPendingProtocol = NULL;
  EFI_STATUS Status;

  DEBUG_CODE_BEGIN();
  //print registered  on debug builds
  DebugPrintProviderList();
  DEBUG_CODE_END();

  gBS->CloseEvent(Event);


  //install setting access
  Status = gBS->InstallMultipleProtocolInterfaces(
    &Context,  //Image handle was stored as the context
    &gDfciSettingAccessProtocolGuid,
    &mSystemSettingAccessProtocol,
    NULL
    );

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to Install DFCI Settings Access Protocol. %r\n", Status));
  }


  if (!EFI_ERROR(gBS->LocateProtocol(&gDfciAuthenticationProvisioningPendingGuid, NULL, &AuthPendingProtocol)))
  {
    DEBUG((DEBUG_INFO, "%a - Auth Provisioning Pending Protocol Installed.  Skip Checking for Pending Updates\n", __FUNCTION__));
    return;
  }

  CheckForPendingUpdates();

  //Check for Settings Provisioning
  Status = PopulateCurrentSettingsIfNeeded();
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Populate Current Settings If Needed returned an error. %r\n", __FUNCTION__, Status));
  }
}

/**
Notify function for ReadyToBoot

@param[in]  Event   The Event that is being processed.
@param[in]  Context The Event Context.

**/
VOID
EFIAPI
SettingManagerReadyToBootEventNotify(
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  EFI_STATUS Status;

  gBS->CloseEvent(Event);

  //Check for Settings Provisioning
  Status = PopulateCurrentSettingsIfNeeded();
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Populate Current Settings If Needed returned an error. %r\n", __FUNCTION__, Status));
  }
}

/**
Pass thru function for using the Auth Protocol to get auth and token

@param[in]  SignedData      - Pointer to signed data
@param[in]  SignedDataLen   - Length of signed data
@param[in]  Signature       - Pointer to WIN_CERT_UEFI_GUID that contains the signature  
@param[in,out] AuthToken - returned Auth Token.  Caller must allocate.  data only valid if success. 

**/
EFI_STATUS
EFIAPI
CheckAuthAndGetToken(
  IN     UINT8           *SignedData,
  IN     UINTN           SignedDataLen,
  IN     WIN_CERTIFICATE *Signature,
  IN OUT DFCI_AUTH_TOKEN   *AuthToken
  )
{
  EFI_STATUS Status;
  //get mAuthProtocol 
  if (mAuthProtocol == NULL)
  {
    Status = gBS->LocateProtocol(
      &gDfciAuthenticationProtocolGuid,
      NULL,
      (VOID **)&mAuthProtocol
      );

    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to locate AuthProtocol.  Can't use check auth.  %r\n", __FUNCTION__, Status));
      mAuthProtocol = NULL;
      return Status;
    }
  }

  return mAuthProtocol->AuthWithSignedData(mAuthProtocol, SignedData, SignedDataLen, Signature, AuthToken);
}

/**
Pass thru function for using the Auth Protocol to dispose of an auth token 
so it can no longer be used in the system. 

@param[in]  AuthToken - Pointer to auth token to dispose of
**/
EFI_STATUS
EFIAPI
AuthTokenDispose(
  IN DFCI_AUTH_TOKEN  *AuthToken
  )
{
  if ((AuthToken == NULL) || (*AuthToken == DFCI_AUTH_TOKEN_INVALID))
  {
    return EFI_SUCCESS;
  }

  //Can't get here if mAuthProtocol is NULL
  if (mAuthProtocol != NULL) 
  {
    return mAuthProtocol->DisposeAuthToken(mAuthProtocol, AuthToken);
  }

  DEBUG((DEBUG_ERROR, "%a - Can't dispose of auth token because no AuthProtocol. \n", __FUNCTION__));
  return EFI_NOT_READY;
}


/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

**/
EFI_STATUS
EFIAPI
Init (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  
  
  //Install Setting Provider Support Protocol and Permission Protocol
  Status = gBS->InstallMultipleProtocolInterfaces(
    &ImageHandle,
    &gDfciSettingsProviderSupportProtocolGuid,
    &mProviderProtocol,
    &gDfciSettingPermissionsProtocolGuid,
    &mPermissionProtocol,
    NULL
    );

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to Install DFCI Settings Provider Support/Permission Protocol. %r\n", Status));
    goto EXIT;
  }

  //
  // Register notify function to print all settings and publish SettingsAccess on BdsEntry Event.
  //
  Status = gBS->CreateEventEx(
    EVT_NOTIFY_SIGNAL,
    TPL_CALLBACK,
    SettingManagerDxeEventNotify,
    ImageHandle,  //set the context to the image handle
    &gDfciStartOfBdsNotifyGuid,
    &mSmDxeEvent
    );

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Create Event Ex for End of Dxe. %r\n", __FUNCTION__, Status));
  }

  //
  // Register notify function to re-publish Settings at ReadyToBoot so current settings can be placed in FACS.
  //
  Status = gBS->CreateEventEx(
    EVT_NOTIFY_SIGNAL,
    TPL_CALLBACK,
    SettingManagerReadyToBootEventNotify,
    ImageHandle,  //set the context to the image handle
    &gEfiEventPreReadyToBootGuid,
    &mSmReadyToBootEvent
    );

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Create Event Ex for Ready to Boot. %r\n", __FUNCTION__, Status));
  }

EXIT:
  return Status;
}
