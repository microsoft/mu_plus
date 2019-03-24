/** @file

  Implements a simple label control for displaying text.

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


//////////////////////////////////////////////////////////////////////////////
// Private
//
static
EFI_STATUS
UpdateLabelText (IN     Label       *this,
                 IN     CHAR16      *NewLabelText)
{
    EFI_STATUS  Status = EFI_SUCCESS;
    UINT32      MaxGlyphDescent;


    // Free the current label string buffer if there is one.
    //
    if (NULL != this->m_pLabel->pLabelText)
    {
        FreePool(this->m_pLabel->pLabelText);
    }

    // Allocate a new buffer and save the callers label text string.
    //
    this->m_pLabel->pLabelText = AllocatePool((StrLen(NewLabelText) + 1) * sizeof(CHAR16));       // Includes NULL terminator.

    ASSERT (NULL != this->m_pLabel->pLabelText);
    if (NULL == this->m_pLabel->pLabelText)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Copy the label text.
    //
    StrCpyS(this->m_pLabel->pLabelText, StrLen(NewLabelText) + 1, NewLabelText);

    // Calculate the corresponding text-as-bitmap bounding rectangle.  Start with the bounding rectangle limit.
    //
    CopyMem(&this->m_pLabel->LabelBoundsCurrent, &this->m_pLabel->LabelBoundsLimit, sizeof(SWM_RECT));

    Status = GetTextStringBitmapSize(NewLabelText,
                                     this->m_FontInfo,
                                     TRUE,
                                     EFI_HII_OUT_FLAG_WRAP,
                                     &this->m_pLabel->LabelBoundsCurrent,
                                     &MaxGlyphDescent
                                    );

Exit:

    return Status;
}

static
EFI_STATUS
RenderLabel(IN Label  *this)
{
    EFI_STATUS              Status = EFI_SUCCESS;
    EFI_FONT_DISPLAY_INFO   *StringInfo = NULL;
    EFI_IMAGE_OUTPUT        *BltBuffer = NULL;
    SWM_RECT                *LabelBounds = &this->m_pLabel->LabelBoundsCurrent;


    // Prepare string blitting buffer.
    //
    BltBuffer = (EFI_IMAGE_OUTPUT *) AllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));

    ASSERT (NULL != BltBuffer);
    if (NULL == BltBuffer)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // The blitting buffer should describe current label bounding box limits in order to wrap as specified.
    //
    //BltBuffer->Width        = (UINT16)LabelBounds->Right;
    BltBuffer->Width        = (UINT16)this->m_pLabel->LabelBoundsLimit.Right +1;       // TODO
    BltBuffer->Height       = (UINT16)LabelBounds->Bottom + 1;
    BltBuffer->Image.Screen = mUITGop;

    StringInfo = BuildFontDisplayInfoFromFontInfo (this->m_FontInfo);
    if (NULL == StringInfo)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    StringInfo->ForegroundColor = this->m_TextColor;
    StringInfo->BackgroundColor = this->m_BackgroundColor;

    StringInfo->FontInfoMask     = EFI_FONT_INFO_ANY_FONT;

    mUITSWM->StringToWindow (mUITSWM,
                             mClientImageHandle,
                             EFI_HII_OUT_FLAG_WRAP | EFI_HII_DIRECT_TO_SCREEN,
                             this->m_pLabel->pLabelText,
                             StringInfo,
                             &BltBuffer,
                             LabelBounds->Left,
                             LabelBounds->Top,
                             NULL,
                             NULL,
                             NULL
                            );

Exit:

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
SetControlBounds (IN Label      *this,
                  IN SWM_RECT   Bounds)
{
    EFI_STATUS  Status  = EFI_SUCCESS;
    INT32       XOffset = (Bounds.Left - this->m_pLabel->LabelBoundsCurrent.Left);
    INT32       YOffset = (Bounds.Top - this->m_pLabel->LabelBoundsCurrent.Top);


    // Translate (and possibly truncate) the current label bounding box.
    //
    CopyMem (&this->m_pLabel->LabelBoundsCurrent, &Bounds, sizeof(SWM_RECT));

    // Also translate the bounding box limit.
    //
    this->m_pLabel->LabelBoundsLimit.Left   += XOffset;
    this->m_pLabel->LabelBoundsLimit.Right  += XOffset;
    this->m_pLabel->LabelBoundsLimit.Top    += YOffset;
    this->m_pLabel->LabelBoundsLimit.Bottom += YOffset;


    return Status;
}


static
EFI_STATUS
GetControlBounds (IN  Label     *this,
                  OUT SWM_RECT  *pBounds)
{
    EFI_STATUS Status = EFI_SUCCESS;


    CopyMem (pBounds, &this->m_pLabel->LabelBoundsCurrent, sizeof(SWM_RECT));

    return Status;
}


static
EFI_STATUS
SetControlState (IN Label          *this,
                 IN OBJECT_STATE    State)
{
    return EFI_SUCCESS;//Object state cannot be changed
}

static
OBJECT_STATE
GetControlState(IN Label          *this)
{
    return NORMAL; //Object state not maintained for this control. Return the default
}


static
EFI_STATUS
CopySettings (IN Label  *this,
              IN Label  *prev) {

    UpdateLabelText (this,prev->m_pLabel->pLabelText);
    return EFI_SUCCESS;
}


static
OBJECT_STATE
Draw (IN    Label               *this,
      IN    BOOLEAN             DrawHighlight,
      IN    SWM_INPUT_STATE     *pInputState,
      OUT   VOID                **pSelectionContext)
{
    EFI_STATUS  Status = EFI_SUCCESS;


    // Draw the label.
    //
    Status = RenderLabel (this);

    // No selection context associated with a label.
    //
    if (NULL != pSelectionContext)
    {
        *pSelectionContext = NULL;
    }

    return NORMAL;
}


//////////////////////////////////////////////////////////////////////////////
//
//
static
VOID
Ctor(IN struct _Label                  *this,
     IN SWM_RECT                       *LabelBox,
     IN EFI_FONT_INFO                  *FontInfo,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pTextColor,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pBackgroundColor,
     IN CHAR16                         *pLabelText)
{


    // Capture requested font size and style.
    //
    this->m_FontInfo = DupFontInfo (FontInfo);
    if (NULL == this->m_FontInfo)
    {
        goto Exit;
    }

    // Capture text and background colors.
    //
    this->m_TextColor       = *pTextColor;
    this->m_BackgroundColor = *pBackgroundColor;

    // Allocate space for storing display information.
    // TODO - this needn't be a seperate structure from the label context structure.
    //
    this->m_pLabel = AllocateZeroPool(sizeof(LabelDisplayInfo));

    ASSERT(NULL != this->m_pLabel);
    if (NULL == this->m_pLabel)
    {
        goto Exit;
    }

    // Capture the bounding box as the absolute limit on the label.
    //
    CopyMem (&this->m_pLabel->LabelBoundsLimit, LabelBox, sizeof(SWM_RECT));

    // Update the label text string and associated book-keeping and cached state.
    //
    UpdateLabelText (this,
                     pLabelText
                    );

    // Member Variables
    this->Base.ControlType      = LABEL;

    // Functions.
    //
    this->Base.Draw             = (DrawFunctionPtr) &Draw;
    this->Base.SetControlBounds = (SetControlBoundsFunctionPtr) &SetControlBounds;
    this->Base.GetControlBounds = (GetControlBoundsFunctionPtr) &GetControlBounds;
    this->Base.SetControlState  = (SetControlStateFunctionPtr) &SetControlState;
    this->Base.GetControlState  = (GetControlStateFunctionPtr) &GetControlState;
    this->Base.CopySettings     = (CopySettingsFunctionPtr) &CopySettings;
    this->UpdateLabelText       = &UpdateLabelText;

Exit:

    return;
}


static
VOID Dtor(VOID *this)
{
    Label *privthis = (Label *)this;

    if (NULL != privthis->m_pLabel->pLabelText)
    {
        FreePool(privthis->m_pLabel->pLabelText);
    }

    if (NULL != privthis->m_pLabel)
    {
        FreePool(privthis->m_pLabel);
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
Label *new_Label(IN UINT32                          OrigX,
                 IN UINT32                          OrigY,
                 IN UINT32                          LabelWidth,
                 IN UINT32                          LabelHeight,
                 IN EFI_FONT_INFO                   *FontInfo,
                 IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *pTextColor,
                 IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *pBackgroundColor,
                 IN CHAR16                          *pLabelText)
{

    SWM_RECT    Rect;

    Label *L = (Label *)AllocateZeroPool(sizeof(Label));
    ASSERT(NULL != L);

    if (NULL != L)
    {
        L->Ctor         = &Ctor;
        L->Base.Dtor    = &Dtor;

        Rect.Left       = OrigX;
        Rect.Right      = (OrigX + LabelWidth - 1);
        Rect.Top        = OrigY;
        Rect.Bottom     = (OrigY + LabelHeight - 1);

        L->Ctor(L,
                &Rect,
                FontInfo,
                pTextColor,
                pBackgroundColor,
                pLabelText);
    }

    return L;
}


VOID delete_Label(IN Label *this)
{
    if (NULL != this)
    {
        this->Base.Dtor((VOID *)this);
    }
}
