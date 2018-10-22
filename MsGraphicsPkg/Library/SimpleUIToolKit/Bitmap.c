/** @file

  Implements a simple control for managing & displaying a bitmap image.

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
RenderBitmap(IN Bitmap  *this)
{
    UINT32      Width  = (this->m_BitmapBounds.Right - this->m_BitmapBounds.Left + 1);
    UINT32      Height = (this->m_BitmapBounds.Bottom - this->m_BitmapBounds.Top + 1);


    // Draw the bitmap.
    //
    mUITGop->Blt(mUITGop,
                 this->m_Bitmap,
                 EfiBltBufferToVideo,
                 0,
                 0,
                 this->m_BitmapBounds.Left,
                 this->m_BitmapBounds.Top,
                 Width,
                 Height,
                 0
                );


    return EFI_SUCCESS;
}


static
EFI_STATUS
SetControlBounds (IN Bitmap     *this,
                  IN SWM_RECT   Bounds)
{
    EFI_STATUS  Status  = EFI_SUCCESS;
    INT32       XOffset = (Bounds.Left - this->m_BitmapBounds.Left);
    INT32       YOffset = (Bounds.Top  - this->m_BitmapBounds.Top);


    // Translate (and possibly truncate) the current bitmap bounding box.
    //
    CopyMem (&this->m_BitmapBounds, &Bounds, sizeof(SWM_RECT));

    // Also translate the bounding box limit.
    //
    this->m_BitmapBounds.Left   += XOffset;
    this->m_BitmapBounds.Right  += XOffset;
    this->m_BitmapBounds.Top    += YOffset;
    this->m_BitmapBounds.Bottom += YOffset;


    return Status;
}


static
EFI_STATUS
GetControlBounds (IN  Bitmap    *this,
                  OUT SWM_RECT  *pBounds)
{
    EFI_STATUS Status = EFI_SUCCESS;


    CopyMem (pBounds, &this->m_BitmapBounds, sizeof(SWM_RECT));

    return Status;
}


static
EFI_STATUS
SetControlState (IN Bitmap          *this,
                 IN OBJECT_STATE    State)
{
    return EFI_SUCCESS; //Object State cannot be changed
}

static
OBJECT_STATE
GetControlState(IN Bitmap            *this)
{
    return NORMAL; // There is no object state maintained. Return the default
}


static
EFI_STATUS
CopySettings (IN Bitmap  *this,
              IN Bitmap  *prev)
{
    return EFI_SUCCESS;
}


static
OBJECT_STATE
Draw (IN    Bitmap              *this,
      IN    BOOLEAN             DrawHighlight,
      IN    SWM_INPUT_STATE     *pInputState,
      OUT   VOID                **pSelectionContext)
{

    // Draw the bitmap.
    //
    RenderBitmap (this);

    // No selection context associated with a bitmap.
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
Ctor(IN struct  _Bitmap                 *this,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *Bitmap,
     IN SWM_RECT                         BitmapBounds)
{
    UINT32  Width       = (BitmapBounds.Right - BitmapBounds.Left + 1);
    UINT32  Height      = (BitmapBounds.Bottom - BitmapBounds.Top + 1);
    UINT32  BitmapSize;


    // Capture the bounding box of the bitmap.
    //
    CopyMem (&this->m_BitmapBounds, &BitmapBounds, sizeof(SWM_RECT));

    // Allocate space for the bitmap and make a copy.
    //
    BitmapSize = (Width * Height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    this->m_Bitmap = AllocatePool(BitmapSize);

    ASSERT (NULL != this->m_Bitmap);
    if (NULL == this->m_Bitmap)
    {
        goto Exit;
    }

    CopyMem (this->m_Bitmap, Bitmap, BitmapSize);

    // Define the control type.
    //
    this->Base.ControlType      = BITMAP;

    // Functions.
    //
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
    Bitmap *privthis = (Bitmap *)this;

    if (NULL != privthis->m_Bitmap)
    {
        FreePool(privthis->m_Bitmap);
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
Bitmap *new_Bitmap(IN UINT32                         OrigX,
                   IN UINT32                         OrigY,
                   IN UINT32                         BitmapWidth,
                   IN UINT32                         BitmapHeight,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BitmapBuffer)
{
    SWM_RECT    Rect;


    Bitmap *B = (Bitmap *)AllocateZeroPool(sizeof(Bitmap));

    ASSERT(NULL != B);
    if (NULL != B)
    {
        B->Ctor         = &Ctor;
        B->Base.Dtor    = &Dtor;

        Rect.Left       = OrigX;
        Rect.Right      = (OrigX + BitmapWidth - 1);
        Rect.Top        = OrigY;
        Rect.Bottom     = (OrigY + BitmapHeight - 1);

        B->Ctor(B,
                BitmapBuffer,
                Rect);
    }

    return B;
}


VOID delete_Bitmap(IN Bitmap *this)
{
    if (NULL != this)
    {
        this->Base.Dtor((VOID *)this);
    }
}
