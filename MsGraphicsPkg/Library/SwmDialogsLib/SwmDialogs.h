/** @file

  Local include file for SwmDialogs.

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

#ifndef __SWM_DIALOGS_INTERNAL_H__
#define __SWM_DIALOGS_INTERNAL_H__

#include <Uefi.h>

#include <Protocol/GraphicsOutput.h>
#include <Protocol/HiiFont.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/SimpleWindowManager.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MsColorTableLib.h>
#include <Library/MsUiThemeLib.h>
#include <Protocol/OnScreenKeyboard.h>
#include <Library/SwmDialogsLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <UIToolKit/SimpleUIToolKit.h>
#include <UIToolKit/Label.h>

extern EFI_GRAPHICS_OUTPUT_PROTOCOL      *gGop;
extern EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *gSimpleTextInEx;
extern EFI_HANDLE                         gPriorityHandle;
extern EFI_HII_HANDLE                     gSwmDialogsHiiHandle;

// PRIORITY Protocol Guid
#define SWM_PRIORITY_PROTOCOL_GUID  /* 567d4f03-6ff1-45cd-8fc5-9f192bc1450a */     \
{                                                                                  \
    0x567d4f03, 0x6ff1, 0x45cd, { 0x8f, 0xc5, 0x9f, 0x19, 0x2b, 0xc1, 0x45, 0x0a } \
}

typedef struct{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       DialogTextColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       TitleBarTextColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       ErrorTextColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       EditBoxBackGroundColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       EditBoxTextColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       EditBoxGrayOutColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       EditBoxGrayOutTextColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       DialogTextSelectColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       DialogBackGroundColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       DialogFrameColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       DialogButtonHoverColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       DialogButtonSelectColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       DialogButtonGrayOutColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       DialogButtonRingColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       DialogButtonTextColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       DialogButtonSelectTextColor;
} DIALOG_THEME;

/**
    Displays a modal dialog box that contains a set of buttons and a brief use-specific message such as a
    prompt or status information.  The message box returns an integer value that indicates which button
    the user clicked.

    @param[in]  this            Pointer to the instance of this driver.
    @param[in]  pText           Message to be displayed.
    @param[in]  pCaption        Dialog box title.
    @param[in]  Type            Contents and behavior of the dialog box.
    @param[in]  Timeout         Timeout value (0 = No Timeout)

    @retval EFI_SUCCESS      Successfully set the window frame.

**/
EFI_STATUS
MessageBoxInternal (IN  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL     *this,
                    IN  CHAR16                                *pTitleBarText,
                    IN  CHAR16                                *pText,
                    IN  CHAR16                                *pCaption,
                    IN  UINT32                                 Type,
                    IN  UINT64                                Timeout,
                    OUT SWM_MB_RESULT                         *Result);

/**
    Displays a modal dialog box used to prompt for or to confirm a password. The password dialog returns a typed integer value
    that indicates which button the user clicked along with the password string provided.

    NOTE: Password dialog layout is designed for high resolution displays and won't necessarily look good at lower resolutions.

    @param[in]  this            Pointer to the instance of this driver.
    @param[in]  pTitleBarText   Dialog title bar text.
    @param[in]  pCaptionText    Dialog box title.
    @param[in]  pBodyText       Dialog message text.
    @param[in]  pErrorText      (OPTIONAL) Error message message text.
    @param[in]  Type            Contents and behavior of the dialog box.
    @param[out] Result          Button selection result.
    @param[out] Password        Pointer to the password string.

    @retval EFI_SUCCESS         Successfully processed password dialog input.

**/
EFI_STATUS
PasswordDialogInternal (IN  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *this,
                        IN  CHAR16                              *pTitleBarText,
                        IN  CHAR16                              *pCaptionText,
                        IN  CHAR16                              *pBodyText,
                        IN  CHAR16                              *pErrorText,
                        IN  SWM_PWD_DIALOG_TYPE                 Type,
                        OUT SWM_MB_RESULT                       *Result,
                        OUT CHAR16                              **Password);

/**
    Displays a modal dialog box that presents a list of choices to the user and allows them to select
    an option. Title, caption, and body text are customizable, as are the options in the list. Dialog
    will contain a submit and a cancel button and will inform the user of which button was pressed.

    @param[in]  This            Pointer to the instance of this driver.
    @param[in]  pTitleBarText   Dialog title bar text.
    @param[in]  pCaption        Dialog box title.
    @param[in]  pBodyText       Message to be displayed.
    @param[in]  ppOptionsList   Pointer to a list of string pointers that will be used as options.
    @param[in]  OptionsCount    The number of options in the list.
    @param[out] Result          Button selection result.
    @param[out] SelectedIndex   An index of the selected option.

    @retval EFI_SUCCESS         Successfully processed simple list dialog input.

**/
EFI_STATUS
SingleSelectDialogInternal (IN  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *This,
                            IN  CHAR16                              *pTitleBarText,
                            IN  CHAR16                              *pCaptionText,
                            IN  CHAR16                              *pBodyText,
                            IN  CHAR16                              **ppOptionsList,
                            IN  UINTN                               OptionsCount,
                            OUT SWM_MB_RESULT                       *Result,
                            OUT UINTN                               *SelectedIndex);

/**
Displays a modal dialog box used to prompt for or to confirm a password. The password dialog returns a typed integer value
that indicates which button the user clicked along with the password string provided.

NOTE: Password dialog layout is designed for high resolution displays and won't necessarily look good at lower resolutions.

@param[in]  this            Pointer to the instance of this driver.
@param[in]  pTitleBarText   Dialog title bar text.
@param[in]  pCaptionText    Dialog box title.
@param[in]  pBodyText       Dialog message text.
@param[in]  pErrorText      (OPTIONAL) Error message message text.
@param[in]  Type            Contents and behavior of the dialog box.
@param[out] Result          Button selection result.
@param[out] Password        (OPTIONAL) Pointer to the password string.
@param[out] Thumbprint      (OPTIONAL) Pointer to the Thumbprint string.

@retval EFI_SUCCESS         Successfully processed password dialog input.

**/
EFI_STATUS
VerifyThumbprintInternal(IN  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *this,
                         IN  CHAR16                              *pTitleBarText,
                         IN  CHAR16                              *pCaptionText,
                         IN  CHAR16                              *pBodyText,
                         IN  CHAR16                              *pCertText,
                         IN  CHAR16                              *pConfirmText,
                         IN  CHAR16                              *pErrorText,
                         IN  SWM_PWD_DIALOG_TYPE                 Type,
                         OUT SWM_MB_RESULT                       *Result,
                         OUT CHAR16                              **Password OPTIONAL,
                         OUT CHAR16                              **Thumbprint OPTIONAL);

#endif  // __SWM_DIALOGS_INTERNAL_H__
