/**@file
IdentityAndAuthManagerDxe.c

Entrypoint for the Identity And Auth Manager Dxe driver. This should handle
all the DXE specific behavior and leave the core code logic to the other C
files within this module.

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

  SaveState = FALSE;
  ZeroTouchState = GetZeroTouchState();
  switch (ZeroTouchState) {
    case ZERO_TOUCH_OPT_IN:
      if ((mInternalCertStore.Certs[CERT_ZTD_INDEX].Cert == NULL) &&
          (mInternalCertStore.Certs[CERT_OWNER_INDEX].Cert == NULL))
      {
        if (FeaturePcdGet(PcdDfciEnabled)) {
          Status = GetZeroTouchCertificate( &mInternalCertStore.Certs[CERT_ZTD_INDEX].Cert,
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
        FreePool (mInternalCertStore.Certs[CERT_ZTD_INDEX].Cert);
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



