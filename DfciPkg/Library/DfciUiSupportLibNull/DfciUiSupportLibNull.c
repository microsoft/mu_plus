#include <Library/DfciUiSupportLib.h>

EFI_STATUS
EFIAPI
DfciUiDisplayDfciAuthDialog (
  IN  CHAR16                              *TitleText,
  IN  CHAR16                              *CaptionText,
  IN  CHAR16                              *BodyText,
  IN  CHAR16                              *CertText,
  IN  CHAR16                              *ConfirmText,
  IN  CHAR16                              *ErrorText,
  IN  BOOLEAN                             PasswordType,
  OUT DFCI_MB_RESULT                      *Result,
  OUT CHAR16                              **Password OPTIONAL,
  OUT CHAR16                              **Thumbprint OPTIONAL
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
  )
{

  return EFI_SUCCESS;
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

/**

  This routine allows the library to make preperations to use the UI.
 
  @retval  Returns an EFI_STATUS.
    
**/
EFI_STATUS
EFIAPI
DfciUiPrepareToUseUi (
    VOID
    )
{

  return EFI_SUCCESS;
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

/**
  This routine is called by DFCI to check if certificate provisioning needs to
  be delayed. If components needed for a user to approve a provisioning request
  are not available, DFCI will delay the processing.

  @param  Unenroll               Supplies a value that indicates if the
                                 provisioning request is for unenrolling the
                                 device from DFCI.
  @param  LocalAuthNeeded        Supplies a value that idicates if the
                                 provisioning operation requires local user
                                 authentication.

  @return EFI_SUCCESS            Delayed processing is not needed.
  @return other status           Delayed processing is needed.

**/
EFI_STATUS
EFIAPI
CheckForDelayProcessingNeeded(
  IN BOOLEAN Unenroll,
  IN BOOLEAN LocalAuthNeeded
  )
{

  return EFI_SUCCESS;
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
GetAnswerFromUser(
  DFCI_AUTHENTICATION_PROTOCOL* AuthMgrProtocol,    
  UINT8* TrustedCert,
  UINT16 TrustedCertSize,
  OUT DFCI_AUTH_TOKEN* AuthToken
  )
{

  return EFI_SUCCESS;
}
