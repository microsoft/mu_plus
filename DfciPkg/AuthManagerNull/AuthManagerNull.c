/**@file
AuthManagerNull.c

Consistently provides the same token.
Don't use in production!

Copyright (c) 2019, Microsoft Corporation

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
#include <DfciSystemSettingTypes.h>

#include <Protocol/DfciAuthentication.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_HANDLE                         mImageHandle = NULL;



EFI_STATUS
EFIAPI
GetIdentityProperties(
IN  CONST DFCI_AUTHENTICATION_PROTOCOL     *This,
IN  CONST DFCI_AUTH_TOKEN                  *IdentityToken,
IN OUT DFCI_IDENTITY_PROPERTIES            *Properties
)
{
  DEBUG((DEBUG_ERROR, "NullAuthManager - %a\n", __FUNCTION__));
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
DisposeAuthToken(
IN  CONST DFCI_AUTHENTICATION_PROTOCOL        *This,
IN OUT DFCI_AUTH_TOKEN                        *IdentityToken
)
{
  DEBUG((DEBUG_ERROR, "NullAuthManager - %a\n", __FUNCTION__));
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
AuthWithSignedData(
  IN  CONST DFCI_AUTHENTICATION_PROTOCOL     *This,
  IN  CONST UINT8                            *SignedData,
  IN  UINTN                                   SignedDataLength,
  IN  CONST WIN_CERTIFICATE                  *Signature,
  IN OUT DFCI_AUTH_TOKEN                     *IdentityToken
)
{
  DEBUG((DEBUG_ERROR, "NullAuthManager - %a\n", __FUNCTION__));
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
AuthWithPW(
IN  CONST DFCI_AUTHENTICATION_PROTOCOL     *This,
IN  CONST CHAR16                           *Password OPTIONAL,
IN  UINTN                                   PasswordLength,
OUT DFCI_AUTH_TOKEN                        *IdentityToken
)
{
  DEBUG((DEBUG_ERROR, "NullAuthManager - %a\n", __FUNCTION__));

  *IdentityToken = 0x37;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GetRecoveryPacket(
  IN CONST DFCI_AUTHENTICATION_PROTOCOL       *This,
  IN       DFCI_IDENTITY_ID                   Identity,
  OUT      DFCI_AUTH_RECOVERY_PACKET          **Packet
  )
{
  DEBUG((DEBUG_ERROR, "NullAuthManager - %a\n", __FUNCTION__));
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
SetRecoveryResponse(
  IN CONST DFCI_AUTHENTICATION_PROTOCOL       *This,
  IN CONST UINT8                              *RecoveryResponse,
  IN       UINTN                               Size
  )
{
  DEBUG((DEBUG_ERROR, "NullAuthManager - %a\n", __FUNCTION__));
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
GetEnrolledIdentities(
  IN CONST DFCI_AUTHENTICATION_PROTOCOL       *This,
  OUT      DFCI_IDENTITY_MASK                 *EnrolledIdentities
  )
{
  DEBUG((DEBUG_ERROR, "NullAuthManager - %a\n", __FUNCTION__));
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
GetCertInfo(
  IN CONST DFCI_AUTHENTICATION_PROTOCOL       *This,
  IN       DFCI_IDENTITY_ID                    Identity,
  IN CONST UINT8                              *Cert          OPTIONAL,
  IN       UINTN                               CertSize,
  IN       DFCI_CERT_REQUEST                   CertRequest,
  IN       DFCI_CERT_FORMAT                    CertFormat,
  OUT      VOID                              **Value,
  OUT      UINTN                              *ValueSize     OPTIONAL
  )
{
  DEBUG((DEBUG_ERROR, "NullAuthManager - %a\n", __FUNCTION__));
  return EFI_UNSUPPORTED;
}

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
  EFI_STATUS Status;
  DEBUG((DEBUG_ERROR, "NullAuthManager %a, this is not a secure implementation of AuthManager!!\n", __FUNCTION__));
  //Install Auth Provider Support Protocol and Apply Identity Protocol
  Status = gBS->InstallMultipleProtocolInterfaces(
    &mImageHandle,
    &gDfciAuthenticationProtocolGuid,
    &mAuthProtocol,
    NULL);

  return Status;
}