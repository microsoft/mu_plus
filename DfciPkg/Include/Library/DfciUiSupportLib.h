/** @file
DfciUiSupportLib.h

Library supports UI components associated with DFCI.

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
  );

/**

  This routine indicates if the UI is ready and can be used.

  @param   ManufacturingMode - Platforms may have a manufacturing mode.
                               DFCI Auto opt-in's the management cert included
                               in the firmware volume in Manufacturing Mode.
                               TRUE if the device is in Manufacturing Mode

  @retval  TRUE if the UI is ready to use, else FALSE.

**/
BOOLEAN
EFIAPI
DfciUiIsUiAvailable (
    VOID
  );

/**
 * Display a Message Box
 *
 * NOTE: The UI must be avaialable
 *
 * @param TitleBarText
 * @param Text
 * @param Caption
 * @param Type
 * @param Timeout
 * @param Result
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DfciUiDisplayMessageBox (
    IN  CHAR16          *TitleBarText,
    IN  CHAR16          *Text,
    IN  CHAR16          *Caption,
    IN  UINT32           Type,
    IN  UINT64           Timeout,
    OUT DFCI_MB_RESULT   *Result
    );

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
 * @param Thumbprint
 * @param Result
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
  );

#endif //__DFCI_UI_SUPPORT_LIB_H__
