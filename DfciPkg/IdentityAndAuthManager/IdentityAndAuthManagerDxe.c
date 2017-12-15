/*++ @file
    Entrypoint for the Identity And Auth Manager Dxe driver.
    This should handle all the DXE specific behavior and leave the
    core code logic to the other C files within this module.


    Copyright (C) 2014 Microsoft Corporation. All Rights Reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    PARTICULAR PURPOSE.

--*/
#include "IdentityAndAuthManager.h"

DFCI_AUTHENTICATION_PROTOCOL  mAuthProtocol = {
    GetEnrolledIdentities,
    AuthWithPW,
    AuthWithSignedData,
    DisposeAuthToken,
    GetIdentityProperties,
    GetCertInfo,
    GetRecoveryPacket,
    SetRecoveryResponse
};

DFCI_SETTING_PERMISSIONS_PROTOCOL *mDfciSettingsPermissionProtocol = NULL;


/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

**/
EFI_STATUS
EFIAPI
Init(
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = PopulateInternalCertStore();  //Check variable and load existing data into internal store
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "PopulateInternalCertStore failed %r\n", Status));
  }

  DebugPrintCertStore(&mInternalCertStore);

  Status = gBS->LocateProtocol(&gDfciSettingPermissionsProtocolGuid, NULL, &mDfciSettingsPermissionProtocol);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - DfciSystemSettingPermissionsProtocolGuid not available. %r\n", __FUNCTION__, Status));
    return Status;
  }

  //Check for new Input data
  CheckForNewProvisionInput();

  //Install Auth Provider Support Protocol
  Status = gBS->InstallMultipleProtocolInterfaces(
    &ImageHandle,
    &gDfciAuthenticationProtocolGuid,
    &mAuthProtocol,
    NULL
    );

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to Install DFCI Auth Protocol. %r\n", Status));
  }


  return EFI_SUCCESS;
}



