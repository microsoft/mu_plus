/** @file

  Simple Window Manger (SWM) implementation

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

// *** Pre-processor Constants ***
//

// Password Dialog font sizes.  These represent vertical heights (in pixels) which in turn map to one of the custom fonts
// registered by the simple window manager.
//
#define SWM_PWD_CUSTOM_FONT_BUTTONTEXT_HEIGHT     MsUiGetSmallFontHeight ()
#define SWM_PWD_CUSTOM_FONT_TITLEBAR_HEIGHT       MsUiGetSmallFontHeight ()
#define SWM_PWD_CUSTOM_FONT_CAPTION_HEIGHT        MsUiGetLargeFontHeight ()
#define SWM_PWD_CUSTOM_FONT_BODY_HEIGHT           MsUiGetSmallFontHeight ()
#define SWM_PWD_CUSTOM_FONT_EDITBOX_HEIGHT        MsUiGetFixedFontHeight ()

// Password Dialog layout percentages and padding.  Change these values to adjust relative positions and sizes of dialog controls.
//
#define SWM_PWD_DIALOG_HEIGHT_PERCENT             55          // Password dialog is 55% the height of the screen.
#define SWM_PWD_DIALOG_WIDTH_PERCENT              60          // Password dialog is 60% the width of the screen.
#define SWM_PWD_DIALOG_TITLEBAR_HEIGHT_PERCENT     8          // Password dialog titlebar height is 8% the height of the dialog.
#define SWM_PWD_DIALOG_FRAME_WIDTH_PX             MsUiScaleByTheme (8)   // Password dialog frame width in pixels.
#define SWM_PWD_DIALOG_TITLEBAR_TEXT_X_PERCENT     3          // Password dialog titlebar text x position is 3% the width of the dialog.
#define SWM_PWD_DIALOG_CAPTION_X_PERCENT           4          // Password dialog caption text x position is 4% the width of the dialog.
#define SWM_PWD_DIALOG_CAPTION_Y_PERCENT          10          // Password dialog caption text y position is 10% the height of the dialog.
#define SWM_PWD_DIALOG_RIGHT_PADDING_PERCENT       4          // Password dialog right side padding is 4% of the width of the dialog.
#define SWM_PWD_DIALOG_CONTROL_VERTICAL_PAD_PX    MsUiScaleByTheme (60)  // Password dialog number of vertical pixels between controls.

#define SWM_PWD_DIALOG_FIRST_BUTTON_X_PERCENT     61          // Password dialog first (leftmost) button x position is 61% the width of the screen.
#define SWM_PWD_DIALOG_FIRST_BUTTON_Y_PERCENT     15          // Password dialog first (leftmost) button y position is 15% the height of the screen (from the bottom).
#define SWM_PWD_DIALOG_BUTTONTEXT_PADDING_PX      MsUiScaleByTheme (100) // Password dialog button text left-right padding in pixels.
#define SWM_PWD_DIALOG_BUTTON_ASPECT_RATIO        3           // Password dialog button aspect ration is 1:3 (length:width).
#define SWM_PWD_DIALOG_BUTTON_SPACE_PERCENT       30          // Password dialog button spacing is 30% the width of the largest button.
#define SWM_PWD_DIALOG_MAX_PWD_DISPLAY_CHARS      30          // Password dialog maximum number of password characters to display (editbox size).


// Password Dialog button text.
//
// TODO: These should move to a UNI file so they can later be localized.
//
#define SWM_PWD_OK_TEXT_STRING                    L"OK"
#define SWM_PWD_CANCEL_TEXT_STRING                L"Cancel"
#define SWM_PWD_NEW_PASSWORD_STRING               L"New Password"
#define SWM_PWD_CONFIRM_PASSWORD_STRING           L"Confirm Password"
#define SWM_PWD_PASSWORD_STRING                   L"Password"
#define SWM_PWD_PASSWORDS_DO_NOT_MATCH            L"The provided passwords do not match."


// *** Globals ***
//
// TODO - better way to handle sharing these between the local functions?
//
static EditBox         *mCurrentPassword = NULL;
static EditBox         *mNewPassword = NULL;
static EditBox         *mNewPasswordConfirm = NULL;
static Label           *mErrorLabel;

static MS_ONSCREEN_KEYBOARD_PROTOCOL    *mOSKProtocol;

/*
Initializes the color based on the DIALOG Type.
Currently there are only two. ALERT and default(everything that is not alert)
*/
static
DIALOG_THEME InitializeTheme(
    IN    SWM_PWD_DIALOG_TYPE   Type
    )
{
    DIALOG_THEME DialogTheme;

    switch (Type){
      case SWM_PWD_TYPE_ALERT_PASSWORD:
      {
        DialogTheme.DialogTextColor = gMsColorTable.PasswordDialogTextColor;
        DialogTheme.TitleBarTextColor = gMsColorTable.PasswordDialogTitleBarTextColor;
        DialogTheme.ErrorTextColor = gMsColorTable.PasswordDialogErrorTextColor;
        DialogTheme.EditBoxBackGroundColor = gMsColorTable.PasswordDialogEditBoxBackgroundColor;
        DialogTheme.EditBoxTextColor = gMsColorTable.PasswordDialogEditBoxTextColor;
        DialogTheme.EditBoxGrayOutColor = gMsColorTable.PasswordDialogEditBoxGrayoutColor;
        DialogTheme.EditBoxGrayOutTextColor = gMsColorTable.PasswordDialogEditBoxGrayoutTextColor;
        DialogTheme.DialogTextSelectColor = gMsColorTable.PasswordDialogTextSelectColor;
        DialogTheme.DialogBackGroundColor = gMsColorTable.PasswordDialogBackGroundColor;
        DialogTheme.DialogFrameColor = gMsColorTable.PasswordDialogFrameColor;
        DialogTheme.DialogButtonHoverColor = gMsColorTable.PasswordDialogButtonHoverColor;
        DialogTheme.DialogButtonSelectColor = gMsColorTable.PasswordDialogButtonSelectColor;
        DialogTheme.DialogButtonGrayOutColor = gMsColorTable.PasswordDialogButtonGrayOutColor;
        DialogTheme.DialogButtonRingColor = gMsColorTable.PasswordDialogButtonRingColor;
        DialogTheme.DialogButtonTextColor = gMsColorTable.PasswordDialogButtonTextColor;
        DialogTheme.DialogButtonSelectTextColor = gMsColorTable.PasswordDialogButtonSelectTextColor;
        break;
      }
      default:
      {
        DialogTheme.DialogTextColor = gMsColorTable.DefaultDialogTextColor;
        DialogTheme.TitleBarTextColor = gMsColorTable.DefaultTitleBarTextColor;
        DialogTheme.ErrorTextColor = gMsColorTable.DefaultErrorTextColor;
        DialogTheme.EditBoxBackGroundColor = gMsColorTable.DefaultEditBoxBackGroundColor;
        DialogTheme.EditBoxTextColor = gMsColorTable.DefaultEditBoxTextColor;
        DialogTheme.EditBoxGrayOutColor = gMsColorTable.DefaultEditBoxGrayOutColor;
        DialogTheme.EditBoxGrayOutTextColor = gMsColorTable.DefaultEditBoxGrayOutTextColor;
        DialogTheme.DialogTextSelectColor = gMsColorTable.DefaultDialogTextSelectColor;
        DialogTheme.DialogBackGroundColor = gMsColorTable.DefaultDialogBackGroundColor;
        DialogTheme.DialogFrameColor = gMsColorTable.DefaultDialogFrameColor;
        DialogTheme.DialogButtonHoverColor = gMsColorTable.DefaultDialogButtonHoverColor;
        DialogTheme.DialogButtonSelectColor = gMsColorTable.DefaultDialogButtonSelectColor;
        DialogTheme.DialogButtonGrayOutColor = gMsColorTable.DefaultDialogButtonGrayOutColor;
        DialogTheme.DialogButtonRingColor = gMsColorTable.DefaultDialogButtonRingColor;
        DialogTheme.DialogButtonTextColor = gMsColorTable.DefaultDialogButtonTextColor;
        DialogTheme.DialogButtonSelectTextColor = gMsColorTable.DefaultDialogButtonSelectTextColor;
        break;
       }
    }
    return DialogTheme;
}

/**
    Creates the Password Dialog's canvas and all the hosted child controls.

    NOTE: The controls allocated in this routine are all freed when the canvas is freed after use below.

    @param[in]  this            Pointer to the instance of this driver.
    @param[in]  DialogBounds    Bounding rectangle that defines the canvas dimensions.
    @param[in]  pCaptionText    Dialog Title text.
    @param[in]  pBodyText       Dialog Body text.
    @param[in]  pErrorText      Dialog Error text (optional).
    @param[in]  Type            Contents and behavior of the dialog box.
    @param[out] DialogCanvasOut Pointer to the constructed canvas.

    @retval EFI_SUCCESS         Successfully created the password dialog canvas and controls.

**/
static
EFI_STATUS
CreateDialogControls (IN    MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *this,
                      IN    SWM_RECT                            DialogBounds,
                      IN    CHAR16                              *pCaptionText,
                      IN    CHAR16                              *pBodyText,
                      IN    CHAR16                              *pErrorText,
                      IN    SWM_PWD_DIALOG_TYPE                 Type,
                      IN    DIALOG_THEME                        DialogTheme,
                      OUT   Canvas                              **DialogCanvasOut)
{
    EFI_STATUS      Status                      = EFI_SUCCESS;
    UINT32          DialogOrigX                 = DialogBounds.Left;
    UINT32          DialogOrigY                 = DialogBounds.Top;
    UINT32          DialogWidth                 = (DialogBounds.Right - DialogBounds.Left + 1);
    UINT32          DialogHeight                = (DialogBounds.Bottom - DialogBounds.Top + 1);
    SWM_RECT        StringRect;
    SWM_RECT        ControlBounds;
    UINT32          ControlOrigX, ControlOrigY;
    UINT32          ControlWidth, ControlHeight, MaxGlyphDescent;
    Canvas          *DialogCanvas               = NULL;
    Label           *CaptionLabel               = NULL;
    Label           *BodyLabel                  = NULL;
    Button          *OKButton                   = NULL;
    Button          *CancelButton               = NULL;
    EFI_FONT_INFO   FontInfo;


    // Create a canvas for hosting the password dialog child controls.
    //
    DialogCanvas = new_Canvas (DialogBounds,
                               &DialogTheme.DialogBackGroundColor
                              );

    if (NULL == DialogCanvas)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Calculate the appropriate place to put the dialog's caption text.
    //
    ControlOrigX = (DialogOrigX + ((DialogWidth  * SWM_PWD_DIALOG_CAPTION_X_PERCENT) / 100));
    ControlOrigY = (DialogOrigY + ((DialogHeight * SWM_PWD_DIALOG_CAPTION_Y_PERCENT) / 100));


    // Select an appropriate font and colors for the caption text (larger font than the body).
    //
    FontInfo.FontSize    = SWM_PWD_CUSTOM_FONT_CAPTION_HEIGHT;
    FontInfo.FontStyle   = EFI_HII_FONT_STYLE_NORMAL;
    FontInfo.FontName[0] = L'\0';


    // Draw Password Dialog CAPTION.
    //
    CaptionLabel = new_Label (ControlOrigX,
                              ControlOrigY,
                              (DialogBounds.Right - ControlOrigX - ((DialogWidth * SWM_PWD_DIALOG_CAPTION_X_PERCENT) / 100)),
                              (DialogBounds.Bottom - ControlOrigY),       // In theory we could take up the entire dialog.
                              &FontInfo,
                              &DialogTheme.DialogTextColor,
                              &DialogTheme.DialogBackGroundColor,
                              pCaptionText
                             );

    if (NULL == CaptionLabel)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Add the control to the canvas.
    //
    DialogCanvas->AddControl (DialogCanvas,
                              FALSE,    // Not highlightable.
                              FALSE,    // Not invisible.
                              (VOID *)CaptionLabel
                             );

    CaptionLabel->Base.GetControlBounds (CaptionLabel,
                                         &ControlBounds
                                        );

    // Calculate the appropriate place to put the dialog's body text.
    //
    ControlOrigY += ((ControlBounds.Bottom - ControlBounds.Top + 1) + SWM_PWD_DIALOG_CONTROL_VERTICAL_PAD_PX);


    // Select an appropriate font and colors for the body text.
    //
    FontInfo.FontSize    = SWM_PWD_CUSTOM_FONT_BODY_HEIGHT;
    FontInfo.FontStyle   = EFI_HII_FONT_STYLE_NORMAL;

    // Draw Password Dialog BODY TEXT.
    //
    BodyLabel = new_Label (ControlOrigX,
                           ControlOrigY,
                           (DialogBounds.Right - ControlOrigX - ((DialogWidth * SWM_PWD_DIALOG_RIGHT_PADDING_PERCENT) / 100)),
                           (DialogBounds.Bottom - ControlOrigY),       // In theory we could take up the entire dialog.
                           &FontInfo,
                           &DialogTheme.DialogTextColor,
                           &DialogTheme.DialogBackGroundColor,
                           pBodyText
                          );

    if (NULL == BodyLabel)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Add the control to the canvas.
    //
    DialogCanvas->AddControl (DialogCanvas,
                              FALSE,    // Not highlightable.
                              FALSE,    // Not invisible.
                              (VOID *)BodyLabel
                             );

    BodyLabel->Base.GetControlBounds (BodyLabel,
                                      &ControlBounds
                                     );

    // Calculate the appropriate place to put the dialog's password editbox.
    //
    ControlOrigY += ((ControlBounds.Bottom - ControlBounds.Top + 1) + SWM_PWD_DIALOG_CONTROL_VERTICAL_PAD_PX);


    // Select an appropriate font and colors for the body text.
    //
    FontInfo.FontSize    = SWM_PWD_CUSTOM_FONT_EDITBOX_HEIGHT;
    FontInfo.FontStyle   = EFI_HII_FONT_STYLE_NORMAL;

    switch (Type)
    {
    case SWM_PWD_TYPE_SET_PASSWORD:
        {
            mCurrentPassword     = NULL;

            // Create the editbox for new password input.
            //
            mNewPassword = new_EditBox (ControlOrigX,
                                        ControlOrigY,
                                        SWM_PWD_DIALOG_MAX_PWD_DISPLAY_CHARS,
                                        UIT_EDITBOX_TYPE_PASSWORD,
                                        &FontInfo,
                                        &DialogTheme.EditBoxBackGroundColor,
                                        &DialogTheme.EditBoxTextColor,
                                        &DialogTheme.EditBoxGrayOutColor,
                                        &DialogTheme.EditBoxGrayOutTextColor,
                                        &DialogTheme.DialogTextSelectColor,
                                        SWM_PWD_NEW_PASSWORD_STRING,
                                        NULL
                                       );

            if (NULL == mNewPassword)
            {
                Status = EFI_OUT_OF_RESOURCES;
                goto Exit;
            }

            // Add the control to the canvas.
            //
            DialogCanvas->AddControl (DialogCanvas,
                                      TRUE,     // Highlightable.
                                      FALSE,    // Not invisible.
                                      (VOID *)mNewPassword
                                     );

            mNewPassword->Base.GetControlBounds (mNewPassword,
                                                 &ControlBounds
                                                );

            ControlOrigY += ((ControlBounds.Bottom - ControlBounds.Top + 1) + SWM_PWD_DIALOG_CONTROL_VERTICAL_PAD_PX);

            // Create the editbox for new password confirmation input.
            //
            mNewPasswordConfirm = new_EditBox (ControlOrigX,
                                               ControlOrigY,
                                               SWM_PWD_DIALOG_MAX_PWD_DISPLAY_CHARS,
                                               UIT_EDITBOX_TYPE_PASSWORD,
                                               &FontInfo,
                                               &DialogTheme.EditBoxBackGroundColor,
                                               &DialogTheme.EditBoxTextColor,
                                               &DialogTheme.EditBoxGrayOutColor,
                                               &DialogTheme.EditBoxGrayOutTextColor,
                                               &DialogTheme.DialogTextSelectColor,
                                               SWM_PWD_CONFIRM_PASSWORD_STRING,
                                               NULL
                                              );

            if (NULL == mNewPasswordConfirm)
            {
                Status = EFI_OUT_OF_RESOURCES;
                goto Exit;
            }

            // Add the control to the canvas.
            //
            DialogCanvas->AddControl (DialogCanvas,
                                      TRUE,     // Highlightable.
                                      FALSE,    // Not invisible.
                                      (VOID *)mNewPasswordConfirm
                                     );

            mNewPasswordConfirm->Base.GetControlBounds (mNewPasswordConfirm,
                                                        &ControlBounds
                                                       );

            ControlOrigY += ((ControlBounds.Bottom - ControlBounds.Top + 1) + SWM_PWD_DIALOG_CONTROL_VERTICAL_PAD_PX);
         }
        break;
    case SWM_PWD_TYPE_ALERT_PASSWORD:
    case SWM_PWD_TYPE_PROMPT_PASSWORD:
    default:
        {
            mNewPassword                = NULL;
            mNewPasswordConfirm         = NULL;

            // Create the editbox for current password input.
            //
            mCurrentPassword = new_EditBox (ControlOrigX,
                                            ControlOrigY,
                                            SWM_PWD_DIALOG_MAX_PWD_DISPLAY_CHARS,
                                            UIT_EDITBOX_TYPE_PASSWORD,
                                            &FontInfo,
                                            &DialogTheme.EditBoxBackGroundColor,
                                            &DialogTheme.EditBoxTextColor,
                                            &DialogTheme.EditBoxGrayOutColor,
                                            &DialogTheme.EditBoxGrayOutTextColor,
                                            &DialogTheme.DialogTextSelectColor,
                                            SWM_PWD_PASSWORD_STRING,
                                            NULL
                                           );

            if (NULL == mCurrentPassword)
            {
                Status = EFI_OUT_OF_RESOURCES;
                goto Exit;
            }

            // Add the control to the canvas.
            //
            DialogCanvas->AddControl (DialogCanvas,
                                      TRUE,     // Highlightable.
                                      FALSE,    // Not invisible.
                                      (VOID *)mCurrentPassword
                                     );

            mCurrentPassword->Base.GetControlBounds (mCurrentPassword,
                                                     &ControlBounds
                                                    );

            ControlOrigY += ((ControlBounds.Bottom - ControlBounds.Top + 1) + SWM_PWD_DIALOG_CONTROL_VERTICAL_PAD_PX);
        }
        break;
    }

    // Select an appropriate font and colors for the error text.
    //
    FontInfo.FontSize    = SWM_PWD_CUSTOM_FONT_BODY_HEIGHT;
    FontInfo.FontStyle   = EFI_HII_FONT_STYLE_NORMAL;


    // Draw Password Dialog ERROR TEXT.
    //
    mErrorLabel = new_Label (ControlOrigX,
                             ControlOrigY,
                             (DialogWidth  - ControlOrigX),
                             (DialogOrigY + DialogHeight - ControlOrigY),       // In theory we could take up the entire dialog.
                             &FontInfo,
                             &DialogTheme.ErrorTextColor,
                             &DialogTheme.DialogBackGroundColor,
                             pErrorText
                            );

    if (NULL == mErrorLabel)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Add the control to the canvas.
    //
    DialogCanvas->AddControl (DialogCanvas,
                              FALSE,    // Not highlightable.
                              FALSE,    // Not invisible.
                              (VOID *)mErrorLabel
                             );


    // Select an appropriate font and colors for button text.
    //
    FontInfo.FontSize    = SWM_PWD_CUSTOM_FONT_BUTTONTEXT_HEIGHT;
    FontInfo.FontStyle   = EFI_HII_FONT_STYLE_NORMAL;


    // Calculate the string bitmap size of the largest button text.
    //
    GetTextStringBitmapSize (SWM_PWD_OK_TEXT_STRING,
                             &FontInfo,
                             FALSE,
                             EFI_HII_OUT_FLAG_CLIP |
                             EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                             EFI_HII_IGNORE_LINE_BREAK,
                             &StringRect,
                             &MaxGlyphDescent
                            );

    // Calculate the position and size of the first button.
    //
    ControlWidth    = (StringRect.Right - StringRect.Left + 1);
    ControlHeight   = (StringRect.Bottom - StringRect.Top + 1);
    ControlOrigX    = (DialogOrigX + ((DialogWidth * SWM_PWD_DIALOG_FIRST_BUTTON_X_PERCENT) / 100));
    ControlOrigY    = (DialogOrigY + DialogHeight) - ((DialogHeight * SWM_PWD_DIALOG_FIRST_BUTTON_Y_PERCENT) / 100);

    // Size is the maximum button text length plus padding both before and after.
    //
    ControlWidth    += (SWM_PWD_DIALOG_BUTTONTEXT_PADDING_PX * 2);
    ControlHeight   =  (ControlWidth / SWM_PWD_DIALOG_BUTTON_ASPECT_RATIO);

    // Draw the OK Button.
    //
    OKButton = new_Button (ControlOrigX,
                           ControlOrigY,
                           ControlWidth,
                           ControlHeight,
                           &FontInfo,
                           &DialogTheme.DialogBackGroundColor,            // Normal.
                           &DialogTheme.DialogButtonHoverColor,           // Hover.
                           &DialogTheme.DialogButtonSelectColor,          // Select.
                           &DialogTheme.DialogButtonGrayOutColor,         // GrayOut
                           &DialogTheme.DialogButtonRingColor,            // Button ring.
                           &DialogTheme.DialogButtonTextColor,            // Normal text.
                           &DialogTheme.DialogButtonSelectTextColor,      // Normal text.
                           SWM_PWD_OK_TEXT_STRING,
                           (VOID *)SWM_MB_IDOK          // TODO - not the best way to do this.
                          );

    if (NULL == OKButton)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Add the control to the canvas.
    //
    DialogCanvas->AddControl (DialogCanvas,
                              TRUE,     // Highlightable.
                              FALSE,    // Not invisible.
                              (VOID *)OKButton
                             );


    // Draw the Cancel Button.
    //
    ControlOrigX += (ControlWidth + ((ControlWidth * SWM_PWD_DIALOG_BUTTON_SPACE_PERCENT) / 100));

    CancelButton = new_Button (ControlOrigX,
                               ControlOrigY,
                               ControlWidth,
                               ControlHeight,
                               &FontInfo,
                               &DialogTheme.DialogBackGroundColor,            // Normal.
                               &DialogTheme.DialogButtonHoverColor,           // Hover.
                               &DialogTheme.DialogButtonSelectColor,          // Select.
                               &DialogTheme.DialogButtonGrayOutColor,         // GrayOut
                               &DialogTheme.DialogButtonRingColor,            // Button ring.
                               &DialogTheme.DialogButtonTextColor,            // Normal text.
                               &DialogTheme.DialogButtonSelectTextColor,      // Normal text.
                               SWM_PWD_CANCEL_TEXT_STRING,
                               (VOID *)SWM_MB_IDCANCEL          // TODO - not the best way to do this.
                              );

    if (NULL == CancelButton)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Add the control to the canvas.
    //
    DialogCanvas->AddControl (DialogCanvas,
                              TRUE,     // Highlightable.
                              FALSE,    // Not invisible.
                              (VOID *)CancelButton
                             );

    // Denote the button as the default control (for key input if nothing is highlighted).
    //
    DialogCanvas->SetDefaultControl(DialogCanvas,
        (VOID *)OKButton
        );


    // Set keyboard input focus on the password editbox.
    //
    if (Type == SWM_PWD_TYPE_SET_PASSWORD){
        DialogCanvas->SetHighlight(DialogCanvas,
            mNewPassword
            );
    }
    else{ // current password and alert password type
        DialogCanvas->SetHighlight(DialogCanvas,
            mCurrentPassword
            );
    }

    // Return the pointer to the canvas.
    //
    *DialogCanvasOut = DialogCanvas;

Exit:

    return Status;
}


/**
    Draws the password dialog's outer frame and fills its background.

    @param[in]  FrameRect       Dialog box's outer rectangle.
    @param[in]  CanvasRect      Dialog box's canvas rectangle. This is the "working" area of the dialog.
    @param[in]  pTitleBarText   Dialog's titlebar text.

    @retval EFI_SUCCESS         Successfully drew the dialog frame and background.

**/
static
EFI_STATUS
DrawDialogFrame (IN MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *this,
                 IN SWM_RECT                            FrameRect,
                 IN SWM_RECT                            CanvasRect,
                 IN CHAR16                              *pTitleBarText,
                 IN DIALOG_THEME                        DialogTheme)
{
    EFI_STATUS              Status          = EFI_SUCCESS;
    EFI_FONT_DISPLAY_INFO   StringInfo;
    EFI_IMAGE_OUTPUT        *pBltBuffer;


    // For performance reasons, drawing the frame as four individual (small) rectangles is faster than a single large rectangle.
    //
    this->BltWindow (this,                            // Top
                     gImageHandle,
                     &DialogTheme.DialogFrameColor,
                     EfiBltVideoFill,
                     0,
                     0,
                     FrameRect.Left,
                     FrameRect.Top,
                     (FrameRect.Right - FrameRect.Left + 1),
                     (CanvasRect.Top - FrameRect.Top + 1),
                     0
                    );

    this->BltWindow (this,                            // Left
                     gImageHandle,
                     &DialogTheme.DialogFrameColor,
                     EfiBltVideoFill,
                     0,
                     0,
                     FrameRect.Left,
                     CanvasRect.Top,
                     (CanvasRect.Left - FrameRect.Left + 1),
                     (FrameRect.Bottom - CanvasRect.Top + 1),
                     0
                    );

    this->BltWindow (this,                            // Right
                     gImageHandle,
                     &DialogTheme.DialogFrameColor,
                     EfiBltVideoFill,
                     0,
                     0,
                     CanvasRect.Right,
                     CanvasRect.Top,
                     (FrameRect.Right - CanvasRect.Right + 1),
                     (FrameRect.Bottom - CanvasRect.Top + 1),
                     0
                    );

    this->BltWindow (this,                            // Bottom
                     gImageHandle,
                     &DialogTheme.DialogFrameColor,
                     EfiBltVideoFill,
                     0,
                     0,
                     CanvasRect.Left,
                     CanvasRect.Bottom,
                     (CanvasRect.Right - CanvasRect.Left + 1),
                     (FrameRect.Bottom - CanvasRect.Bottom + 1),
                     0
                    );


    // For performance reasons, the canvas has been designed not to paint the entire dialog background.  Instead it only knows how to clear
    // current child control bounding rectanges.  So we fill in the entire dialog background once, here.
    //
    this->BltWindow (this,
                     gImageHandle,
                     &DialogTheme.DialogBackGroundColor,
                     EfiBltVideoFill,
                     0,
                     0,
                     CanvasRect.Left,
                     CanvasRect.Top,
                     (CanvasRect.Right - CanvasRect.Left + 1),
                     (CanvasRect.Bottom - CanvasRect.Top + 1),
                     0
                    );


    // Draw titlebar text.
    //
    pBltBuffer = (EFI_IMAGE_OUTPUT *) AllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));

    ASSERT (pBltBuffer != NULL);
    if (NULL == pBltBuffer)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    pBltBuffer->Width        = (UINT16)gGop->Mode->Info->HorizontalResolution;
    pBltBuffer->Height       = (UINT16)gGop->Mode->Info->VerticalResolution;
    pBltBuffer->Image.Screen = gGop;

    // Select a font (size & style) and font colors.
    //
    StringInfo.FontInfoMask         = EFI_FONT_INFO_ANY_FONT;
    StringInfo.FontInfo.FontSize    = SWM_PWD_CUSTOM_FONT_TITLEBAR_HEIGHT;
    StringInfo.FontInfo.FontStyle   = EFI_HII_FONT_STYLE_NORMAL;
    StringInfo.FontInfo.FontName[0] = L'\0';

    CopyMem (&StringInfo.ForegroundColor, &DialogTheme.TitleBarTextColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    CopyMem(&StringInfo.BackgroundColor, &DialogTheme.DialogFrameColor, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

    // Determine the size the TitleBar text string will occupy on the screen.
    //
    UINT32      MaxDescent;
    SWM_RECT    StringRect;

    GetTextStringBitmapSize (pTitleBarText,
                             &StringInfo.FontInfo,
                             FALSE,
                             EFI_HII_OUT_FLAG_CLIP |
                             EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                             EFI_HII_IGNORE_LINE_BREAK,
                             &StringRect,
                             &MaxDescent
                            );

    // Render the string to the screen, vertically centered.
    //
    UINT32  FrameWidth      = (FrameRect.Right - FrameRect.Left + 1);
    UINT32  TitleBarHeight  = (CanvasRect.Top - FrameRect.Top + 1);

    this->StringToWindow (this,
                          gImageHandle,
                          EFI_HII_OUT_FLAG_CLIP |
                          EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                          EFI_HII_IGNORE_LINE_BREAK | EFI_HII_DIRECT_TO_SCREEN,
                          pTitleBarText,
                          &StringInfo,
                          &pBltBuffer,
                          (FrameRect.Left + ((FrameWidth * SWM_PWD_DIALOG_TITLEBAR_TEXT_X_PERCENT) / 100)),
                          (FrameRect.Top  + ((TitleBarHeight / 2) - ((StringRect.Bottom - StringRect.Top + 1) / 2)) + MaxDescent),  // Vertically center in the titlebar.
                          NULL,
                          NULL,
                          NULL);

Exit:

    if (NULL != pBltBuffer)
    {
        FreePool(pBltBuffer);
    }

    return Status;
}


/**
    Creates the password dialog, canvas, and all child controls.

    @param[in]  this            Pointer to the instance of this driver.
    @param[in]  FrameRect       Bounding rectangle that defines the canvas dimensions.
    @param[in]  pTitleBarText   Dialog titlebar text.
    @param[in]  pCaptionText    Dialog caption text.
    @param[in]  pBodyText       Dialog body text.
    @param[in]  pErrorText      Dialog error text (optional).
    @param[in]  Type            Contents and behavior of the dialog box.
    @param[out] DialogCanvasOut Pointer to the constructed canvas.

    @retval EFI_SUCCESS         Successfully created the password dialog canvas and controls.

**/
static
EFI_STATUS
CreatePasswordDialog (IN    MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *this,
                      IN    SWM_RECT                            FrameRect,
                      IN    CHAR16                              *pTitleBarText,
                      IN    CHAR16                              *pCaptionText,
                      IN    CHAR16                              *pBodyText,
                      IN    CHAR16                              *pErrorText,
                      IN    SWM_PWD_DIALOG_TYPE                 Type,
                      IN    DIALOG_THEME                        DialogTheme,
                      OUT   Canvas                              **DialogCanvasOut)
{
    EFI_STATUS  Status          = EFI_SUCCESS;
    UINT32      DialogHeight    = (FrameRect.Bottom - FrameRect.Top + 1);
    SWM_RECT    CanvasRect;

    // Since we have a dialog titlebar and frame, the actual canvas area of the dialog is smaller.
    //
    CanvasRect.Left     = (FrameRect.Left + SWM_PWD_DIALOG_FRAME_WIDTH_PX);
    CanvasRect.Top      = (FrameRect.Top  + ((DialogHeight * SWM_PWD_DIALOG_TITLEBAR_HEIGHT_PERCENT) / 100));
    CanvasRect.Right    = (FrameRect.Right - SWM_PWD_DIALOG_FRAME_WIDTH_PX);
    CanvasRect.Bottom   = (FrameRect.Bottom - SWM_PWD_DIALOG_FRAME_WIDTH_PX);

    // Create a canvas and all of the child controls that make up the Password Dialog.
    //
    Status = CreateDialogControls (this,
                                   CanvasRect,
                                   pCaptionText,
                                   pBodyText,
                                   pErrorText,
                                   Type,
                                   DialogTheme,
                                   DialogCanvasOut     // Use the caller's parameter to store the canvas pointer.
                                  );

    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [SWM]: Failed to create Password Dialog controls (%r).\r\n", Status));
        goto Exit;
    }

    // Draw the dialog body and frame.
    //
    DrawDialogFrame (this,
                     FrameRect,
                     CanvasRect,
                     pTitleBarText,
                     DialogTheme
                    );

Exit:

    return Status;
}


/**
    Processes user input (i.e., keyboard, touch, and mouse) and interaction with the Password Dialog.

    @param[in]  this            Pointer to the instance of this driver.
    @param[in]  FrameRect       Dialog box outer rectangle.
    @param[in]  DialogCanvas    Dialog box canvas hosting all the child controls.
    @param[in]  TitleBarText    Dialog box titlebar text.
    @param[in]  PointerProtocol AbsolutePointerProtocol.
    @param[in]  Type            Contents and behavior of the dialog box.
    @param[out] PasswordString  Pointer to the password string.

    @retval EFI_SUCCESS         Successfully processed password dialog input.

**/
static
SWM_MB_RESULT
ProcessDialogInput (IN  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *this,
                    IN  SWM_RECT                            FrameRect,
                    IN  Canvas                              *DialogCanvas,
                    IN  CHAR16                              *pTitleBarText,
                    IN  EFI_ABSOLUTE_POINTER_PROTOCOL       *PointerProtocol,
                    IN  SWM_PWD_DIALOG_TYPE                 Type,
                    IN  DIALOG_THEME                        DialogTheme,
                    OUT CHAR16                              **PasswordString)
{
    EFI_STATUS          Status              = EFI_SUCCESS;
    BOOLEAN             DefaultPosition     = TRUE;
    UINTN               Index;
    OBJECT_STATE        State               = NORMAL;
    SWM_MB_RESULT       ButtonResult        = 0;
    VOID                *pContext           = NULL;
    SWM_INPUT_STATE     InputState;
#define PROCESS_DIALOG_NUM_EVENTS 2
    EFI_EVENT           WaitEvents[PROCESS_DIALOG_NUM_EVENTS];

    // Wait for user input.
    //
    WaitEvents[0] = gSimpleTextInEx->WaitForKeyEx;
    WaitEvents[1] = PointerProtocol->WaitForInput;

    ButtonResult = 0;
    ZeroMem (&InputState, sizeof(SWM_INPUT_STATE));

    do
    {
        // Render the canvas and all child controls.
        //
        State = DialogCanvas->Base.Draw (DialogCanvas,
                                         FALSE,
                                         &InputState,
                                         &pContext
                                        );

        // Keyboard input focus means the user used touch/mouse to select one of the edit boxes, display the on-screen keyboard.
        //
        if (KEYFOCUS == State && NULL != mOSKProtocol && TRUE == DefaultPosition)
        {
            SWM_RECT    CanvasRect;
            SWM_RECT    OSKRect;


            // Set client state inactive (messages will by default go to the default client).
            //
            this->ActivateWindow (this,
                                  gImageHandle,
                                  FALSE);

            // Get the current canvas bounding rectangle.
            //
            DialogCanvas->Base.GetControlBounds (DialogCanvas,
                                                 &CanvasRect
                                                );

            // Get the OSK bounding rectangle.
            //
            mOSKProtocol->GetKeyboardBounds (mOSKProtocol,
                                             &OSKRect
                                            );

            // Calculate the vertical delta needed to center the dialog between the top of the screen and the OSK, shift everything up by that amount.  The OSK
            // is docked, centered, at the bottom of the screen.
            //
            UINT32  VertOffset = (FrameRect.Top - ((OSKRect.Top / 2) - ((FrameRect.Bottom - FrameRect.Top + 1) / 2)));

            FrameRect.Top       -= VertOffset;
            FrameRect.Bottom    -= VertOffset;
            CanvasRect.Top      -= VertOffset;
            CanvasRect.Bottom   -= VertOffset;

            // Set the window manager focus area bounding rectangle.
            //
            this->SetWindowFrame (this,
                                  gImageHandle,
                                  &FrameRect);

            // Set window manager client state active.
            //
            this->ActivateWindow (this,
                                  gImageHandle,
                                  TRUE);

            // Draw the dialog body and frame at the new location.
            //
            DrawDialogFrame (this,
                             FrameRect,
                             CanvasRect,
                             pTitleBarText,
                             DialogTheme
                            );

            // Move the canvas and all existing child controls up.
            //
            DialogCanvas->Base.SetControlBounds (DialogCanvas,
                                                 CanvasRect
                                                );

            // Show the on-screen keyboard for input.
            //
            mOSKProtocol->ShowKeyboard (mOSKProtocol, TRUE);

            // Render the canvas and continue processing input.
            //
            State = DialogCanvas->Base.Draw (DialogCanvas,
                                             FALSE,
                                             NULL,
                                             &pContext
                                            );

            // Indicate that the dialog has been moved up to make room for the OSK.
            //
            DefaultPosition = FALSE;
        }

        // If one of the controls indicated they were selected, take action.  Grab the associated context and if a button
        // was selected, decide the action to be taken.
        //
        if (SELECT == State)
        {
            // Determine which button was pressed by the context returned.
            //
            // TODO - avoid having to cast a constant value from a pointer.
            //
            ButtonResult = (SWM_MB_RESULT)(UINTN)pContext;

            // If user selected cancel, exit.
            //
            if (SWM_MB_IDCANCEL == ButtonResult)
            {
                *PasswordString = NULL;
                break;
            }

            // If the user selected OK and is trying to set a new password, make sure both the new password and the
            // confirmation match.
            //
            if (SWM_MB_IDOK == ButtonResult && SWM_PWD_TYPE_SET_PASSWORD == Type)
            {
                // If both passwords are NULL, it's meant to be cleared.
                //
                if (L'\0' == mNewPassword->GetCurrentTextString(mNewPassword)[0] && L'\0' == mNewPasswordConfirm->GetCurrentTextString(mNewPasswordConfirm)[0])
                {
                    // Done.
                    //
                    *PasswordString = NULL;
                    break;
                }

                // If the two passwords don't match, request from the user again.
                //
                if (0 != StrCmp(mNewPassword->GetCurrentTextString(mNewPassword), mNewPasswordConfirm->GetCurrentTextString(mNewPasswordConfirm)))
                {
                    // Clear the canvas.
                    //
                    DialogCanvas->ClearCanvas (DialogCanvas);

                    // Passwords don't match.  Clear the edit boxes and try again.
                    //
                    mNewPassword->ClearEditBox (mNewPassword);
                    mNewPasswordConfirm->ClearEditBox (mNewPasswordConfirm);

                    // Clear the user input state.
                    //
                    ButtonResult = 0;
                    ZeroMem (&InputState, sizeof(SWM_INPUT_STATE));

                    // Set focus on the first editbox again.
                    //
                    DialogCanvas->SetHighlight (DialogCanvas,
                                                mNewPassword
                                               );

                    // Update the error text label to tell the user what happened.
                    //
                    mErrorLabel->UpdateLabelText (mErrorLabel,
                                                  SWM_PWD_PASSWORDS_DO_NOT_MATCH
                                                 );

                    // Try again...
                    //
                    continue;
                }
            }


            // For anything else, we allocate storage to pass the password string back to the caller.  Note that
            // this should be the only allocated buffer to hold the password string and it should be freed by the caller
            // as soon as possible.
            //
            EditBox *TempEditBox    = (SWM_PWD_TYPE_SET_PASSWORD == Type ? mNewPassword : mCurrentPassword);
            CHAR16  *EditBoxBuffer  = TempEditBox->GetCurrentTextString (TempEditBox);

            if (NULL != EditBoxBuffer)
            {
                UINT32 StringSize = (UINT32)((StrLen(EditBoxBuffer) + 1) * sizeof(CHAR16));
                *PasswordString = (CHAR16 *)AllocateZeroPool(StringSize);

                ASSERT(NULL != *PasswordString);
                if (NULL != *PasswordString)
                {
                    StrCpyS(*PasswordString, StrLen(EditBoxBuffer) + 1, EditBoxBuffer);
                }
            }

            // Now that we're ready to return, let's clear out the temporary buffers.
            //
            if (SWM_PWD_TYPE_SET_PASSWORD == Type)
            {
              mNewPassword->WipeBuffer (mNewPassword);
              mNewPasswordConfirm->WipeBuffer (mNewPasswordConfirm);
            }
            else
            {
              mCurrentPassword->WipeBuffer (mCurrentPassword);
            }

            // Exit.
            //
            break;
        }

        while (EFI_SUCCESS == Status)
        {
            // Wait for user input.
            //
            Index = 0;
            Status = this->WaitForEvent (PROCESS_DIALOG_NUM_EVENTS,  // Two events (pointer and keyboard).
                                         WaitEvents,
                                        &Index,
                                         0,
                                         FALSE);                         // No Timeout

            if (EFI_SUCCESS == Status && 0 == Index)
            {
                // Received KEYBOARD input.
                //
                InputState.InputType = SWM_INPUT_TYPE_KEY;

                // Read key press data.
                //
                Status = gSimpleTextInEx->ReadKeyStrokeEx (gSimpleTextInEx,
                                                           &InputState.State.KeyState
                                                          );

                // If the user pressed ESC, exit without doing anything.
                //
                if (SCAN_ESC == InputState.State.KeyState.Key.ScanCode)
                {
                    ButtonResult = SWM_MB_IDCANCEL;
                    break;
                }

                // If user pressed SHIFT-TAB, move the highlight to the previous control.
                //
                if (CHAR_TAB == InputState.State.KeyState.Key.UnicodeChar && 0 != (InputState.State.KeyState.KeyState.KeyShiftState & (EFI_LEFT_SHIFT_PRESSED | EFI_RIGHT_SHIFT_PRESSED)))
                {
                    // Send the key to the form canvas for processing.
                    //
                    Status = DialogCanvas->MoveHighlight (DialogCanvas,
                                                          FALSE
                                                         );

                    // If the highlight moved past the top control, clear control highlight and try again - this will wrap the highlight around
                    // to the bottom.  The reason we don't do this automatically is because in other
                    // scenarios, the TAB order needs to include controls outside the canvas (ex:
                    // the Front Page's Top-Menu.
                    //
                    if (EFI_NOT_FOUND == Status)
                    {
                        DialogCanvas->ClearHighlight (DialogCanvas);

                        Status = DialogCanvas->MoveHighlight (DialogCanvas,
                                                              FALSE
                                                             );
                    }

                    continue;
                }

                // If user pressed TAB, move the highlight to the next control.
                //
                if (CHAR_TAB == InputState.State.KeyState.Key.UnicodeChar)
                {
                        // Send the key to the form canvas for processing.
                        //
                        Status = DialogCanvas->MoveHighlight (DialogCanvas,
                                                              TRUE
                                                             );

                        // If we moved the highlight to the end of the list of controls, move it back
                        // to the top by clearing teh current highlight and moving to next.  The reason we don't do
                        // this automatically is because in other scenarios, the TAB order needs to include controls
                        // outside the canvas (ex: the Front Page's Top-Menu.
                        //
                        if (EFI_NOT_FOUND == Status)
                        {
                            DialogCanvas->ClearHighlight (DialogCanvas);

                            Status = DialogCanvas->MoveHighlight (DialogCanvas,
                                                                  TRUE
                                                                 );
                        }

                        continue;
                }

                break;
            }
            else if (EFI_SUCCESS == Status && 1 == Index)
            {
                // Received TOUCH input.
                //

                static BOOLEAN  WatchForFirstFingerUpEvent = FALSE;
                BOOLEAN         WatchForFirstFingerUpEvent2;

                InputState.InputType = SWM_INPUT_TYPE_TOUCH;

                Status = PointerProtocol->GetState (PointerProtocol,
                                                   &InputState.State.TouchState
                                                     );

                // Filter out all extra pointer moves with finger UP.
                WatchForFirstFingerUpEvent2 = WatchForFirstFingerUpEvent;
                WatchForFirstFingerUpEvent = SWM_IS_FINGER_DOWN (InputState.State.TouchState);
                if (!SWM_IS_FINGER_DOWN (InputState.State.TouchState) && (FALSE == WatchForFirstFingerUpEvent2))
                {
                    continue;
                }
                break;
            }
        }

    } while (0 == ButtonResult && EFI_SUCCESS == Status);


    return (ButtonResult);
}

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
                        OUT CHAR16                              **Password)
{
    EFI_STATUS                      Status                      = EFI_SUCCESS;
    UINT32                          ScreenWidth,  ScreenHeight;
    UINT32                          DialogWidth,  DialogHeight;
    UINT32                          DialogOrigX,  DialogOrigY;
    Canvas                          *DialogCanvas               = NULL;
    SWM_RECT                        FrameRect;
    EFI_ABSOLUTE_POINTER_PROTOCOL   *PointerProtocol;
    EFI_EVENT                       PaintEvent                  = NULL;
    CHAR16                          *PasswordString             = NULL;
    DIALOG_THEME                    DialogTheme;


    // Validate caller arguments.
    //
    if (NULL == Password || NULL == Result)
    {
        Status = EFI_INVALID_PARAMETER;
        goto Exit;
    }

    // Get the current display resolution and use it to determine the size of the password dialog.
    //
    ScreenWidth     =   gGop->Mode->Info->HorizontalResolution;
    ScreenHeight    =   gGop->Mode->Info->VerticalResolution;

    DialogWidth     =   ((ScreenWidth  * SWM_PWD_DIALOG_WIDTH_PERCENT)  / 100);
    DialogHeight    =   ((ScreenHeight * SWM_PWD_DIALOG_HEIGHT_PERCENT) / 100);

    // Calculate the location of the password dialog's upper left corner origin (center of the screen vertically).  This
    // is the default location when the on-screen keyboard (OSK) isn't being displayed.
    //
    DialogOrigX = ((ScreenWidth  / 2)  - (DialogWidth / 2));
    DialogOrigY = ((ScreenHeight / 2) - (DialogHeight / 2));

    // Calculate the dialog's outer rectangle.  Note that the dialog may need to co-exist with the OSK for input so we
    // need to share screen real estate and therefore cooperate for pointer event input.  When the OSK is displayed, the
    // password dialog will be shifted up vertically to make room.
    //
    FrameRect.Left      = DialogOrigX;
    FrameRect.Top       = DialogOrigY;
    FrameRect.Right     = (DialogOrigX + DialogWidth - 1);
    FrameRect.Bottom    = (DialogOrigY + DialogHeight - 1);

    // Locate the on-screen keyboard (OSK) protocol.  It may be used for input on a touch-only device.
    //
    if (NULL == mOSKProtocol)
    {
        Status = gBS->LocateProtocol (&gMsOSKProtocolGuid,
                                      NULL,
                                      (VOID **)&mOSKProtocol

                                     );
        if (EFI_ERROR (Status))
        {
            DEBUG((DEBUG_WARN, "WARN [SWM]: Failed to locate on-screen keyboard protocol (%r).\r\n", Status));
            mOSKProtocol = NULL;
        }
    }

    if (NULL != mOSKProtocol)
    {
        // Configure the OSK position, size, and configuration (85% of screen width, bottom center position, docked).
        //
        mOSKProtocol->ShowKeyboard (mOSKProtocol, FALSE);
        mOSKProtocol->ShowKeyboardIcon (mOSKProtocol, FALSE);
        mOSKProtocol->SetKeyboardSize (mOSKProtocol, 85);
        mOSKProtocol->SetKeyboardPosition (mOSKProtocol, BottomCenter, Docked);
        mOSKProtocol->ShowDockAndCloseButtons (mOSKProtocol, FALSE);
    }

    // Register with the Simple Window Manager to get mouse and touch input events.
    //
    Status = this->RegisterClient(this,
                                  gImageHandle,
                                  SWM_Z_ORDER_POPUP,
                                  &FrameRect,
                                  NULL,
                                  NULL,
                                  &PointerProtocol,
                                  &PaintEvent);

    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [PasswordDlg]: Failed to register the password dialog as a client: %r.\r\n", Status));
        goto Exit;
    }

    // Set window manager client state active.
    //
    this->ActivateWindow (this,
                          gImageHandle,
                          TRUE);

    // Enable the mouse pointer to be displayed if a USB mouse or Blade trackpad is attached and is moved.
    //
    this->EnableMousePointer (this,
                              TRUE);
//Initialize Color Theme

    DialogTheme = InitializeTheme(Type);

    // Create the password dialog and all its child controls at the specified screen location & size.
    //
    Status = CreatePasswordDialog (this,
                                   FrameRect,
                                   pTitleBarText,
                                   pCaptionText,
                                   pBodyText,
                                   pErrorText,
                                   Type,
                                   DialogTheme,
                                   &DialogCanvas
                                  );

    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [SWM]: Failed to create Password Dialog: %r.\r\n", Status));
        goto Exit;
    }

    // Process user input and obtain the user password string.
    //
    *Result = ProcessDialogInput (this,
                                  FrameRect,
                                  DialogCanvas,
                                  pTitleBarText,
                                  PointerProtocol,
                                  Type,
                                  DialogTheme,
                                  &PasswordString
                                 );

    if (SWM_MB_IDOK == *Result)
    {
        *Password = PasswordString;
    }

    // Set client state inactive (messages will by default go to the default client).
    //
    this->ActivateWindow (this,
                          gImageHandle,
                          FALSE);

Exit:

    // Hide the keyboard (if it was being displayed).
    //
    if (NULL != mOSKProtocol)
    {
        // Hide the on-screen keyboard (if we were showing it).
        //
        mOSKProtocol->ShowKeyboard (mOSKProtocol, FALSE);
    }

    // Unregister with the window manager as a client.
    //
    this->UnregisterClient (this,
                            gImageHandle);

    // Clean-up.
    //

    // Free the canvas (and all child controls it's hosting).
    //
    if (NULL != DialogCanvas)
    {
        delete_Canvas (DialogCanvas);
    }

    return Status;
}

