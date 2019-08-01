/** @file
DfciUiSupportLibNull.c

NULL instance of the UiSupportLib.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DfciUiSupportLib.h>

/**
  This routine indicates if the system is in Manufacturing Mode.

  @retval  ManufacturingMode - Platforms may have a manufacturing mode.
                               DFCI Auto opt-in's the management cert included
                               in the firmware volume in Manufacturing Mode.
                               TRUE if the device is in Manufacturing Mode
**/
BOOLEAN
EFIAPI
DfciUiIsManufacturingMode (
  VOID
  ) {

  return FALSE;
}

/**

  This routine indicates if the UI is ready and can be used.

  @retval  TRUE if the UI is ready to use, else FALSE.

**/
BOOLEAN
EFIAPI
DfciUiIsUiAvailable (
   VOID
  )
{

  return TRUE;
}

EFI_STATUS
EFIAPI
DfciUiDisplayMessageBox (
    IN  CHAR16          *TitleBarText,
    IN  CHAR16          *Text,
    IN  CHAR16          *Caption,
    IN  UINT32          Type,
    IN  UINT64          Timeout,
    OUT DFCI_MB_RESULT   *Result
    )
{

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DfciUiDisplayPasswordDialog (
  IN  CHAR16                              *TitleText,
  IN  CHAR16                              *CaptionText,
  IN  CHAR16                              *BodyText,
  IN  CHAR16                              *ErrorText,
  OUT DFCI_MB_RESULT                      *Result,
  OUT CHAR16                              **Password
  ) {

    return EFI_SUCCESS;
}


/**
 * DfciUiDisplayDfciAuthDialog
 *
 * @param TitleText
 * @param CaptionText
 * @param BodyText
 * @param CertText
 * @param ConfirmText
 * @param ErrorText
 * @param PasswordType
 * @param Result
 * @param OPTIONAL
 * @param OPTIONAL
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DfciUiDisplayAuthDialog (
  IN  CHAR16                              *TitleText,
  IN  CHAR16                              *CaptionText,
  IN  CHAR16                              *BodyText,
  IN  CHAR16                              *CertText,
  IN  CHAR16                              *ConfirmText,
  IN  CHAR16                              *ErrorText,
  IN  BOOLEAN                             PasswordType,
  IN  CHAR16                              *Thumbprint,
  OUT DFCI_MB_RESULT                      *Result,
  OUT CHAR16                              **Password OPTIONAL
)  {

    return EFI_UNSUPPORTED;
}

/**
  This routine is called by DFCI to prompt a local user to confirm certificate
  provisioning operations.

  @param  AuthMgrProtocol        Supplies a pointer to the authentication
                                 manager protocol.
  @param  TrustedCert            Supplies a pointer to a trusted certificate.
  @param  TrustedCertSize        Supplies the size in bytes of the trusted
                                 certificate.
  @param  AuthToken              Supplies a pointer that will receive an auth-
                                 entication token.

  @return EFI_NOT_READY          Indicates that UI components are not available.
  @return EFI_ACCESS_DENIED      The user rejected the operation.
  @return EFI_SUCCESS            The user approved the operation.

**/
EFI_STATUS
EFIAPI
DfciUiGetAnswerFromUser(
  DFCI_AUTHENTICATION_PROTOCOL* AuthMgrProtocol,
  UINT8* TrustedCert,
  UINT16 TrustedCertSize,
  OUT DFCI_AUTH_TOKEN* AuthToken
  )
{

  return EFI_SUCCESS;
}

/**
    DfciUiExitSecurityBoundary

    UEFI that support locked settings variables can lock those
    variable when this function is called.  DFCI will call this function
    before enabling USB or the Network device which are considered unsafe.

    Signal PreReadyToBoot - lock private settings variable to insure
           USB or Network don't have access to locked settings.
    Disable the OSK from displaying (PreReadyToBoot also enables the OSK)
**/
VOID
DfciUiExitSecurityBoundary (VOID)
{
  return;
}