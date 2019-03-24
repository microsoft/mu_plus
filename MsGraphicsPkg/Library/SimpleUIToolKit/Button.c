/** @file

  Implements a simple button control.

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



#define UIT_B_HIGHLIGHT_RING_WIDTH     MsUiScaleByTheme(4)        // Number of pixels making up the button's highlight ring.
#define UIT_B_OUTER_BORDER_WIDTH       MsUiScaleByTheme(5)        // Number of pixels making up the buttons outer border.


//////////////////////////////////////////////////////////////////////////////
// Private
//
static
EFI_STATUS
RenderButton(IN Button  *this,
             IN BOOLEAN DrawHighlight)
{
    EFI_STATUS                      Status = EFI_SUCCESS;
    UINTN                           Width, Height;
    EFI_FONT_DISPLAY_INFO           *StringInfo = NULL;
    EFI_IMAGE_OUTPUT                *pBltBuffer = NULL;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *pTextColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *pFillColor;


    // Color.
    //
    switch (this->m_pButton->State) {
    case KEYDEFAULT:
    case HOVER:
    case SELECT:
        if (this->m_ButtonDown)
        {
            pFillColor = &this->m_SelectColor;
            pTextColor = &this->m_SelectTextColor;
        }
        else
        {
            pFillColor = &this->m_HoverColor;
            pTextColor = &this->m_NormalTextColor;
        }
        break;
    case GRAYED:
        pFillColor = &this->m_NormalColor;
        pTextColor = &this->m_GrayOutTextColor;
        break;
    case NORMAL:
    default:
        pFillColor = &this->m_NormalColor;
        pTextColor = &this->m_NormalTextColor;
        break;
    }

    StringInfo = BuildFontDisplayInfoFromFontInfo (this->m_FontInfo);
    if (NULL == StringInfo)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    CopyMem (&StringInfo->BackgroundColor, pFillColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    CopyMem (&StringInfo->ForegroundColor, pTextColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

    // Prepare string blitting buffer.
    //
    pBltBuffer = (EFI_IMAGE_OUTPUT *) AllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));
    ASSERT (pBltBuffer != NULL);

    if (NULL == pBltBuffer)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    pBltBuffer->Width        = (UINT16)mUITGop->Mode->Info->HorizontalResolution;
    pBltBuffer->Height       = (UINT16)mUITGop->Mode->Info->VerticalResolution;
    pBltBuffer->Image.Screen = mUITGop;

    // Compute button width and height.
    //
    Width   = (this->m_pButton->ButtonBounds.Right - this->m_pButton->ButtonBounds.Left + 1);
    Height  = (this->m_pButton->ButtonBounds.Bottom - this->m_pButton->ButtonBounds.Top) + 1;

    // Draw the button's - outer rectangle first.
    //
    DrawRectangleOutline (this->m_pButton->ButtonBounds.Left,
                          this->m_pButton->ButtonBounds.Top,
                          (UINT32)Width,
                          (UINT32)Height,
                          UIT_B_OUTER_BORDER_WIDTH,
                          &this->m_ButtonRingColor
                         );

    // TODO
    UINTN ButtonFillOrigX  = this->m_pButton->ButtonBounds.Left + UIT_B_OUTER_BORDER_WIDTH;
    UINTN ButtonFillOrigY  = this->m_pButton->ButtonBounds.Top  + UIT_B_OUTER_BORDER_WIDTH;
    UINTN ButtonFillWidth  = (Width  - (UIT_B_OUTER_BORDER_WIDTH * 2));
    UINTN ButtonFillHeight = (Height - (UIT_B_OUTER_BORDER_WIDTH * 2));

    // Draw the button's - inner rectangle next.
    //
    mUITSWM->BltWindow (mUITSWM,
                        mClientImageHandle,
                        pFillColor,
                        EfiBltVideoFill,
                        0,
                        0,
                        ButtonFillOrigX,
                        ButtonFillOrigY,
                        ButtonFillWidth,
                        ButtonFillHeight,
                        ButtonFillWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                       );


    // Draw the button's - keyboard focus highilght rectangle if it's needed.
    //
    if (TRUE == DrawHighlight)
    {
        DrawRectangleOutline ((UINT32)ButtonFillOrigX+1,
                              (UINT32)ButtonFillOrigY+1,
                              (UINT32)ButtonFillWidth-2,
                              (UINT32)ButtonFillHeight-2,
                              UIT_B_HIGHLIGHT_RING_WIDTH,
                              &gMsColorTable.ButtonHighlightBoundColor
                             );
    }

    // Draw button text.
    //
    // Select preferred font size and style for the button (smaller, normal font).
    //
    StringInfo->FontInfoMask = EFI_FONT_INFO_ANY_FONT;

    mUITSWM->StringToWindow (mUITSWM,
                             mClientImageHandle,
                             EFI_HII_OUT_FLAG_CLIP |
                             EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                             EFI_HII_IGNORE_LINE_BREAK | EFI_HII_DIRECT_TO_SCREEN,
                             this->m_pButton->pButtonText,
                             StringInfo,
                             &pBltBuffer,
                             this->m_pButton->ButtonTextBounds.Left,
                             this->m_pButton->ButtonTextBounds.Top,
                             NULL,
                             NULL,
                             NULL
                            );

Exit:

    if (NULL != StringInfo)
    {
        FreePool (StringInfo);
    }

    if (NULL != pBltBuffer)
    {
        FreePool(pBltBuffer);
    }

    return Status;
}


static
EFI_STATUS
SetControlBounds(IN Button      *this,
                 IN SWM_RECT    Bounds)
{
    EFI_STATUS Status = EFI_SUCCESS;
    UINT32  TextOffsetX, TextOffsetY;


    // Compute the relative offset from button origin to button text origin.
    //
    TextOffsetX = (this->m_pButton->ButtonTextBounds.Left - this->m_pButton->ButtonBounds.Left);
    TextOffsetY = (this->m_pButton->ButtonTextBounds.Top  - this->m_pButton->ButtonBounds.Top);

    // Translate (and possibly resize) the button bounding box.
    //
    CopyMem (&this->m_pButton->ButtonBounds, &Bounds, sizeof(SWM_RECT));

    // Translate (and possibly resize) button text bounding box.
    //
    SWM_RECT    *TextRect   = &this->m_pButton->ButtonTextBounds;
    UINT32      TextWidth   = (TextRect->Right - TextRect->Left);
    UINT32      TextHeight  = (TextRect->Bottom - TextRect->Top);

    Bounds.Left     += TextOffsetX;
    Bounds.Top      += TextOffsetY;
    Bounds.Right    = ((Bounds.Left + TextWidth) < Bounds.Right ? (Bounds.Right + TextWidth) : Bounds.Right);
    Bounds.Bottom   = ((Bounds.Top + TextHeight) < Bounds.Bottom ? (Bounds.Top + TextHeight) : Bounds.Bottom);

    CopyMem (&this->m_pButton->ButtonTextBounds, &Bounds, sizeof(SWM_RECT));

    return Status;
}


static
EFI_STATUS
GetControlBounds (IN  Button    *this,
                  OUT SWM_RECT  *pBounds)
{
    EFI_STATUS Status = EFI_SUCCESS;

    CopyMem (pBounds, &this->m_pButton->ButtonBounds, sizeof(SWM_RECT));

    return Status;
}


static
EFI_STATUS
SetControlState (IN Button          *this,
                 IN OBJECT_STATE    State)
{
    this->m_pButton->State = State;

    return EFI_SUCCESS;
}

static
OBJECT_STATE
GetControlState(IN Button         *this)
{
    return this->m_pButton->State;
}

static
EFI_STATUS
CopySettings (IN Button  *this,
              IN Button  *prev) {
    UINTN   TextSize;

    this->m_pButton->State = prev->m_pButton->State;

    if (NULL != this->m_pButton->pButtonText)
    {
        FreePool (this->m_pButton->pButtonText);
    }

    TextSize = StrSize (prev->m_pButton->pButtonText);

    this->m_pButton->pButtonText = AllocateCopyPool (TextSize,prev->m_pButton->pButtonText);

    return EFI_SUCCESS;
}


static
OBJECT_STATE
Draw (IN    Button              *this,
      IN    BOOLEAN             DrawHighlight,
      IN    SWM_INPUT_STATE     *pInputState,
      OUT   VOID                **pSelectionContext)
{
    EFI_STATUS  Status      = EFI_SUCCESS;
    SWM_RECT    *pRect      = &this->m_pButton->ButtonBounds;
    VOID        *Context    = NULL;


    // If there is no input state, simply draw the button then return.
    //
    if ((NULL == pInputState) || (this->m_pButton->State == GRAYED))
    {
        Status = RenderButton (this,
                               DrawHighlight
                              );

        goto Exit;
    }

    // If there is user keyboard input, handle it here.  For buttons, we only recognize
    // <ENTER> and <SPACE> as valid keys.  Both select the button.
    //
    if (SWM_INPUT_TYPE_KEY == pInputState->InputType)
    {
        EFI_KEY_DATA *pKey = &pInputState->State.KeyState;

        if (CHAR_CARRIAGE_RETURN == pKey->Key.UnicodeChar || L' ' == pKey->Key.UnicodeChar)
        {
            this->m_pButton->State = SELECT;
            Context = this->m_pSelectionContext;
        }
        else
        {
            // Unrecognized keyboard input - simply exit.
            //
            goto Exit;
        }

        // Draw the button.
        //
        Status = RenderButton (this,
                               DrawHighlight
                              );

        // We're done, exit.
        //
        goto Exit;
    }

    // If there is touch input, check whether the pointer location falls within the button's bounding box.
    //
    if (SWM_INPUT_TYPE_TOUCH == pInputState->InputType &&
        pInputState->State.TouchState.CurrentX >= pRect->Left && pInputState->State.TouchState.CurrentX <= pRect->Right &&
        pInputState->State.TouchState.CurrentY >= pRect->Top  && pInputState->State.TouchState.CurrentY <= pRect->Bottom)
    {
        this->m_pButton->State = HOVER;
        if (pInputState->State.TouchState.ActiveButtons & 0x1)
        {
            this->m_ButtonDown = TRUE;
        }
        else
        {
            if (this->m_ButtonDown == TRUE)
            {
                this->m_pButton->State = SELECT;
                Context = this->m_pSelectionContext;
            }
        }
    }
    else
    {
        if (KEYDEFAULT != this->m_pButton->State)
        {
            this->m_pButton->State = NORMAL;
        }
        this->m_ButtonDown = FALSE;
    }

    // Draw the button.
    //
    Status = RenderButton (this,
                           DrawHighlight
                          );

Exit:

    if (NULL != pSelectionContext)
    {
        *pSelectionContext = Context;
    }

    return (this->m_pButton->State);
}


//////////////////////////////////////////////////////////////////////////////
//
//
static
VOID
Ctor(IN struct _Button                *this,
     IN SWM_RECT                       ButtonBox,
     IN EFI_FONT_INFO                  *FontInfo,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pNormalColor,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pHoverColor,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pSelectColor,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pGrayOutTextColor,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pButtonRingColor,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pNormalTextColor,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pSelectTextColor,
     IN CHAR16                         *pButtonText,
     IN VOID                           *pSelectionContext)
{
    UINT32  MaxGlyphDescent;

    // Initialize variables.
    //
    this->m_FontInfo = DupFontInfo (FontInfo);
    if (NULL == this->m_FontInfo)
    {
        goto Exit;
    }

    this->m_NormalColor     = *pNormalColor;
    this->m_HoverColor      = *pHoverColor;
    this->m_SelectColor     = *pSelectColor;
    this->m_GrayOutTextColor= *pGrayOutTextColor;
    this->m_ButtonRingColor = *pButtonRingColor;
    this->m_NormalTextColor = *pNormalTextColor;
    this->m_SelectTextColor = *pSelectTextColor;
    this->m_ButtonDown      = FALSE;

    this->m_pButton = AllocateZeroPool(sizeof(ButtonDisplayInfo));
    ASSERT(NULL != this->m_pButton);

    if (NULL == this->m_pButton)
    {
        goto Exit;
    }

    // Save the associated selection context pointer.
    //
    this->m_pSelectionContext = pSelectionContext;

    // Save pointer to the button's text.
    //
    this->m_pButton->pButtonText = AllocateCopyPool (StrSize(pButtonText),pButtonText);

    // Save the button bounding box.
    //
    CopyMem(&this->m_pButton->ButtonBounds, &ButtonBox, sizeof(SWM_RECT));

    // Calculate cell text bounding rectangle (cell text should be vertically centered in the cell).
    //
    CopyMem(&this->m_pButton->ButtonTextBounds, &ButtonBox, sizeof(SWM_RECT));

    GetTextStringBitmapSize (pButtonText,
                             FontInfo,
                             TRUE,
                             EFI_HII_OUT_FLAG_CLIP |
                             EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                             EFI_HII_IGNORE_LINE_BREAK,
                             &this->m_pButton->ButtonTextBounds,
                             &MaxGlyphDescent
                            );

    UINT32 ButtonWidth  = (this->m_pButton->ButtonBounds.Right - this->m_pButton->ButtonBounds.Left + 1);
    UINT32 ButtonHeight = (this->m_pButton->ButtonBounds.Bottom - this->m_pButton->ButtonBounds.Top + 1);
    UINT32 StringWidth  = (this->m_pButton->ButtonTextBounds.Right - this->m_pButton->ButtonTextBounds.Left + 1);
    UINT32 StringHeight = (this->m_pButton->ButtonTextBounds.Bottom - this->m_pButton->ButtonTextBounds.Top + 1);

    this->m_pButton->ButtonTextBounds.Left      += ((ButtonWidth / 2) - (StringWidth / 2));
    this->m_pButton->ButtonTextBounds.Right     += ((ButtonWidth / 2) - (StringWidth / 2));
    this->m_pButton->ButtonTextBounds.Top       += ((ButtonHeight / 2) - ((StringHeight - MaxGlyphDescent) / 2));
    this->m_pButton->ButtonTextBounds.Bottom    += ((ButtonHeight / 2) - ((StringHeight - MaxGlyphDescent) / 2));

    // Configure button state.
    //
    // TODO - function should take a flag to indicate default selection.
    //
    this->m_pButton->State      = NORMAL;

    // Member Variables
    this->Base.ControlType      = BUTTON;

    // Functions.
    //(SetControlBoundsFunctionPtr)




    this->Base.Draw             = (DrawFunctionPtr) &Draw;
    this->Base.SetControlBounds = (SetControlBoundsFunctionPtr) &SetControlBounds;
    this->Base.GetControlBounds = (GetControlBoundsFunctionPtr) &GetControlBounds;
    this->Base.SetControlState  = (SetControlStateFunctionPtr) &SetControlState;
    this->Base.GetControlState  = (GetControlStateFunctionPtr) &GetControlState;
    this->Base.CopySettings     = (CopySettingsFunctionPtr) &CopySettings;

Exit:

    return;
}


static
VOID Dtor(VOID *this)
{
    Button *privthis = (Button *)this;

    if (NULL != privthis->m_pButton->pButtonText)
    {
        FreePool(privthis->m_pButton->pButtonText);
    }

    if (NULL != privthis->m_pButton)
    {
        FreePool(privthis->m_pButton);
    }

    if (NULL != privthis->m_FontInfo)
    {
        FreePool(privthis->m_FontInfo);
    }

    if (NULL != privthis)
    {
        FreePool(privthis);
    }

    return;
}


//////////////////////////////////////////////////////////////////////////////
// Public
//
Button *new_Button(IN UINT32                            OrigX,
                   IN UINT32                            OrigY,
                   IN UINT32                            ButtonWidth,
                   IN UINT32                            ButtonHeight,
                   IN EFI_FONT_INFO                     *FontInfo,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *pNormalColor,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *pHoverColor,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *pSelectColor,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *pGrayOutTextColor,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *pButtonRingColor,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *pNormalTextColor,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *pSelectTextColor,
                   IN CHAR16                            *pButtonText,
                   IN VOID                              *pSelectionContext)
{

    SWM_RECT    Rect, TempRect;
    UINT32      MaxGlyphDescent;

    Button *B = (Button *)AllocateZeroPool(sizeof(Button));
    ASSERT(NULL != B);

    if (NULL != B)
    {
        B->Ctor         = &Ctor;
        B->Base.Dtor    = &Dtor;

        // Set origin corners.
        Rect.Left       = OrigX;
        Rect.Top        = OrigY;

        // Adjust for width, if provided.
        Rect.Right  = (ButtonWidth != SUI_BUTTON_AUTO_SIZE) ? (OrigX + ButtonWidth - 1) : OrigX;
        // Adjust for height, if provided.
        Rect.Bottom = (ButtonHeight != SUI_BUTTON_AUTO_SIZE) ? (OrigY + ButtonHeight - 1) : OrigY;

        // If either of the dimensions should be set automatically,
        // determine the text dimensions.
        // We have to do this here because the constructor consumes a RECT, not width or height.
        if ((ButtonWidth == SUI_BUTTON_AUTO_SIZE) || (ButtonHeight == SUI_BUTTON_AUTO_SIZE))
        {
            // Yes, this is wasteful and redundant.
            // Don't care.
            GetTextStringBitmapSize (pButtonText,
                                     FontInfo,
                                     FALSE,
                                     EFI_HII_OUT_FLAG_CLIP |
                                     EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                                     EFI_HII_IGNORE_LINE_BREAK,
                                     &TempRect,
                                     &MaxGlyphDescent
                                    );

            Rect.Right = Rect.Left + (TempRect.Right - TempRect.Left) + SUI_BUTTON_HIGHLIGHT_X_PAD;     // Add px to allow space for the "Tab highlight".
            Rect.Bottom = Rect.Top + (TempRect.Bottom - TempRect.Top) + SUI_BUTTON_HIGHLIGHT_Y_PAD;     // Add px to allow space for the "Tab highlight".
        }

        B->Ctor(B,
                Rect,
                FontInfo,
                pNormalColor,
                pHoverColor,
                pSelectColor,
                pGrayOutTextColor,
                pButtonRingColor,
                pNormalTextColor,
                pSelectTextColor,
                pButtonText,
                pSelectionContext
               );
    }

    return B;
}


VOID delete_Button(IN Button *this)
{
    if (NULL != this)
    {
        this->Base.Dtor((VOID *)this);
    }
}
