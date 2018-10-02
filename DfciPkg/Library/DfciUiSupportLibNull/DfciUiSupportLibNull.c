/** @file
DfciUiSupportLibNull.c

NULL instance of the UiSupportLib.

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

#include <Library/DfciUiSupportLib.h>

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
  OUT CHAR16                              **Password OPTIONAL,
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

