/* @file
DfciUiSupportLib - Library supports UI components associated with DFCI.


Copyright (C) 2016 Microsoft Corporation. All Rights Reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

**/

#ifndef __DFCI_UI_SUPPORT_LIB_H__
#define __DFCI_UI_SUPPORT_LIB_H__

#include <DfciSystemSettingTypes.h>
#include <Protocol/DfciAuthentication.h>

#define DFCI_MB_OK                   0x00000000  // The message box contains one push button: OK. This is the default.
#define DFCI_MB_OKCANCEL             0x00000001  // The message box contains two push buttons: OK and Cancel.
#define DFCI_MB_RESTART              0x00000009  // The message box contains one push button: Restart (ID_OK).

// Message Box Default Button Configuration Types
//
#define DFCI_MB_DEFAULT(A)           (A & 0xF00) // Mask off everything but the Default button.
#define DFCI_MB_DEFAULT_ACTION       0x00000000  // The current default action
#define DFCI_MB_DEFBUTTON1           0x00000100  // The first button is the default button. MB_DEFBUTTON1 is the default unless MB_DEFBUTTON2, MB_DEFBUTTON3, or MB_DEFBUTTON4 is specified.
#define DFCI_MB_DEFBUTTON2           0x00000200  // The second button is the default button.

// MessageBox Styles
#define DFCI_MB_STYLE_TYPE(A)        (A & 0xF0000)      // Mask off everything but the style type.
#define DFCI_MB_STYLE_NORMAL         0x00000000         // The normal MessageBox.
#define DFCI_MB_STYLE_ALERT1         0x00010000         // The First alert message box (yesslow)
#define DFCI_MB_STYLE_ALERT2         0x00020000         // The Second alert message box (Red)

typedef UINTN DFCI_AUTH_TOKEN;

// Auth defines and values
#define DFCI_AUTH_TOKEN_INVALID (0x0)

typedef enum
{
    DFCI_MB_IDOK = 1,                // The OK button was selected.
    DFCI_MB_IDCANCEL,                // The Cancel button was selected.
    DFCI_MB_IDABORT,                 // The Abort button was selected.
    DFCI_MB_IDRETRY,                 // The Retry button was selected.
    DFCI_MB_IDIGNORE,                // The Ignore button was selected.
    DFCI_MB_IDYES,                   // The Yes button was selected.
    DFCI_MB_IDNO,                    // The No button was selected.
    DFCI_MB_IDTRYAGAIN = 10,         // The Try Again button was selected.
    DFCI_MB_IDCONTINUE,              // The Continue button was selected.
    DFCI_MB_TIMEOUT,                 // MessageBox with Timeout timed out
    DFCI_MB_IDNEXT,                  // The Next button was selected
    DFCI_MB_IDRESTART                // The Restart button was selected
} DFCI_MB_RESULT;

EFI_STATUS
EFIAPI
DfciUiDisplayPasswordDialog (
  IN  CHAR16                              *TitleText,
  IN  CHAR16                              *CaptionText,
  IN  CHAR16                              *BodyText,
  IN  CHAR16                              *ErrorText,
  OUT DFCI_MB_RESULT                      *Result,
  OUT CHAR16                              **Password
  );

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
  );

EFI_STATUS
EFIAPI
DfciUiDisplayMessageBox (
    IN  CHAR16          *TitleBarText,
    IN  CHAR16          *Text,
    IN  CHAR16          *Caption,
    IN  UINT32          Type,
    IN  UINT64          Timeout,
    OUT DFCI_MB_RESULT   *Result
    );

/**
  This routine indicates if the UI is ready and can be used.
 
  @retval  TRUE if the UI is ready to use, else FALSE.
    
**/
BOOLEAN
EFIAPI
DfciUiIsUiAvailable (
  VOID
  );

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
DfciUiCheckForDelayProcessingNeeded (
  IN BOOLEAN Unenroll,
  IN BOOLEAN LocalAuthNeeded
  );

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
  );


#endif //__DFCI_UI_SUPPORT_LIB_H__
