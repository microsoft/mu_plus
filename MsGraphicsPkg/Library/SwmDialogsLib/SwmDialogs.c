/** @file

  Copyright (c) 2015 - 2018, Microsoft Corporation.

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

#include "SwmDialogs.h"

// SwmDialogs HII Package GUID: { fa096591-b216-4ec8-9aa7-09a4d5700e82}
#define SWMDIALOGS_HII_PACKAGE_LIST_GUID                                                  \
{                                                                                  \
    0xfa096591, 0xb216, 0x4ec8, { 0x9a, 0xa7, 0x09, 0xa4, 0xd5, 0x70, 0x0e, 0x82 } \
}

       EFI_GRAPHICS_OUTPUT_PROTOCOL      *gGop = NULL;
       EFI_HANDLE                         gPriorityHandle = NULL;
       EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *gSimpleTextInEx = NULL;
       EFI_HII_HANDLE                     gSwmDialogsHiiHandle = NULL;

static EFI_GUID                           gPriorityGuid = SWM_PRIORITY_PROTOCOL_GUID;
static EFI_GUID                           gSwmDialogsHiiPackageListGuid = SWMDIALOGS_HII_PACKAGE_LIST_GUID;
static MS_SIMPLE_WINDOW_MANAGER_PROTOCOL *gSwmProtocol = NULL;
static EFI_EVENT                          gSwmRegisterEvent = NULL;
static VOID                              *gSwmRegistration;

/**
 *  MessageBox.  Display a Message box
 *
 *
 * @param pTitleBarText  - Text for titlebar of message box
 * @param pCaption       - Text for Title of message box
 * @param pBodyText      - Test for body of the message box
 * @param Type           - SWM_MB_STYLE - Normal/Alert2/Alert2
 * @param Timeout        - Number of 100ns unite of timeout (compatible with UEFI Event Time)
 * @param Result         - Indicator of what button was pressed
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
    OUT SWM_MB_RESULT       *Result)
{

    if (gSwmProtocol == NULL) {
        return EFI_ABORTED;
    }

    return MessageBoxInternal(gSwmProtocol,
                              pTitleBarText,
                              pCaption,
                              pBodyText,
                              Type,
                              Timeout,
                              Result);
}

/**
 *  PasswordPrompt.  Display a Message box and receive hidden text
 *
 *
 * @param pTitleBarText  - Text for titlebar of message box
 * @param pCaptionText   - Text for Title of message box
 * @param pBodyText      - Tecx for body of the message box
 * @param pErrorText     - Text for error message (for reprompt)
 * @param Type           - SWM_MB_STYLE - Normal/Alert2/Alert2
 * @param Result         - Indicator of what button was pressed
 * @param Password       - Where to store an allocated buffer with the Password
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
  ) {

    if (gSwmProtocol == NULL) {
        return EFI_ABORTED;
    }

    return PasswordDialogInternal (gSwmProtocol,
                                   pTitleBarText,
                                   pCaptionText,
                                   pBodyText,
                                   pErrorText,
                                   Type,
                                   Result,
                                   Password);
}

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
 * @param SelectedIndex  - Index of selected option when SMB_RESULT_OK
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
  ) {

    if (gSwmProtocol == NULL) {
        return EFI_ABORTED;
    }

    return SingleSelectDialogInternal (gSwmProtocol,
                                       pTitleBarText,
                                       pCaptionText,
                                       pBodyText,
                                       pOptionsList,
                                       OptionsCount,
                                       Result,
                                       SelectedIndex);
}

/**
 *  VerifyThumbprintPrompt.  Display a Message box with a Thumbprint
 *                               verification text box, and an optional
 *                               Password box.
 *
 * @param pTitleBarText   - Text for titlebar of message box
 * @param pCaptionText    - Text for caption (Title) of message box
 * @param pBodyText       - Text for body of the message box
 * @param pCertText       - Multi line text - Subject, Issuer, Thumbprint - 2
 * @param pConfirmText    - User instruction displayed in dialog
 * @param pErrorText      - Error text to display for retry
 * @param Type            - SWM_MB_STYLE - Normal/Alert2/Alert2
 * @param Result          - SMB_RESULT
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
    OUT CHAR16              **Password OPTIONAL,
    OUT CHAR16              **Thumbprint OPTIONAL
  ) {

    if (gSwmProtocol == NULL) {
        return EFI_ABORTED;
    }

   return VerifyThumbprintInternal(gSwmProtocol,
                                   pTitleBarText,
                                   pCaptionText,
                                   pBodyText,
                                   pCertText,
                                   pConfirmText,
                                   pErrorText,
                                   Type,
                                   Result,
                                   Password,
                                   Thumbprint);
}

/**
  SwmDialogsReady - Allow a caller to verify that Dialogs can be displayed


  @retval EFI_STATUS

**/
BOOLEAN
EFIAPI
SwmDialogsReady (
  ) {

  return (gSwmProtocol != NULL);
}

/**
  AllocateRequireProtocols - Allocate Gop and SimpleTxtInEx

          SimpleWindowManagerProtocol comes alive due to allocating
          the console.  This means Gop and SimpleTxtInEx is present


  @retval EFI_STATUS

**/
static
EFI_STATUS
AllocateRequiredProtocols (
  VOID
  ){
    EFI_STATUS  Status;

    Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **) &gGop);
    if (EFI_ERROR (Status)) {
        DEBUG((DEBUG_INFO,"%a: Failed to get GraphicsOutput (%r).\r\n", __FUNCTION__, Status));
        gSwmProtocol = NULL;
    } else {
        if (gST->ConsoleInHandle != NULL) {
            Status = gBS->OpenProtocol (
                        gST->ConsoleInHandle,
                        &gEfiSimpleTextInputExProtocolGuid,
                        (VOID **) &gSimpleTextInEx,
                        NULL,
                        NULL,
                        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
        } else {
          DEBUG((DEBUG_ERROR,"%a: SystemTable ConsoleInHandle is NULL\n", __FUNCTION__));
          Status = EFI_NOT_READY;
        }

        if (EFI_ERROR (Status)) {
            DEBUG((DEBUG_INFO,"%a: Failed to get SimpleTextInEx (%r).\r\n", __FUNCTION__, Status));
            gSwmProtocol = NULL;
        } else {
            // Create a handle to use for the high priority power off dialog
            gPriorityHandle = NULL;
            Status = gBS->InstallProtocolInterface (&gPriorityHandle, &gPriorityGuid, EFI_NATIVE_INTERFACE, NULL);
            if (EFI_ERROR (Status)) {
                DEBUG ((DEBUG_ERROR, "ERROR [SwmDialogs]: Failed to create Priority Handle. Code=%r\n", Status));
            } else {
                Status = InitializeUIToolKit(gImageHandle);
                if (EFI_ERROR(Status)) {
                    DEBUG((DEBUG_ERROR, "ERROR [SwmDialogs]: Failed to initialize the UI toolkit (%r).\r\n", Status));
                }
            }
        }
    }
    return Status;
}

/**
  SWM registration notification callback

  @param[in] Event      Event that signalled the callback.
  @param[in] Context    Pointer to an optional event contxt.

  @retval None.

**/
static
VOID
EFIAPI
SwmRegisteredCallback (IN  EFI_EVENT    Event,
                       IN  VOID         *Context) {
    EFI_STATUS Status;

    Status = gBS->LocateProtocol (&gMsSWMProtocolGuid,
                                   gSwmRegistration,
                                   (VOID **)&gSwmProtocol
                                 );
    if (EFI_ERROR(Status)) {
        gSwmProtocol = NULL;
        DEBUG((DEBUG_ERROR, "Unable to locate SWM. Code=%r\n",Status));
    } else {
        Status = AllocateRequiredProtocols ();
        if (EFI_ERROR(Status)) {
            gSwmProtocol = NULL;
            DEBUG((DEBUG_ERROR, "ERROR [SwmDialogs]: Failed to find required protocols (%r).\r\n", Status));
        }
    }

    return;
}

/**
  Constructor for SwmDialogs

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.
  @return Status         From internal routine or boot object

 **/
EFI_STATUS
EFIAPI
SwmDialogsConstructor (
    IN EFI_HANDLE            ImageHandle,
    IN EFI_SYSTEM_TABLE      *SystemTable
    ) {
    EFI_STATUS                          Status;

    Status = gBS->LocateProtocol (&gMsSWMProtocolGuid, NULL, (VOID **) &gSwmProtocol);

    if (EFI_ERROR(Status)) {
        Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL,
                                   TPL_CALLBACK,
                                   SwmRegisteredCallback,
                                   NULL,
                                   &gSwmRegisterEvent
                                  );
        if (EFI_ERROR (Status)) {
            DEBUG((DEBUG_INFO,"%a: Failed to create SWM registration event (%r).\r\n", __FUNCTION__, Status));
        } else {
            Status = gBS->RegisterProtocolNotify (&gMsSWMProtocolGuid,
                                                   gSwmRegisterEvent,
                                                  &gSwmRegistration
                                                 );
            if (EFI_ERROR (Status)) {
                DEBUG((DEBUG_INFO, "%a: Failed to register for SWM registration notifications (%r).\r\n",
                       __FUNCTION__, Status));
            }
        }
    } else {
        AllocateRequiredProtocols ();
    }

    // Register all HII packages.
    //
    gSwmDialogsHiiHandle = HiiAddPackages (&gSwmDialogsHiiPackageListGuid,
                                           gImageHandle,
                                           SwmDialogsLibStrings,
                                           NULL);

    if (NULL == gSwmDialogsHiiHandle)
    {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG((DEBUG_ERROR, "ERROR [SwmDialogs]: Failed to register HII packages (%r).\r\n", Status));
    }

    return EFI_SUCCESS;
}

/**
  Destructor for SwmDialogs

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.
  @return Status         From internal routine or boot object

 **/
EFI_STATUS
EFIAPI
SwmDialogsDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    if (gSwmRegisterEvent != NULL) {
        SystemTable->BootServices->CloseEvent(gSwmRegisterEvent);
    }

    return EFI_SUCCESS;
}
