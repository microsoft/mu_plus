/**@file
IdentityAndAuthManagerDxe.c

Entrypoint for the Identity And Auth Manager Dxe driver. This should handle
all the DXE specific behavior and leave the core code logic to the other C
files within this module.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

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

// Apply Identity Protocol
DFCI_APPLY_PACKET_PROTOCOL mApplyIdentityProtocol = {
       DFCI_APPLY_PACKET_SIGNATURE,
       DFCI_APPLY_PACKET_VERSION,
       {
           0,
           0,
           0
       },
       ApplyNewIdentityPacket,
       SetIdentityResponse,
       LKG_Handler
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
  BOOLEAN          SaveState;
  EFI_STATUS       Status = EFI_SUCCESS;
  ZERO_TOUCH_STATE ZeroTouchState;

  Status = gBS->LocateProtocol(&gDfciSettingPermissionsProtocolGuid, NULL, (VOID **) &mDfciSettingsPermissionProtocol);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - DfciSystemSettingPermissionsProtocolGuid not available. %r\n", __FUNCTION__, Status));
    return Status;
  }

  Status = PopulateInternalCertStore();  //Check variable and load existing data into internal store
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "PopulateInternalCertStore failed %r\n", Status));
  }

  // If user has not previously opted out of device management, and the system is
  // in Mfg mode, automatically opt in for device management.
  ZeroTouchState = GetZeroTouchState();
  if (ZERO_TOUCH_INACTIVE == ZeroTouchState) {
    if (DfciUiIsManufacturingMode()) {
      ZeroTouchState = ZERO_TOUCH_OPT_IN;
    }
  }

  SaveState = FALSE;
  switch (ZeroTouchState) {
    case ZERO_TOUCH_OPT_IN:
      if ((mInternalCertStore.Certs[CERT_ZTD_INDEX].Cert == NULL) &&
          (mInternalCertStore.Certs[CERT_OWNER_INDEX].Cert == NULL))
      {
        if (FeaturePcdGet(PcdDfciEnabled)) {
          Status = GetZeroTouchCertificate( (UINT8 **) &mInternalCertStore.Certs[CERT_ZTD_INDEX].Cert,
                                            &mInternalCertStore.Certs[CERT_ZTD_INDEX].CertSize);
          if (EFI_ERROR(Status))
          {
            DEBUG((DEBUG_ERROR, "[AM] - Unable to obtain built in cert. Code=%r.\n",Status));
          } else {
            SaveState = TRUE;
            mInternalCertStore.PopulatedIdentities |= DFCI_IDENTITY_SIGNER_ZTD;
          }
        }
      }
      break;

    case ZERO_TOUCH_OPT_OUT:
      if (mInternalCertStore.Certs[CERT_ZTD_INDEX].Cert != NULL)
      {
        SaveState = TRUE;
        FreePool ((VOID *) mInternalCertStore.Certs[CERT_ZTD_INDEX].Cert);
        mInternalCertStore.Certs[CERT_ZTD_INDEX].Cert = NULL;
        mInternalCertStore.Certs[CERT_ZTD_INDEX].CertSize = 0;
        mInternalCertStore.PopulatedIdentities &= ~DFCI_IDENTITY_SIGNER_ZTD;
      }
      break;

    default:
      break;
  }

  if (SaveState) {
    Status = SaveProvisionedData();
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "[AM] - Unable to save provisioned data with ZTD. Code=%r.\n",Status));
    }
    else
    {
      DEBUG((DEBUG_INFO, "%a - Added or removed ZTD\n", __FUNCTION__));
    }
    PopulateCurrentIdentities(TRUE);       // Force updating Current XML when changing ZTD
  }

  // Print the current internal store.

  DebugPrintCertStore(&mInternalCertStore);

  PopulateCurrentIdentities(FALSE);       // If there are no "Current" Identities Variable,
                                          // Populate the Current XML.

  //Install Auth Provider Support Protocol and Apply Identity Protocol
  Status = gBS->InstallMultipleProtocolInterfaces(
    &ImageHandle,
    &gDfciAuthenticationProtocolGuid,
    &mAuthProtocol,
    &gDfciApplyIdentityProtocolGuid,
    &mApplyIdentityProtocol,
    NULL
    );

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to Install DFCI Auth Protocol. %r\n", Status));
  }


  return EFI_SUCCESS;
}



