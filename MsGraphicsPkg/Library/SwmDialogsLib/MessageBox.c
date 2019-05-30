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

// Dialog font sizes.  These represent vertical heights (in pixels) which in turn map to one of the custom fonts
// registered by the simple window manager.
//
#define SWM_MB_CUSTOM_FONT_BUTTONTEXT_HEIGHT    MsUiGetSmallFontHeight ()
#define SWM_MB_CUSTOM_FONT_TITLEBAR_HEIGHT      MsUiGetSmallFontHeight ()
#define SWM_MB_CUSTOM_FONT_CAPTION_HEIGHT       MsUiGetLargeFontHeight ()
#define SWM_MB_CUSTOM_FONT_BODY_HEIGHT          MsUiGetSmallFontHeight ()

// Dialog layout percentages and padding.  Change these values to adjust relative positions and sizes of dialog controls.
//
#define SWM_MB_DIALOG_HEIGHT_PERCENT            55          // Dialog is 55% the height of the screen.
#define SWM_MB_DIALOG_WIDTH_PERCENT             60          // Dialog is 60% the width of the screen.
#define SWM_MB_DIALOG_TITLEBAR_HEIGHT_PERCENT    8          // Dialog titlebar height is 8% the height of the dialog.
#define SWM_MB_DIALOG_FRAME_WIDTH_PX            MsUiScaleByTheme (8)   // Dialog frame width in pixels.
#define SWM_MB_DIALOG_TITLEBAR_TEXT_X_PERCENT    3          // Dialog titlebar text x position is 3% the width of the dialog.
#define SWM_MB_DIALOG_CAPTION_X_PERCENT          4          // Dialog caption text x position is 4% the width of the dialog.
#define SWM_MB_DIALOG_CAPTION_Y_PERCENT         10          // Dialog caption text y position is 10% the height of the dialog.
#define SWM_MB_DIALOG_RIGHT_PADDING_PERCENT      4          // Dialog right side padding is 4% of the width of the dialog.
#define SWM_MB_DIALOG_CONTROL_VERTICAL_PAD_PX   MsUiScaleByTheme (60)  // Dialog number of vertical pixels between controls.
#define SWM_MB_DIALOG_PRIORITY_OFFSET_PERCENT    3          // Dialog number of diagonal pixels to offset the Priority Dialog.

#define SWM_MB_DIALOG_FIRST_BUTTON_X_PERCENT    61          // Dialog first (leftmost) button x position is 61% the width of the screen.
#define SWM_MB_DIALOG_FIRST_BUTTON_Y_PERCENT     7          // Dialog first (leftmost) button y position is  7% the height of the screen (from the bottom to bottom of control).
#define SWM_MB_DIALOG_BUTTONTEXT_PADDING_PX     MsUiScaleByTheme (100)  // Dialog button text left-right padding in pixels.
#define SWM_MB_DIALOG_BUTTON_ASPECT_RATIO       3           // Dialog button aspect ration is 1:3 (length:width).
#define SWM_MB_DIALOG_BUTTON_SPACE_PERCENT      30          // Dialog button spacing is 30% the width of the largest button.


/**
    Creates the Dialog's canvas and all the hosted child controls.

    NOTE: The controls allocated in this routine are all freed when the canvas is freed after use below.

    @param[in]  this            Pointer to the instance of this driver.
    @param[in]  DialogBounds    Bounding rectangle that defines the canvas dimensions.
    @param[in]  pCaptionText    Dialog Title text.
    @param[in]  pBodyText       Dialog Body text.
    @param[out] DialogCanvasOut Pointer to the constructed canvas.

    @retval EFI_SUCCESS         Successfully created the dialog canvas and controls.

**/
static EFI_STATUS
CreateDialogControls (IN    MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *this,
                      IN    SWM_RECT                            DialogBounds,
                      IN    CHAR16                              *pCaptionText,
                      IN    CHAR16                              *pBodyText,
                      IN    UINT32                              Type,
                      IN    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *BackgroundColor,
                      IN    EFI_HANDLE                          MessageBoxHandle,
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
    Button          *Button1                    = NULL;
    Button          *Button2                    = NULL;
    EFI_FONT_INFO   FontInfo;
    // Locals to support MessageBox types.
    // NOTE: Button1 is the rightmost button. Button2 is optional.
    UINT32          BaseType;
    CHAR16          *LongestButtonText = NULL, *Button1Text = NULL, *Button2Text = NULL;
    SWM_MB_RESULT   Button1Code, Button2Code = 0;
    Button          *DefaultButton;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *ButtonSelectColor, *ButtonHoverColor;

    // Handle button type considerations.
    //
    BaseType = SWM_MB_BASE_TYPE(Type);
    switch (BaseType)
    {
      case SWM_MB_OKCANCEL:
      case SWM_MB_RETRYCANCEL:
        // Load the Cancel as Button1 (the rightmost button).
        Button1Text = (CHAR16 *)HiiGetString (gSwmDialogsHiiHandle, STRING_TOKEN (STR_GENERIC_CANCEL_STRING), NULL);
        Button1Code = SWM_MB_IDCANCEL;
        if (SWM_MB_RETRYCANCEL == BaseType)
        {
          // Load the Retry as Button2 (the leftmost button in a two button config).
          Button2Text = (CHAR16 *)HiiGetString (gSwmDialogsHiiHandle, STRING_TOKEN (STR_GENERIC_RETRY_STRING), NULL);
          Button2Code = SWM_MB_IDRETRY;
        }
        else
        {
          // Load the Ok as Button2 (the leftmost button in a two button config).
          Button2Text = (CHAR16 *)HiiGetString (gSwmDialogsHiiHandle, STRING_TOKEN (STR_GENERIC_OK_STRING), NULL);
          Button2Code = SWM_MB_IDOK;
        }
        break;

      case SWM_MB_CANCELNEXT:
          // Load the Next as Button1 (the rightmost button in a two button config).
          Button1Text = (CHAR16 *)HiiGetString (gSwmDialogsHiiHandle, STRING_TOKEN (STR_GENERIC_NEXT_STRING), NULL);
          Button1Code = SWM_MB_IDNEXT;
          Button2Text = (CHAR16 *)HiiGetString (gSwmDialogsHiiHandle, STRING_TOKEN (STR_GENERIC_CANCEL_STRING), NULL);
          Button2Code = SWM_MB_IDCANCEL;
        break;

      case SWM_MB_CANCEL:
        Button1Text = (CHAR16 *)HiiGetString (gSwmDialogsHiiHandle, STRING_TOKEN (STR_GENERIC_CANCEL_STRING), NULL);
        Button1Code = SWM_MB_IDCANCEL;
        break;

      case SWM_MB_RESTART:
        Button1Text = (CHAR16 *)HiiGetString (gSwmDialogsHiiHandle, STRING_TOKEN (STR_GENERIC_RESTART_STRING), NULL);
        Button1Code = SWM_MB_IDRESTART;
        break;

      case SWM_MB_OK:
        // Load the Ok as Button1 (the rightmost button).
        Button1Text = (CHAR16 *)HiiGetString (gSwmDialogsHiiHandle, STRING_TOKEN (STR_GENERIC_OK_STRING), NULL);
        Button1Code = SWM_MB_IDOK;
        break;

      case SWM_MB_YESNO:
        // Load the No as Button1 (the rightmost button).
        Button1Text = (CHAR16 *)HiiGetString (gSwmDialogsHiiHandle, STRING_TOKEN (STR_GENERIC_NO_STRING), NULL);
        Button1Code = SWM_MB_IDNO;
        // Load the Yes as Button2 (the leftmost button).
        Button2Text = (CHAR16 *)HiiGetString (gSwmDialogsHiiHandle, STRING_TOKEN (STR_GENERIC_YES_STRING), NULL);
        Button2Code = SWM_MB_IDYES;
        break;

      default:
        DEBUG((DEBUG_ERROR, "ERROR [SWM]: Unsupported MessageBox type %d.\r\n", BaseType));
        Status = EFI_UNSUPPORTED;
        goto Exit;
    }

    // Determine the longest button string for button sizing considerations.
    //
    LongestButtonText = Button1Text;
    // If Button2 is set, use it if it's longer.
    if (NULL != Button2Text &&
        StrLen (Button2Text) > StrLen (LongestButtonText))
    {
      LongestButtonText = Button2Text;
    }


    // Create a canvas for hosting the dialog child controls.
    //
    DialogCanvas = new_Canvas (DialogBounds,
                               BackgroundColor
                              );

    if (NULL == DialogCanvas)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Calculate the appropriate place to put the dialog's caption text.
    //
    ControlOrigX = (DialogOrigX + ((DialogWidth  * SWM_MB_DIALOG_CAPTION_X_PERCENT) / 100));
    ControlOrigY = (DialogOrigY + ((DialogHeight * SWM_MB_DIALOG_CAPTION_Y_PERCENT) / 100));


    // Select an appropriate font and colors for the caption text (larger font than the body).
    //
    FontInfo.FontSize    = SWM_MB_CUSTOM_FONT_CAPTION_HEIGHT;
    FontInfo.FontStyle   = EFI_HII_FONT_STYLE_NORMAL;
    FontInfo.FontName[0] = L'\0';


    // Draw Dialog CAPTION.
    //
    CaptionLabel = new_Label (ControlOrigX,
                              ControlOrigY,
                              (DialogBounds.Right - ControlOrigX - ((DialogWidth * SWM_MB_DIALOG_CAPTION_X_PERCENT) / 100)),
                              (DialogBounds.Bottom - ControlOrigY),       // In theory we could take up the entire dialog.
                              &FontInfo,
                              &gMsColorTable.MessageBoxTextColor,
                              BackgroundColor,
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
    ControlOrigY += ((ControlBounds.Bottom - ControlBounds.Top + 1) + SWM_MB_DIALOG_CONTROL_VERTICAL_PAD_PX);


    // Select an appropriate font and colors for the body text.
    //
    FontInfo.FontSize    = SWM_MB_CUSTOM_FONT_BODY_HEIGHT;
    FontInfo.FontStyle   = EFI_HII_FONT_STYLE_NORMAL;

    // Draw Dialog BODY TEXT.
    //
    BodyLabel = new_Label (ControlOrigX,
                           ControlOrigY,
                           (DialogBounds.Right - ControlOrigX - ((DialogWidth * SWM_MB_DIALOG_RIGHT_PADDING_PERCENT) / 100)),
                           (DialogBounds.Bottom - ControlOrigY),       // In theory we could take up the entire dialog.
                           &FontInfo,
                           &gMsColorTable.MessageBoxTextColor,
                           BackgroundColor,
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

    // Select an appropriate font and colors for button text.
    //
    FontInfo.FontSize    = SWM_MB_CUSTOM_FONT_BUTTONTEXT_HEIGHT;
    FontInfo.FontStyle   = EFI_HII_FONT_STYLE_NORMAL;


    // Calculate the string bitmap size of the largest button text.
    //
    GetTextStringBitmapSize (LongestButtonText,
                             &FontInfo,
                             FALSE,
                             EFI_HII_OUT_FLAG_CLIP |
                             EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                             EFI_HII_IGNORE_LINE_BREAK,
                             &StringRect,
                             &MaxGlyphDescent
                            );

    // Calculate the size and shape of the buttons.
    //
    ControlWidth    = (StringRect.Right - StringRect.Left + 1) + (SWM_MB_DIALOG_BUTTONTEXT_PADDING_PX * 2);
    ControlHeight   = (ControlWidth / SWM_MB_DIALOG_BUTTON_ASPECT_RATIO);

    // Calculate the position and size of the first button.
    //
    // X-Orig is:
    //  - Left side of dialog + width of dialog = right side of dialog
    ControlOrigX    = (DialogOrigX + DialogWidth);
    //  - Right side of dialog - right side padding = right side of rightmost button.
    ControlOrigX    -= ((DialogWidth * SWM_MB_DIALOG_RIGHT_PADDING_PERCENT) / 100);
    //  - Right side of rightmost button - two button widths and padding = left side of leftmost button.
    ControlOrigX    -= (ControlWidth * 2) + ((ControlWidth * SWM_MB_DIALOG_BUTTON_SPACE_PERCENT) / 100);
    // Y-Orig is: Top of dialog plus height minus padding.
    ControlOrigY    = (DialogOrigY + DialogHeight) - ((DialogHeight * SWM_MB_DIALOG_FIRST_BUTTON_Y_PERCENT) / 100) - ControlHeight;

//TODO: Change the MessageBox Colors to Themes just like PasswordDialog and replace this.
    switch (SWM_MB_STYLE_TYPE(Type)) {
    case SWM_MB_STYLE_ALERT1:
        ButtonSelectColor = &gMsColorTable.MessageBoxButtonSelectAlert1Color;
        ButtonHoverColor = &gMsColorTable.MessageBoxButtonSelectAlert1Color;
        break;
    case SWM_MB_STYLE_ALERT2:
    default:
        ButtonSelectColor = &gMsColorTable.MessageBoxButtonSelectColor;
        ButtonHoverColor = &gMsColorTable.MessageBoxButtonHoverColor;
        break;
    }
    // If provided, draw Button2 (the leftmost button).
    //
    if (NULL != Button2Text)
    {
        Button2 = new_Button (ControlOrigX,
                              ControlOrigY,
                              ControlWidth,
                              ControlHeight,
                              &FontInfo,
                              BackgroundColor,                            // Normal.
                              ButtonHoverColor,                           // Hover.
                              ButtonSelectColor,                          // Select.
                              &gMsColorTable.MessageBoxButtonGrayoutColor,         // GrayOut.
                              &gMsColorTable.MessageBoxButtonRingColor,            // Button ring.
                              &gMsColorTable.MessageBoxButtonTextColor,            // Normal text.
                              &gMsColorTable.MessageBoxButtonSelectTextColor,     // Normal text.
                              Button2Text,
                              (VOID *)(UINTN)Button2Code          // TODO - not the best way to do this.
                              );

        if (NULL == Button2)
        {
            Status = EFI_OUT_OF_RESOURCES;
            goto Exit;
        }

        // Add the control to the canvas.
        //
        DialogCanvas->AddControl (DialogCanvas,
                                  TRUE,     // Highlightable.
                                  FALSE,    // Not invisible.
                                  (VOID *)Button2
                                 );

    }

    // Draw Button1 (the rightmost button).
    //
    ControlOrigX += (ControlWidth + ((ControlWidth * SWM_MB_DIALOG_BUTTON_SPACE_PERCENT) / 100));

    Button1 = new_Button (ControlOrigX,
                           ControlOrigY,
                           ControlWidth,
                           ControlHeight,
                           &FontInfo,
                           BackgroundColor,                            // Normal.
                           ButtonHoverColor,           // Hover.
                           ButtonSelectColor,          // Select.
                           &gMsColorTable.MessageBoxButtonGrayoutColor,         // GrayOut
                           &gMsColorTable.MessageBoxButtonRingColor,            // Button ring.
                           &gMsColorTable.MessageBoxButtonTextColor,            // Normal text.
                           &gMsColorTable.MessageBoxButtonSelectTextColor,     // Normal text.
                           Button1Text,
                           (VOID *)(UINTN)Button1Code          // TODO - not the best way to do this.
                          );

    if (NULL == Button1)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Add the control to the canvas.
    //
    DialogCanvas->AddControl (DialogCanvas,
                              TRUE,     // Highlightable.
                              FALSE,    // Not invisible.
                              (VOID *)Button1
                             );

    // If there isn't another button, set this one as default.
    //

    DefaultButton = NULL;
    switch (SWM_MB_DEFAULT(Type)) {
    case SWM_MB_DEFBUTTON1:
        DefaultButton = Button1;
        break;
    case SWM_MB_DEFAULT_ACTION:
    case SWM_MB_DEFBUTTON2:
        if (NULL != Button2)
        {
            DefaultButton = Button2;
        }
        else
        {
            DefaultButton = Button1;
        }
        break;
    case SWM_MB_NO_DEFAULT:
    default:
        DefaultButton = NULL;
        break;
    }

    if (NULL != DefaultButton)
    {

        // Denote the button as the default control (for key input if nothing is highlighted).
        //
        DialogCanvas->SetDefaultControl (DialogCanvas,
                                         (VOID *)DefaultButton
                                        );
    }

    // Return the pointer to the canvas.
    //
    *DialogCanvasOut = DialogCanvas;

Exit:

    return Status;
}


/**
    Draws the dialog's outer frame and fills its background.

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
                 IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *BackgroundColor,
                 IN EFI_HANDLE                           MessageBoxHandle
                 )
{
    EFI_STATUS              Status          = EFI_SUCCESS;
    EFI_FONT_DISPLAY_INFO   StringInfo;
    EFI_IMAGE_OUTPUT        *pBltBuffer;

    // For performance reasons, drawing the frame as four individual (small) rectangles is faster than a single large rectangle.
    //
    this->BltWindow (this,                            // Top
                     MessageBoxHandle,
                     &gMsColorTable.MessageBoxDialogFrameColor,
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
                     MessageBoxHandle,
                     &gMsColorTable.MessageBoxDialogFrameColor,
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
                     MessageBoxHandle,
                     &gMsColorTable.MessageBoxDialogFrameColor,
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
                     MessageBoxHandle,
                     &gMsColorTable.MessageBoxDialogFrameColor,
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
                     MessageBoxHandle,
                     BackgroundColor,
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
    StringInfo.FontInfo.FontSize    = SWM_MB_CUSTOM_FONT_TITLEBAR_HEIGHT;
    StringInfo.FontInfo.FontStyle   = EFI_HII_FONT_STYLE_NORMAL;
    StringInfo.FontInfo.FontName[0] = L'\0';

    CopyMem (&StringInfo.ForegroundColor, &gMsColorTable.MessageBoxTitleBarTextColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    CopyMem (&StringInfo.BackgroundColor, &gMsColorTable.MessageBoxDialogFrameColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

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
                             &MaxDescent);

    // Render the string to the screen, vertically centered.
    //
    UINT32  FrameWidth      = (FrameRect.Right - FrameRect.Left + 1);
    UINT32  TitleBarHeight  = (CanvasRect.Top - FrameRect.Top + 1);

    this->StringToWindow (this,
                          MessageBoxHandle,
                          EFI_HII_OUT_FLAG_CLIP |
                          EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                          EFI_HII_IGNORE_LINE_BREAK | EFI_HII_DIRECT_TO_SCREEN,
                          pTitleBarText,
                          &StringInfo,
                          &pBltBuffer,
                          (FrameRect.Left + ((FrameWidth * SWM_MB_DIALOG_TITLEBAR_TEXT_X_PERCENT) / 100)),
                          (FrameRect.Top  + ((TitleBarHeight / 2) - ((StringRect.Bottom - StringRect.Top + 1) / 2)) + MaxDescent),  // Vertically center in the titlebar.
                          NULL,
                          NULL,
                          NULL);

Exit:

    if (NULL != pBltBuffer) {
        FreePool(pBltBuffer);
    }

    return Status;
}


/**
    Creates the dialog, canvas, and all child controls.

    @param[in]  this            Pointer to the instance of this driver.
    @param[in]  FrameRect       Bounding rectangle that defines the canvas dimensions.
    @param[in]  pTitleBarText   Dialog titlebar text.
    @param[in]  pCaptionText    Dialog caption text.
    @param[in]  pBodyText       Dialog body text.
    @param[out] DialogCanvasOut Pointer to the constructed canvas.

    @retval EFI_SUCCESS         Successfully created the single-select dialog canvas and controls.

**/
static EFI_STATUS
CreateMessageBoxDialog (IN    MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *this,
                        IN    SWM_RECT                            FrameRect,
                        IN    CHAR16                              *pTitleBarText,
                        IN    CHAR16                              *pCaptionText,
                        IN    CHAR16                              *pBodyText,
                        IN    UINT32                              Type,
                        IN    EFI_HANDLE                          MessageBoxHandle,
                        OUT   Canvas                              **DialogCanvasOut)
{
    EFI_STATUS  Status          = EFI_SUCCESS;
    UINT32      DialogHeight    = (FrameRect.Bottom - FrameRect.Top + 1);
    SWM_RECT    CanvasRect;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *BackgroundColor;

    switch (SWM_MB_STYLE_TYPE(Type)) {
    case SWM_MB_STYLE_ALERT1:
        BackgroundColor = &gMsColorTable.MessageBoxBackgroundAlert1Color;
        break;
    case SWM_MB_STYLE_ALERT2:
        BackgroundColor = &gMsColorTable.MessageBoxBackgroundAlert2Color;
        break;
    default:
        BackgroundColor = &gMsColorTable.MessageBoxBackgroundColor;
        break;
    }

    // Since we have a dialog titlebar and frame, the actual canvas area of the dialog is smaller.
    //
    CanvasRect.Left     = (FrameRect.Left + SWM_MB_DIALOG_FRAME_WIDTH_PX);
    CanvasRect.Top      = (FrameRect.Top  + ((DialogHeight * SWM_MB_DIALOG_TITLEBAR_HEIGHT_PERCENT) / 100));
    CanvasRect.Right    = (FrameRect.Right - SWM_MB_DIALOG_FRAME_WIDTH_PX);
    CanvasRect.Bottom   = (FrameRect.Bottom - SWM_MB_DIALOG_FRAME_WIDTH_PX);

    // Create a canvas and all of the child controls that make up the Single Select Dialog.
    //
    Status = CreateDialogControls (this,
                                   CanvasRect,
                                   pCaptionText,
                                   pBodyText,
                                   Type,
                                   BackgroundColor,
                                   MessageBoxHandle,
                                   DialogCanvasOut     // Use the caller's parameter to store the canvas pointer.
                                  );

    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [SWM]: Failed to create Dialog controls (%r).\r\n", Status));
        goto Exit;
    }

    // Draw the dialog body and frame.
    //
    DrawDialogFrame (this,
                     FrameRect,
                     CanvasRect,
                     pTitleBarText,
                     BackgroundColor,
                     MessageBoxHandle
                    );

Exit:

    return Status;
}


/**
    Processes user input (i.e., keyboard, touch, and mouse) and interaction with the Dialog.

    @param[in]  this            Pointer to the instance of this driver.
    @param[in]  FrameRect       Dialog box outer rectangle.
    @param[in]  DialogCanvas    Dialog box canvas hosting all the child controls.
    @param[in]  TitleBarText    Dialog box titlebar text.
    @param[in]  AbsolutePointer Absolute pointer protocol.

    @retval EFI_SUCCESS         Successfully processed dialog input.

**/
static
SWM_MB_RESULT
ProcessDialogInput (IN  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *this,
                    IN  SWM_RECT                            FrameRect,
                    IN  Canvas                             *DialogCanvas,
                    IN  CHAR16                             *pTitleBarText,
                    IN  EFI_ABSOLUTE_POINTER_PROTOCOL      *PointerProtocol,
                    IN  UINT64                              Timeout)
{
    EFI_STATUS          Status              = EFI_SUCCESS;
    UINTN               Index;
    OBJECT_STATE        State               = NORMAL;
    SWM_MB_RESULT       ButtonResult        = 0;
    VOID                *pContext           = NULL;
    SWM_INPUT_STATE     InputState;
    UINTN               NumberOfEvents      = 2;
    EFI_EVENT           WaitEvents[2];

    // Wait for user input.
    //
    WaitEvents[0] = gSimpleTextInEx->WaitForKeyEx;
    WaitEvents[1] = PointerProtocol->WaitForInput;

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

            // If user clicked either of the buttons, exit.
            // NOTE: This is ugly, but we have to account for all supported buttons.
            //
            if (SWM_MB_IDCANCEL == ButtonResult || SWM_MB_IDOK == ButtonResult ||
                SWM_MB_IDRETRY == ButtonResult || SWM_MB_IDCONTINUE == ButtonResult ||
                SWM_MB_IDYES   == ButtonResult || SWM_MB_IDNO  == ButtonResult ||
                SWM_MB_IDNEXT  == ButtonResult || SWM_MB_IDRESTART == ButtonResult)
            {
                break;
            }
        }

        while (EFI_SUCCESS == Status)
        {
            // Wait for user input.
            //
            Status = this->WaitForEvent (NumberOfEvents,
                                         WaitEvents,
                                        &Index,
                                         Timeout,
                                         FALSE);

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
            else if (EFI_SUCCESS == Status && 2 == Index) {
                ButtonResult = SWM_MB_TIMEOUT;
                break;
            }
        }

    } while (0 == ButtonResult && EFI_SUCCESS == Status);

    return (ButtonResult);
}

/**
    Displays a modal dialog box that contains a set of buttons and a brief use-specific message such as a
    prompt or status information.  The message box returns an integer value that indicates which button
    the user clicked.

    NOTE: MessageBox layout is only designed for "native" screen resolution and won't necessarily look
          good at lower resolution.

    @param[in]  this            Pointer to the instance of this driver.
    @param[in]  pText           Message to be displayed.
    @param[in]  pCaption        Dialog box title.
    @param[in]  Type            Contents and behavior of the dialog box.
    @param[out] Result          Button selection result.

    @retval EFI_SUCCESS             Successfully set the window frame.
    @retval EFI_UNSUPPORTED         The "Type" requested is not yet supported.

**/
EFI_STATUS
MessageBoxInternal (IN  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL     *This,
                    IN  CHAR16                                *pTitleBarText,
                    IN  CHAR16                                *pText,
                    IN  CHAR16                                *pCaption,
                    IN  UINT32                                 Type,
                    IN  UINT64                                 TimeoutRequest,
                    OUT SWM_MB_RESULT                         *Result)
{
    EFI_STATUS                      Status                      = EFI_SUCCESS;
    UINT32                          ScreenWidth,  ScreenHeight;
    UINT32                          DialogWidth,  DialogHeight;
    UINT32                          DialogOrigX,  DialogOrigY;
    Canvas                          *DialogCanvas               = NULL;
    SWM_RECT                        FrameRect;
    EFI_ABSOLUTE_POINTER_PROTOCOL  *PointerProtocol;
    EFI_EVENT                       PaintEvent                  = NULL;
    EFI_HANDLE                      MessageBoxHandle            = NULL;
    UINT32                          Z_Order;

    // Validate caller arguments.
    //
    if (NULL == Result)
    {
        Status = EFI_INVALID_PARAMETER;
        goto Exit;
    }

    // Get the current display resolution and use it to determine the size of the dialog.
    //
    ScreenWidth     =   gGop->Mode->Info->HorizontalResolution;
    ScreenHeight    =   gGop->Mode->Info->VerticalResolution;

    DialogWidth     =   ((ScreenWidth  * SWM_MB_DIALOG_WIDTH_PERCENT)  / 100);
    DialogHeight    =   ((ScreenHeight * SWM_MB_DIALOG_HEIGHT_PERCENT) / 100);

    // Calculate the location of the dialog's upper left corner origin (center of the screen vertically).  This
    // is the default location when the on-screen keyboard (OSK) isn't being displayed.
    //
    DialogOrigX = ((ScreenWidth  / 2)  - (DialogWidth / 2));
    DialogOrigY = ((ScreenHeight / 2) - (DialogHeight / 2));

    // Tell the rendering engine which "surface" to use - normal, or priority.
    MessageBoxHandle = gImageHandle;
    Z_Order = SWM_Z_ORDER_POPUP;
    if ((SWM_MB_STYLE_ALERT2 == SWM_MB_STYLE_TYPE(Type)) && (gPriorityHandle != NULL))
    {
        MessageBoxHandle = gPriorityHandle;
        InitializeUIToolKit (gPriorityHandle);   // Change default handle
        Z_Order = SWM_Z_ORDER_POPUP2;
        DialogOrigX +=  ((ScreenWidth  * SWM_MB_DIALOG_PRIORITY_OFFSET_PERCENT)  / 100);
        DialogOrigY +=  ((ScreenWidth  * SWM_MB_DIALOG_PRIORITY_OFFSET_PERCENT)  / 100);
    }

    // Calculate the dialog's outer rectangle.  Note that the dialog may need to co-exist with the OSK for input so we
    // need to share screen real estate and therefore cooperate for pointer event input.  When the OSK is displayed, the
    // dialog will be shifted up vertically to make room.
    //
    FrameRect.Left      = DialogOrigX;
    FrameRect.Top       = DialogOrigY;
    FrameRect.Right     = (DialogOrigX + DialogWidth - 1);
    FrameRect.Bottom    = (DialogOrigY + DialogHeight - 1);

    // Register with the Simple Window Manager to get mouse and touch input events.
    //
    Status = This->RegisterClient(This,
                                     MessageBoxHandle,
                                     Z_Order,
                                     &FrameRect,
                                     NULL,
                                     NULL,
                                     &PointerProtocol,
                                     &PaintEvent);

    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [MsgBox]: Failed to register the dialog as a client: %r.\r\n", Status));
        goto Exit;
    }

    // Set window manager client state active.
    //
    This->ActivateWindow (This,
                          MessageBoxHandle,
                          TRUE);

    // Enable the mouse pointer to be displayed if a USB mouse or Blade trackpad is attached and is moved.
    //
    This->EnableMousePointer (This,
                                 TRUE);

    // Create the dialog and all its child controls at the specified screen location & size.
    //
    Status = CreateMessageBoxDialog (This,
                                     FrameRect,
                                     pTitleBarText,
                                     pCaption,
                                     pText,
                                     Type,
                                     MessageBoxHandle,
                                     &DialogCanvas);

    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [SWM]: Failed to create Dialog: %r.\r\n", Status));
        goto Exit;
    }

    // Process user input.
    //
    *Result = ProcessDialogInput (This,
                                  FrameRect,
                                  DialogCanvas,
                                  pTitleBarText,
                                  PointerProtocol,
                                  TimeoutRequest);

    // Set client state inactive (messages will by default go to the default client).
    //
    This->ActivateWindow (This,
                             MessageBoxHandle,
                             FALSE);

Exit:
    // Unregister with the window manager as a client.
    //
    This->UnregisterClient (This,
                            MessageBoxHandle);

    if (MessageBoxHandle == gPriorityHandle) {
        InitializeUIToolKit (gImageHandle);  // Restore UI Handle to normal
    }

    // Clean-up.
    //

    // Free the canvas (and all child controls it's hosting).
    //
    if (NULL != DialogCanvas) {
        delete_Canvas (DialogCanvas);
    }

    return Status;
}
