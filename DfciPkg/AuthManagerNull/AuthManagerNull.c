/**@file
AuthManagerNull.c

Consistently provides the same token.
Don't use in production!

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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