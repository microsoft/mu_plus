/** @file
  SwmDialogsLib.h

  Define the Simple Window Manager Dialogs constants and common structures.

  Copyright (c) 2018, Microsoft Corporation.

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

#ifndef _SIMPLE_WINDOW_MANAGER_DIALOGS_H_
#define _SIMPLE_WINDOW_MANAGER_DIALOGS_H_

// Message Box Button Configuration Types
//
#define SWM_MB_BASE_TYPE(A)         (A & 0xF)   // Mask off everything but the base type.
#define SWM_MB_ABORTRETRYIGNORE     0x00000002  // The message box contains three push buttons: Abort, Retry, and Ignore.
#define SWM_MB_CANCELTRYCONTINUE    0x00000006  // The message box contains three push buttons: Cancel, Try Again, Continue. Use this message box type instead of MB_ABORTRETRYIGNORE.
#define SWM_MB_OK                   0x00000000  // The message box contains one push button: OK. This is the default.
#define SWM_MB_OKCANCEL             0x00000001  // The message box contains two push buttons: OK and Cancel.
#define SWM_MB_RETRYCANCEL          0x00000005  // The message box contains two push buttons: Retry and Cancel.
#define SWM_MB_YESNO                0x00000004  // The message box contains two push buttons: Yes and No.
#define SWM_MB_YESNOCANCEL          0x00000003  // The message box contains three push buttons: Yes, No, and Cancel.
#define SWM_MB_CANCEL               0x00000007  // The message box contains one push button: Cancel.
#define SWM_MB_CANCELNEXT           0x00000008  // The message box contains two push buttons: Cancel and Next (ID_OK).
#define SWM_MB_RESTART              0x00000009  // The message box contains one push button: Restart (ID_OK).

// Message Box Optional Button Types
//
#define SWM_MB_HELP                 0x00004000  // Adds a Help button to the message box. When the user clicks the Help button or presses F1, the system sends a WM_HELP message to the owner.

// Message Box Default Button Configuration Types
//
#define SWM_MB_DEFAULT(A)           (A & 0xF00) // Mask off everything but the Default button.
#define SWM_MB_DEFAULT_ACTION       0x00000000  // The current default action
#define SWM_MB_DEFBUTTON1           0x00000100  // The first button is the default button. MB_DEFBUTTON1 is the default unless MB_DEFBUTTON2, MB_DEFBUTTON3, or MB_DEFBUTTON4 is specified.
#define SWM_MB_DEFBUTTON2           0x00000200  // The second button is the default button.
//#define SWM_MB_DEFBUTTON3           0x00000300  // The third button is the default button.
//#define SWM_MB_DEFBUTTON4           0x00000400  // The fourth button is the default button.
#define SWM_MB_NO_DEFAULT           0x00000f00  // No button is the default button. MB_DEFBUTTON1 is the default unless MB_DEFBUTTON2, MB_DEFBUTTON3, or MB_DEFBUTTON4 is specified.

// MessageBox Styles
#define SWM_MB_STYLE_TYPE(A)        (A & 0xF0000)      // Mask off everything but the style type.
#define SWM_MB_STYLE_NORMAL         0x00000000         // The normal MessageBox.
#define SWM_MB_STYLE_ALERT1         0x00010000         // The First alert message box (yesslow)
#define SWM_MB_STYLE_ALERT2         0x00020000         // The Second alert message box (Red)

// Message Box Return Values.
//
typedef enum
{
    SWM_MB_IDOK = 1,                // The OK button was selected.
    SWM_MB_IDCANCEL,                // The Cancel button was selected.
    SWM_MB_IDABORT,                 // The Abort button was selected.
    SWM_MB_IDRETRY,                 // The Retry button was selected.
    SWM_MB_IDIGNORE,                // The Ignore button was selected.
    SWM_MB_IDYES,                   // The Yes button was selected.
    SWM_MB_IDNO,                    // The No button was selected.
    SWM_MB_IDTRYAGAIN = 10,         // The Try Again button was selected.
    SWM_MB_IDCONTINUE,              // The Continue button was selected.
    SWM_MB_TIMEOUT,                 // MessageBox with Timeout timed out
    SWM_MB_IDNEXT,                  // The Next button was selected
    SWM_MB_IDRESTART                // The Restart button was selected
} SWM_MB_RESULT;

// Password Dialog Types.
//
typedef enum
{
    SWM_PWD_TYPE_PROMPT_PASSWORD,   // Display standard password prompt dialog.
    SWM_PWD_TYPE_SET_PASSWORD,      // Display set/change password dialog.
    SWM_PWD_TYPE_ALERT_PASSWORD,    //Display Standard password prompt Dialog in Alert mode
    SWM_THMB_TYPE_ALERT_PASSWORD,    //Display Standard password prompt Dialog in Alert mode with an additional editbox for entering thumbprint. (SEMM enroll)
    SWM_THMB_TYPE_ALERT_THUMBPRINT    //Display Standard password prompt Dialog in Alert mode with an editbox for entering thumbprint and no password. (SEMM enroll)
} SWM_PWD_DIALOG_TYPE;

/**
 *  MessageBox.  Display a Message box 
 *
 *
 * @param pTitleBarText  - Text for titlebar of message box
 * @param pCaption       - Text for Title of message box
 * @param pBodyText      - Test for body of the message box
 * @param Type           - SWM_MB_STYLE - Normal/Alert2/Alert2
 * @param Timeout        - Number of 100ns unite of timeout (compatible with UEFI Event Time)
 * @param Result         - Message Box Result
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
SwmDialogsMessageBox (
    IN  CHAR16              *pTitleBarText,
    IN  CHAR16              *pCaption,
    IN  CHAR16              *pBodyText,
    IN  UINT32              Type,
    IN  UINT64              Timeout,
    OUT SWM_MB_RESULT       *Result
  );

/**
 *  PasswordPrompt.  Display a Message box and receive hidden text
 *
 *
 * @param pTitleBarText  - Text for titlebar of message box
 * @param pCaptionText   - Text for Title of message box
 * @param pBodyText      - Tecx for body of the message box
 * @param pErrorText     - Text for error message (for reprompt)
 * @param Type           - SWM_MB_STYLE - Normal/Alert2/Alert2
 * @param Result         - Message Box Result
 * @param Password       - Where to store pointer to allocated buffer with password result
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
SwmDialogsPasswordPrompt (
    IN  CHAR16              *pTitleBarText,
    IN  CHAR16              *pCaptionText,
    IN  CHAR16              *pBodyText,
    IN  CHAR16              *pErrorText,
    IN  SWM_PWD_DIALOG_TYPE Type,
    OUT SWM_MB_RESULT       *Result,
    OUT CHAR16              **Password
  );

/**
 *  SelectPrompt.  Display a Message box with a selection item and return
 *                 the selected index.
 *
 * @param pTitleBarText  - Text for titlebar of message box
 * @param pCaptionText   - Text for Title of message box
 * @param pBodyText      - Tecx for body of the message box
 * @param pOptionsList   - Array of option text
 * @param OptionsCount   - Cout of options
 * @param Result         - SMB_RESULT 
 * @param SelectedIndex  - Index of selected option when Result is SMB_RESULT_OK
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
SwmDialogsSelectPrompt (
    IN  CHAR16              *pTitleBarText,
    IN  CHAR16              *pCaptionText,
    IN  CHAR16              *pBodyText,
    IN  CHAR16              **pOptionsList,
    IN  UINTN               OptionsCount,
    OUT SWM_MB_RESULT       *Result,
    OUT UINTN               *SelectedIndex
  );

/**
 *  VerifyThumbprintPrompt.  Display a Message box with a Thumbprint
 *                               verification text box, and an optional
 *                               Password box.  
 *
 * @param pTitleBarText  - Text for titlebar of message box
 * @param pCaptionText   - Text for Title of message box
 * @param pBodyText      - Tecx for body of the message box
 * @param pCertText      - Multiline text string to identify the Cert
 * @param pConfirmText   - Instructions for current format of the dialog
 * @param pErrorText     - Error message to display if number of attempts is exceeded
 * @param Type           - SWM_MB_STYLE - Normal/Alert2/Alert2
 * @param Result         - SMB_RESULT 
 * @param Password       - Where to store pointer to allocated buffer with password result
 * @param Thumbprint     - Where to store the two character thumbprint
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
SwmDialogsVerifyThumbprintPrompt (
    IN  CHAR16              *pTitleBarText,
    IN  CHAR16              *pCaptionText,  
    IN  CHAR16              *pBodyText,
    IN  CHAR16              *pCertText,
    IN  CHAR16              *pConfirmText,
    IN  CHAR16              *pErrorText,
    IN  SWM_PWD_DIALOG_TYPE Type,
    OUT SWM_MB_RESULT       *Result,
    OUT CHAR16              **Password,
    OUT CHAR16              **Thumbprint
  );

/**
 *  SwmDIalogsReady.  Are Dialogs ready? The SWMProtocol starts late,
 *                    and SwmDialogsReady returns TRUE if Dialogs are ready.
 */
BOOLEAN
EFIAPI
SwmDialogsReady (
    VOID
  );

#endif      // _SIMPLE_WINDOW_MANAGER_DIALOGS_H_
