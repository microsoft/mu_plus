/** @file

  Implements a simple editbox control.

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

#include "SimpleUIToolKitInternal.h"


#define CHAR_BULLET_UNICODE                     0x2022

#define UIT_EDITBOX_HORIZONTAL_PADDING          MsUiScaleByTheme(30)  // 30 pixels - padding at beginning and end of text.
#define UIT_EDITBOX_VERTICAL_PADDING            MsUiScaleByTheme(20)  // 20 pixels - padding on top and bottom of text.
#define UIT_E_HIGHLIGHT_RING_WIDTH              MsUiScaleByTheme(4)   // Number of pixels making up the EditBox's highlight ring.

static MS_ONSCREEN_KEYBOARD_PROTOCOL           *mOSKProtocol = NULL;


//////////////////////////////////////////////////////////////////////////////
// Private
//
static
EFI_STATUS
RenderEditBox(IN EditBox  *this,
              IN BOOLEAN DrawHighlight)
{
    EFI_STATUS                      Status = EFI_SUCCESS;
    UINTN                           Width, Height;
    EFI_FONT_DISPLAY_INFO           *StringInfo = NULL;
    EFI_IMAGE_OUTPUT                *BltBuffer = NULL;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *TextColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *FillColor;


    // Select colors.
    //
    switch(this->m_State)
    {
    case GRAYED:
        FillColor = &this->m_GrayOutColor;
        TextColor = &this->m_GrayOutTextColor;
        break;
    case NORMAL:
    case HOVER:
    case SELECT:
    default:
        FillColor = &this->m_NormalColor;
        TextColor = &this->m_NormalTextColor;
        break;
    }

    // Highlight color is always white background.
    //
    if (TRUE == DrawHighlight)
    {
        FillColor = &gMsColorTable.EditBoxHighlightBGColor;
    }

    StringInfo = BuildFontDisplayInfoFromFontInfo (this->m_FontInfo);
    if (NULL == StringInfo)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    CopyMem (&StringInfo->BackgroundColor, FillColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    CopyMem (&StringInfo->ForegroundColor, TextColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

    // Prepare the string blitting buffer.
    //
    BltBuffer = (EFI_IMAGE_OUTPUT *) AllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));

    ASSERT (BltBuffer != NULL);
    if (NULL == BltBuffer)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    BltBuffer->Width        = (UINT16)mUITGop->Mode->Info->HorizontalResolution;
    BltBuffer->Height       = (UINT16)mUITGop->Mode->Info->VerticalResolution;
    BltBuffer->Image.Screen = mUITGop;

    // Compute editbox width and height.
    //
    Width   = (this->m_EditBoxBounds.Right  - this->m_EditBoxBounds.Left + 1);
    Height  = (this->m_EditBoxBounds.Bottom - this->m_EditBoxBounds.Top + 1);

    // Draw the editbox background first.
    //
    mUITSWM->BltWindow (mUITSWM,
                        mClientImageHandle,
                        FillColor,
                        EfiBltVideoFill,
                        0,
                        0,
                        this->m_EditBoxBounds.Left,
                        this->m_EditBoxBounds.Top,
                        Width,
                        Height,
                        Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                       );

    // If there is no text in the editbox, display the watermark text.  Note that we assume the editbox rectangle is large
    // enough for the watermark text as well.
    //
    if (0 == this->m_CurrentPosition)
    {
        // Select preferred font size and style for the watermark text.
        //
        StringInfo->FontInfoMask = EFI_FONT_INFO_ANY_FONT;

        // Override text color and font for watermark.
        //
        if (this->m_State != GRAYED){
            StringInfo->ForegroundColor = gMsColorTable.EditBoxWaterMarkFGColor;
        }

        if (UIT_EDITBOX_TYPE_SELECTABLE  != this->m_Type) {   // For selectable, use the same font as the edit string
            StringInfo->FontInfo.FontSize = MsUiGetSmallFontHeight ();   // TODO
        }

        mUITSWM->StringToWindow (mUITSWM,
                                 mClientImageHandle,
                                 EFI_HII_OUT_FLAG_CLIP |
                                 EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                                 EFI_HII_IGNORE_LINE_BREAK | EFI_HII_DIRECT_TO_SCREEN,
                                 this->m_EditBoxWatermarkText,
                                 StringInfo,
                                 &BltBuffer,
                                 this->m_EditBoxTextBounds.Left,
                                 this->m_EditBoxTextBounds.Top,
                                 NULL,
                                 NULL,
                                 NULL
                                );
    }
    else
    {
        // Draw editbox text.
        //
        // Select preferred font size and style for the editbox text.
        StringInfo->FontInfoMask = EFI_FONT_INFO_ANY_FONT;

        mUITSWM->StringToWindow (mUITSWM,
                                 mClientImageHandle,
                                 EFI_HII_OUT_FLAG_CLIP |
                                 EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                                 EFI_HII_IGNORE_LINE_BREAK | EFI_HII_DIRECT_TO_SCREEN,
                                 &this->m_EditBoxDisplayText[this->m_DisplayStartPosition],
                                 StringInfo,
                                 &BltBuffer,
                                 this->m_EditBoxTextBounds.Left,
                                 this->m_EditBoxTextBounds.Top,
                                 NULL,
                                 NULL,
                                 NULL
                               );
    }

    if ((TRUE == DrawHighlight) && (this->m_Type == UIT_EDITBOX_TYPE_SELECTABLE))
    {
        DrawRectangleOutline (this->m_EditBoxBounds.Left,
                              this->m_EditBoxBounds.Top,
                              this->m_EditBoxBounds.Right - this->m_EditBoxBounds.Left - 1,
                              this->m_EditBoxBounds.Bottom - this->m_EditBoxBounds.Top - 1,
                              UIT_E_HIGHLIGHT_RING_WIDTH,
                              &gMsColorTable.EditBoxHighlightBoundColor
                             );
    }
Exit:

    // Clean up.
    //
    if (NULL != BltBuffer)
    {
        FreePool(BltBuffer);
    }

    if (NULL != StringInfo)
    {
        FreePool (StringInfo);
    }

    return Status;
}


static
EFI_STATUS
SetControlBounds (IN EditBox    *this,
                  IN SWM_RECT   Bounds)
{
    EFI_STATUS Status = EFI_SUCCESS;
    UINT32  TextOffsetX, TextOffsetY;


    // Compute the relative offset from editbox origin to editbox text origin.
    //
    TextOffsetX = (this->m_EditBoxTextBounds.Left - this->m_EditBoxBounds.Left);
    TextOffsetY = (this->m_EditBoxTextBounds.Top - this->m_EditBoxBounds.Top);

    // Translate (and possibly resize) the editbox bounding box.
    //
    CopyMem (&this->m_EditBoxBounds, &Bounds, sizeof(SWM_RECT));

    // Translate (and possibly resize) the editbox text bounding box.
    //
    SWM_RECT    *TextRect   = &this->m_EditBoxTextBounds;
    UINT32      TextWidth   = (TextRect->Right - TextRect->Left + 1);
    UINT32      TextHeight  = (TextRect->Bottom - TextRect->Top + 1);

    Bounds.Left     += TextOffsetX;
    Bounds.Top      += TextOffsetY;
    Bounds.Right    = ((Bounds.Left + TextWidth) < Bounds.Right ? (Bounds.Right + TextWidth) : Bounds.Right);
    Bounds.Bottom   = ((Bounds.Top + TextHeight) < Bounds.Bottom ? (Bounds.Top + TextHeight) : Bounds.Bottom);

    CopyMem (&this->m_EditBoxTextBounds, &Bounds, sizeof(SWM_RECT));

    // MaxDisplay Characters =  ((width of the full edibox) +  (AvgCharWidth -1 ) - (left and right padding)) / AvgCharWidth
    this->m_MaxDisplayChars = (this->m_EditBoxBounds.Right - this->m_EditBoxBounds.Left + 1 + this->m_CharWidth - 1 - (2 * UIT_EDITBOX_HORIZONTAL_PADDING)) / this->m_CharWidth;

    return Status;
}


static
EFI_STATUS
GetControlBounds (IN  EditBox   *this,
                  OUT SWM_RECT  *pBounds)
{
    EFI_STATUS Status = EFI_SUCCESS;


    CopyMem (pBounds, &this->m_EditBoxBounds, sizeof(SWM_RECT));

    return Status;
}


static
EFI_STATUS
ClearEditBox (IN  EditBox   *this)
{
    EFI_STATUS  Status = EFI_SUCCESS;

    this->m_EditBoxText[0]  = L'\0';
    this->m_CurrentPosition = 0;

    // Draw an empty edit box.
    //
    RenderEditBox(this,
                  FALSE
                 );

    return Status;
}


static
EFI_STATUS
WipeBuffer (IN  EditBox   *this)
{
    ZeroMem ((UINT8*)&this->m_EditBoxText[0], UIT_EDITBOX_MAX_STRING_LENGTH * sizeof(this->m_EditBoxText[0]));
    return this->ClearEditBox (this);
}


static
VOID
EnableKeyboard (IN  struct _EditBox *this) {

    // Configure the OSK position, size, and configuration (75% of screen width, bottom right position, docked).
    //
    if ((UIT_EDITBOX_TYPE_SELECTABLE != this->m_Type) || this->m_KeyboardEnabled)
    {
        return;
    }

    this->m_KeyboardEnabled = TRUE;

    mOSKProtocol->SetKeyboardSize(mOSKProtocol, 75);
    mOSKProtocol->SetKeyboardPosition(mOSKProtocol, BottomRight, Docked);
    mOSKProtocol->ShowDockAndCloseButtons(mOSKProtocol, FALSE);
    mOSKProtocol->SetKeyboardIconPosition(mOSKProtocol, BottomRight);
    mOSKProtocol->ShowKeyboardIcon(mOSKProtocol, FALSE);
    mOSKProtocol->ShowKeyboard(mOSKProtocol, TRUE);
}


static
CHAR16 *
GetCurrentTextString (IN        EditBox   *this)
{

    // Return a pointer to the editbox string to the caller.
    //
    return(this->m_EditBoxText);
}

static
EFI_STATUS
SetCurrentTextString (IN        EditBox   *this,
                      IN        CHAR16    *NewTextString)
{
    BOOLEAN    RenderRequired = FALSE;
    UINT32     NewTextLen;

    NewTextLen = (UINT32) StrnLenS (NewTextString,(UIT_EDITBOX_MAX_STRING_LENGTH - 1));
    // Return a pointer to the editbox string to the caller.
    //
    if (this->m_State == GRAYED) {  // update watermark as that is the only thing displayed
        RenderRequired = 0 != StrnCmp(this->m_EditBoxWatermarkText, NewTextString, UIT_EDITBOX_MAX_STRING_LENGTH - 1);
        StrnCpyS(this->m_EditBoxWatermarkText, sizeof(this->m_EditBoxWatermarkText)/sizeof(CHAR16), NewTextString, (UIT_EDITBOX_MAX_STRING_LENGTH - 1));
    } else {
        RenderRequired = 0 != StrnCmp(this->m_EditBoxText, NewTextString, UIT_EDITBOX_MAX_STRING_LENGTH - 1);
        StrnCpyS(this->m_EditBoxText, sizeof(this->m_EditBoxText)/sizeof(CHAR16), NewTextString, (UIT_EDITBOX_MAX_STRING_LENGTH - 1));
        this->m_CurrentPosition = NewTextLen;
    }
    if (RenderRequired) {
        RenderEditBox(this, FALSE);
    }
    return EFI_SUCCESS;
}


static
EFI_STATUS
SetControlState (IN EditBox         *this,
                 IN OBJECT_STATE    State)
{
    // Hide the keyboard (if it was being displayed).
    //
    if ((UIT_EDITBOX_TYPE_SELECTABLE != this->m_Type) && (State == KEYFOCUS)) {
        return EFI_INVALID_PARAMETER;
    }

    if (this->m_State != State)
    {
        if (State == KEYFOCUS)
        {
            EnableKeyboard (this);
        }
        else if (this->m_KeyboardEnabled)
        {
            this->m_KeyboardEnabled = FALSE;
            // Hide the on-screen keyboard (if we were showing it).
            //
            mOSKProtocol->ShowKeyboard (mOSKProtocol, FALSE);
        }
        this->m_State = State;
    }


    return EFI_SUCCESS;
}

static
OBJECT_STATE
GetControlState(IN EditBox         *this)
{
    return this->m_State;
}


static
EFI_STATUS
CopySettings (IN EditBox  *this,
              IN EditBox  *prev) {

    this->m_State = prev->m_State;
    this->m_KeyboardEnabled = prev->m_KeyboardEnabled;
    prev->m_KeyboardEnabled = FALSE;   // Don't allow Distructor to turn off keyboard
    this->m_CurrentPosition = prev->m_CurrentPosition;
    this->m_CharWidth = prev->m_CharWidth;
    this->m_MaxDisplayChars = prev->m_MaxDisplayChars;
    this->m_DisplayStartPosition = prev->m_DisplayStartPosition;
    StrnCpyS(this->m_EditBoxText, sizeof(this->m_EditBoxText)/sizeof(CHAR16), prev->m_EditBoxText, (UIT_EDITBOX_MAX_STRING_LENGTH - 1));
    StrnCpyS(this->m_EditBoxDisplayText, sizeof(this->m_EditBoxDisplayText)/sizeof(CHAR16), prev->m_EditBoxDisplayText, (UIT_EDITBOX_MAX_STRING_LENGTH - 1));
    StrnCpyS(this->m_EditBoxWatermarkText, sizeof(this->m_EditBoxWatermarkText)/sizeof(CHAR16), prev->m_EditBoxWatermarkText, (UIT_EDITBOX_MAX_STRING_LENGTH - 1));
    return EFI_SUCCESS;
}


static
OBJECT_STATE
Draw (IN    EditBox             *this,
      IN    BOOLEAN             DrawHighlight,
      IN    SWM_INPUT_STATE     *pInputState,
      OUT   VOID                **pSelectionContext)
{
    EFI_STATUS  Status      = EFI_SUCCESS;
    SWM_RECT    *pRect      = &this->m_EditBoxBounds;
    VOID        *Context    = NULL;

    // If the editbox is GrayedOut render and return.
    //
    if (this->m_State == GRAYED)
    {
        Status = RenderEditBox(this,
            FALSE
            );

        goto Exit;
    }

    // If we're asked by the canvas to draw without highlight, we no longer have focus.
    //
    if (FALSE == DrawHighlight)
    {
        this->m_State = NORMAL;
    }

    // If there is no input state, simply draw the editbox then return.
    //
    if (NULL == pInputState)
    {
        Status = RenderEditBox (this,
                                DrawHighlight
                               );

        goto Exit;
    }

    // If there is user touch/mouse input, indicate we now have focus.
    //
    if (SWM_INPUT_TYPE_TOUCH == pInputState->InputType &&
        pInputState->State.TouchState.CurrentX >= pRect->Left && pInputState->State.TouchState.CurrentX <= pRect->Right &&
        pInputState->State.TouchState.CurrentY >= pRect->Top  && pInputState->State.TouchState.CurrentY <= pRect->Bottom)
    {
        // Set focus so the canvas knows.
        //
        this->m_State = KEYFOCUS;

        if (((pInputState->State.TouchState.ActiveButtons & 0x01) == 0x01) &&  // Only process touch, not release
            (this->m_Type == UIT_EDITBOX_TYPE_SELECTABLE))
        {
            EnableKeyboard (this);
        }

        // We're done.
        //
        goto Exit;
    }

    // If there is user keyboard input, handle it here.
    //
    if (SWM_INPUT_TYPE_KEY == pInputState->InputType)
    {
        BOOLEAN NeedToRender = FALSE;
        CHAR16  InputChar    = pInputState->State.KeyState.Key.UnicodeChar;

        switch (InputChar)
        {
        case CHAR_BACKSPACE:
            if (this->m_CurrentPosition > 0)
            {
                --this->m_CurrentPosition;
                this->m_EditBoxText[this->m_CurrentPosition] = L'\0';
                this->m_EditBoxDisplayText[this->m_CurrentPosition] = L'\0';

                // Move the editbox text starting position as characters are deleted but only in discrete steps to ensure we're always displaying characters if there are any.
                //
                if (this->m_CurrentPosition == this->m_DisplayStartPosition)
                {
                    if (this->m_DisplayStartPosition >= this->m_MaxDisplayChars)
                    {
                        this->m_DisplayStartPosition -= (this->m_MaxDisplayChars / 2);
                    }
                    else
                    {
                        this->m_DisplayStartPosition = 0;
                    }
                }

                // Make sure we render the edit box.
                //
                NeedToRender = TRUE;
            }
            break;

        case CHAR_CARRIAGE_RETURN:
            if (UIT_EDITBOX_TYPE_SELECTABLE  == this->m_Type) {
                Context = this->m_pSelectionContext;
                this->m_State = SELECT;
                // Hide the keyboard (if it was being displayed).
                //
                if (this->m_KeyboardEnabled)
                {
                    // Hide the on-screen keyboard (if we were showing it).
                    //
                    mOSKProtocol->ShowKeyboard (mOSKProtocol, FALSE);
                    this->m_KeyboardEnabled = FALSE;
                }
                NeedToRender = TRUE;
                DrawHighlight = FALSE;
            }
            break;


        case CHAR_LINEFEED:
            // Nothing to do.
            break;

        default:
            if (this->m_CurrentPosition < UIT_EDITBOX_MAX_STRING_LENGTH &&
                InputChar >= 0x0020 && InputChar <= 0x007E)               // Only Basic Latin range of (ASCII) printable characters are allowed.
            {
                // For a password editbox, never show the actual character - just a bullet.
                //
                if (UIT_EDITBOX_TYPE_PASSWORD == this->m_Type)
                {
                    this->m_EditBoxDisplayText[this->m_CurrentPosition] = CHAR_BULLET_UNICODE;
                }
                else
                {
                    this->m_EditBoxDisplayText[this->m_CurrentPosition] = InputChar;
                }

                // Either way, add the char to the internal text buffer.
                //
                this->m_EditBoxText[this->m_CurrentPosition] = InputChar;

                // Move the editbox text starting position.  It moves by one character for each one over the maximum display length.
                //
                if ((this->m_CurrentPosition - this->m_DisplayStartPosition) >= this->m_MaxDisplayChars)
                {
                    ++this->m_DisplayStartPosition;
                }

                ++this->m_CurrentPosition;
                this->m_EditBoxText[this->m_CurrentPosition] = L'\0';
                this->m_EditBoxDisplayText[this->m_CurrentPosition] = L'\0';

                // Make sure we render the edit box.
                //
                NeedToRender = TRUE;
            }
            break;
        }

        // Draw the editbox if needed.
        //
        if (TRUE == NeedToRender)
        {
            Status = RenderEditBox (this,
                                    DrawHighlight
                                   );
        }

        // We're done, exit.
        //
        goto Exit;
    }


Exit:

    if (NULL != pSelectionContext)
    {
        *pSelectionContext = Context;
    }

    return (this->m_State);
}


//////////////////////////////////////////////////////////////////////////////
//
//
static
VOID
Ctor(IN struct _EditBox                *this,
     IN UINT32                         OrigX,
     IN UINT32                         OrigY,
     IN UINT32                         MaxDisplayChars,
     IN UIT_EDITBOX_TYPE               Type,
     IN EFI_FONT_INFO                  *pFontInfo,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pNormalColor,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pNormalTextColor,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pGrayOutColor,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pGrayOutTextColor,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pSelectTextColor,
     IN CHAR16                         *pWatermarkText,
     IN VOID                           *pSelectionContext)
{
    SWM_RECT    TextRect;
    UINT32      TextWidth, TextHeight, MaxGlyphDescent;
    EFI_STATUS  Status;


    // Check paramters.
    //
    if (MaxDisplayChars > UIT_EDITBOX_MAX_STRING_LENGTH)
    {
        return;
    }

    if (NULL != pWatermarkText && StrLen(pWatermarkText) > UIT_EDITBOX_MAX_STRING_LENGTH)
    {
        return;
    }

    // Initialize variables.
    //
    this->m_FontInfo = DupFontInfo (pFontInfo);
    if (NULL == this->m_FontInfo)
    {
        return;
    }

    this->m_NormalColor      = *pNormalColor;
    this->m_NormalTextColor  = *pNormalTextColor;
    this->m_GrayOutColor     = *pGrayOutColor;
    this->m_GrayOutTextColor = *pGrayOutTextColor;
    this->m_SelectTextColor  = *pSelectTextColor;

    // Save the associated selection context pointer.
    //
    this->m_pSelectionContext = pSelectionContext;

    this->m_MaxDisplayChars   = MaxDisplayChars;
    this->m_Type              = Type;

    this->m_CurrentPosition         = 0;
    this->m_DisplayStartPosition    = 0;

    // Save pointer to the editbox's watermark text.
    //
    if (NULL != pWatermarkText)
    {
        StrnCpyS(this->m_EditBoxWatermarkText, sizeof(this->m_EditBoxWatermarkText)/sizeof(CHAR16), pWatermarkText, (UIT_EDITBOX_MAX_STRING_LENGTH - 1));
    }

    // Compute EditBox bounding rectangle from requested text display size.
    //
    SetMem16(this->m_EditBoxText, (MaxDisplayChars * sizeof(CHAR16)), L'W');
    this->m_EditBoxText[MaxDisplayChars] = L'\0';

    GetTextStringBitmapSize (this->m_EditBoxText,
                             pFontInfo,
                             FALSE,
                             EFI_HII_OUT_FLAG_CLIP |
                             EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                             EFI_HII_IGNORE_LINE_BREAK,
                             &TextRect,
                             &MaxGlyphDescent
                            );

    this->m_EditBoxText[0] = L'\0';

    TextWidth     = (TextRect.Right - TextRect.Left + 1);
    TextHeight    = (TextRect.Bottom - TextRect.Top + 1);

    this->m_CharWidth = TextWidth / MaxDisplayChars;    // Average width of one 'W'

    this->m_EditBoxBounds.Left      = OrigX;
    this->m_EditBoxBounds.Top       = OrigY;
    this->m_EditBoxBounds.Right     = (OrigX + TextWidth + (UIT_EDITBOX_HORIZONTAL_PADDING * 2));       // At beginning and end of editbox text.
    this->m_EditBoxBounds.Bottom    = (OrigY + TextHeight + (UIT_EDITBOX_VERTICAL_PADDING * 2));        // At top and bottom of editbox text.


    // Compute EditBox text bounding rectangle (based on max display string length).
    //
    this->m_EditBoxTextBounds.Left      = (OrigX + UIT_EDITBOX_HORIZONTAL_PADDING);
    this->m_EditBoxTextBounds.Top       = (OrigY + UIT_EDITBOX_VERTICAL_PADDING);
    this->m_EditBoxTextBounds.Right     = (this->m_EditBoxBounds.Right - UIT_EDITBOX_HORIZONTAL_PADDING);
    this->m_EditBoxTextBounds.Bottom    = (this->m_EditBoxBounds.Bottom - UIT_EDITBOX_VERTICAL_PADDING);

    // Configure EditBox state.
    //
    this->m_State               = NORMAL;
    this->m_KeyboardEnabled     = FALSE;

    // Member Variables
    this->Base.ControlType      = EDITBOX;

    // Functions.
    //
    this->Base.Draw             = (DrawFunctionPtr) &Draw;
    this->Base.SetControlBounds = (SetControlBoundsFunctionPtr) &SetControlBounds;
    this->Base.GetControlBounds = (GetControlBoundsFunctionPtr) &GetControlBounds;
    this->Base.SetControlState  = (SetControlStateFunctionPtr) &SetControlState;
    this->Base.GetControlState  = (GetControlStateFunctionPtr) &GetControlState;
    this->Base.CopySettings     = (CopySettingsFunctionPtr) &CopySettings;

    this->GetCurrentTextString  = &GetCurrentTextString;
    this->SetCurrentTextString  = &SetCurrentTextString;
    this->ClearEditBox          = &ClearEditBox;
    this->WipeBuffer            = &WipeBuffer;

    if (UIT_EDITBOX_TYPE_SELECTABLE  == this->m_Type)
    {
        Status = gBS->LocateProtocol (&gMsOSKProtocolGuid,
                                      NULL,
                                      (VOID **)&mOSKProtocol

                                     );
        if (EFI_ERROR (Status))
        {
            DEBUG((DEBUG_ERROR, "ERROR [EditBox]: Failed to locate on-screen keyboard protocol - no OSK (%r).\r\n", Status));
            mOSKProtocol = NULL;
            // Guarantee that SELECTABLE also indicate mOSKProtocol is not NULL
            this->m_Type = UIT_EDITBOX_TYPE_NORMAL;
        }
    }

    return;
}


static
VOID Dtor(VOID *this)
{
    EditBox *privthis = (EditBox *)this;


    // Free our memory.
    //
    if (NULL != privthis)
    {
        if (privthis->m_KeyboardEnabled)
        {
            // Hide the on-screen keyboard (if we were showing it).
            //
            mOSKProtocol->ShowKeyboard (mOSKProtocol, FALSE);
        }
        if (NULL != privthis->m_FontInfo)
        {
            FreePool (privthis->m_FontInfo);
        }
        FreePool(privthis);
    }

    return;
}


//////////////////////////////////////////////////////////////////////////////
// Public
//
EditBox *new_EditBox(IN UINT32                              OrigX,
                     IN UINT32                              OrigY,
                     IN UINT32                              MaxDisplayChars,
                     IN UIT_EDITBOX_TYPE                    Type,
                     IN EFI_FONT_INFO                       *pFontInfo,
                     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *pNormalColor,
                     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *pNormalTextColor,
                     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *pGrayOutColor,
                     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *pGrayOutTextColor,
                     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *pSelectTextColor,
                     IN CHAR16                              *pWatermarkText,
                     IN VOID                                *pSelectionContext)
{

    EditBox *EB = (EditBox *)AllocateZeroPool(sizeof(EditBox));
    ASSERT(NULL != EB);

    if (NULL != EB)
    {
        EB->Ctor         = &Ctor;
        EB->Base.Dtor    = &Dtor;

        EB->Ctor(EB,
                 OrigX,
                 OrigY,
                 MaxDisplayChars,
                 Type,
                 pFontInfo,
                 pNormalColor,
                 pNormalTextColor,
                 pGrayOutColor,
                 pGrayOutTextColor,
                 pSelectTextColor,
                 pWatermarkText,
                 pSelectionContext
                );
    }

    return EB;
}


VOID delete_EditBox(IN EditBox *this)
{
    if (NULL != this)
    {
        this->Base.Dtor((VOID *)this);
    }
}
