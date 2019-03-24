/** @file

  Implements a Simple UI Toolkit utility functions.

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


#define MS_DEFAULT_FONT_SIZE       MsUiGetStandardFontHeight ()   // Default font size is 32px high.  (TODO - merge with MsDisplayEngine.h copy).

/**
Calculates the bitmap width and height of the specified text string based on the current font size & style.

@param[in]     pString              The string to measure.
@param[in]     FontInfo             Font information (defines size, style, etc.).
@param[in]     BoundsLimit          TRUE == bounding rectangle restriction, FALSE == no restrction (only limit is the total screen size).
@param[in out] Bounds               On entry, contains the absolute bounds to be imposed on the string.  On exit, contains the actual string bounds.
@param[out]    MaxFontGlyphDescent  Maximum font glyph descent (pixels) for the selected font.

@retval EFI_SUCCESS     The operation completed successfully.

**/
EFI_STATUS
GetTextStringBitmapSize (IN     CHAR16           *pString,
                         IN     EFI_FONT_INFO    *FontInfo,
                         IN     BOOLEAN           BoundsLimit,
                         IN     EFI_HII_OUT_FLAGS HiiFlags,
                         IN OUT SWM_RECT         *Bounds,
                         OUT    UINT32           *MaxFontGlyphDescent)
{
    EFI_STATUS              Status = EFI_SUCCESS;
    EFI_FONT_DISPLAY_INFO   *StringInfo     = NULL;
    EFI_IMAGE_OUTPUT        *BltBuffer      = NULL;
    EFI_HII_ROW_INFO        *StringRowInfo  = NULL;
    UINTN                   RowInfoSize;
    UINT32                  RowIndex;
    UINT16                  Width, Height;
    CHAR16                 *xString;


    // Calculate maximum width and height allowed by the specified bounding rectangle.
    //
    if (TRUE == BoundsLimit)
    {
        Width    = (UINT16)(Bounds->Right - Bounds->Left + 1);
        Height   = (UINT16)(Bounds->Bottom - Bounds->Top + 1);
    }
    else
    {
        // Caller hasn't provided any boundary to enforce.  Assume we have the whole screen.
        //
        ZeroMem (Bounds, sizeof(SWM_RECT));
        Width    = (UINT16)mUITGop->Mode->Info->HorizontalResolution;
        Height   = (UINT16)mUITGop->Mode->Info->VerticalResolution;
    }

    // Get the current preferred font size and style.
    //
    StringInfo = BuildFontDisplayInfoFromFontInfo (FontInfo);
    if (NULL == StringInfo)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    StringInfo->FontInfoMask = EFI_FONT_INFO_ANY_FONT;

    // If a null string was provided, return a standard single character-sizes rectangle.  Null strings are used for UI padding/alignment.
    //
    if (NULL == pString || L'\0' == *pString)
    {
        xString = L" ";
//        Bounds->Right   = (Bounds->Left + MS_DEFAULT_FONT_SIZE);        // Use for both width and height.
//        Bounds->Bottom  = (Bounds->Top + MS_DEFAULT_FONT_SIZE);
//        goto CalcFontDescent;
    } else {
        xString = pString;
    }

    // Prepare string blitting buffer.
    //
    BltBuffer = (EFI_IMAGE_OUTPUT *) AllocatePool (sizeof (EFI_IMAGE_OUTPUT));

    ASSERT (NULL != BltBuffer);
    if (NULL == BltBuffer)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Fill out string blit buffer details.
    //
    BltBuffer->Width         = Width;
    BltBuffer->Height        = Height;
    BltBuffer->Image.Bitmap  = AllocatePool (Width * Height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

    ASSERT (NULL != BltBuffer->Image.Bitmap);
    if (NULL == BltBuffer->Image.Bitmap)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Send in a NULL pointer so we can receive back width and height results.
    //
    StringRowInfo  = (EFI_HII_ROW_INFO *)NULL;
    RowInfoSize = 0;

    mUITSWM->StringToWindow (mUITSWM,
                             mClientImageHandle,
                             HiiFlags,
                             xString,
                             StringInfo,
                             &BltBuffer,
                             0,
                             0,
                             &StringRowInfo,
                             &RowInfoSize,
                             (UINTN *)NULL
                            );

    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR, "ERROR [SUIT]: Failed to calculate string bitmap size: %r.\n", Status));
        goto Exit;
    }

    if (NULL == StringRowInfo || 0 == RowInfoSize)
    {
        goto Exit;
    }

    // Calculate the bounding rectangle around the text as rendered (note this it may be in multiple rows).
    //
    for (RowIndex=0, Width=0, Height=0 ; RowIndex < RowInfoSize ; RowIndex++)
    {
        Width   = (UINT16)(Width < (UINT32)StringRowInfo[RowIndex].LineWidth ? (UINT32)StringRowInfo[RowIndex].LineWidth : Width);
        Height += (UINT16)StringRowInfo[RowIndex].LineHeight;
    }

    // Adjust the caller's right and bottom bounding box limits based on the results.
        //
    Bounds->Right    = (Bounds->Left + Width - 1);
    Bounds->Bottom   = (Bounds->Top + Height - 1);

    DEBUG ((DEBUG_VERBOSE, "INFO [SUIT]: Calculated string bitmap size (Actual=L%d,R%d,T%d,B%d  MaxWidth=%d  MaxHeight=%d  TextRows=%d).\n", Bounds->Left, Bounds->Right, Bounds->Top, Bounds->Bottom, Width, Height, RowInfoSize));

//CalcFontDescent:

        // Determine the maximum font descent value from the font selected.
        // TODO - Need a better way to determine this.  Currently hard-coded based on knowledge of the custom registered fonts in the Simple Window Manager driver.
        //

//        if (StringInfo.FontInfo.FontSize == MsUiGetFixedFontHeight ()) {
            *MaxFontGlyphDescent = 0;
//        } else {
//            *MaxFontGlyphDescent =  (StringInfo.FontInfo.FontSize * 20) / 100;
//        }

Exit:
    // Free the buffers.
    //
    if (NULL != BltBuffer && NULL != BltBuffer->Image.Bitmap)
    {
        FreePool(BltBuffer->Image.Bitmap);
    }
    if (NULL != BltBuffer)
    {
        FreePool(BltBuffer);
    }
    if (NULL != StringRowInfo)
    {
        FreePool(StringRowInfo);
    }
    if (NULL != StringInfo)
    {
        FreePool (StringInfo);
    }



    return Status;
}
// Given two canvas, find the "control" that is in "this" list, and return the eqivalent control
// from "prev" list.

UIT_CANVAS_CHILD_CONTROL *
EFIAPI
GetEquivalentControl (IN UIT_CANVAS_CHILD_CONTROL *Control,
                      IN Canvas                   *src,       // Canvas that has the source Control
                      IN Canvas                   *tgt) {     // Canvas that has the target control

    UIT_CANVAS_CHILD_CONTROL    *pSrcChildControl  = src->m_pControls;
    UIT_CANVAS_CHILD_CONTROL    *pTgtChildControl  = tgt->m_pControls;
    ControlBase                 *pSrcControlBase;
    ControlBase                 *pTgtControlBase;

    if (Control == NULL) {
        return NULL;
    }

    while (pSrcChildControl != NULL) {
        if (pTgtChildControl == NULL) {
            DEBUG((DEBUG_ERROR, "%a control mismatch - Tgt = NULL\n", __FUNCTION__));
            return NULL;
        }

        pSrcControlBase = (ControlBase *) pSrcChildControl->pControl;
        pTgtControlBase = (ControlBase *) pTgtChildControl->pControl;
        if (pSrcControlBase->ControlType != pTgtControlBase->ControlType) {
            DEBUG((DEBUG_ERROR, "%a control mismatch. Src=%d, Tgt=%d\n",__FUNCTION__, pSrcControlBase->ControlType, pTgtControlBase->ControlType));
            return NULL;
        }

        if (pSrcChildControl == Control) {
            return pTgtChildControl;
        }
        pSrcChildControl = pSrcChildControl->pNext;
        pTgtChildControl = pTgtChildControl->pNext;
    }
    if (pTgtChildControl != NULL) {
        DEBUG((DEBUG_ERROR, "%a control mismatch - Srct = NULL\n", __FUNCTION__));
    }
    return NULL;
}


/**
    Draws a rectangular outline to the screen at location and in the size, line width, and color specified.

    @param[in]      X                   Line X-coordinate starting position within Bitmap.
    @param[in]      Y                   Line Y-coordinate starting position within Bitmap.
    @param[in]      NumberOfPixels      Number of pixels making up the line, extending from (X,Y) to the right.
    @param[in]      Color               Line color.
    @param[out]     Bitmap              Bitmap buffer that will be draw into.

    @retval         EFI_SUCCESS

**/

EFI_STATUS
EFIAPI
DrawRectangleOutline (IN UINT32                        OrigX,
                      IN UINT32                        OrigY,
                      IN UINT32                        Width,
                      IN UINT32                        Height,
                      IN UINT32                        LineWidth,
                      IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Color)
{
    EFI_STATUS  Status = EFI_SUCCESS;


    if (NULL == mUITGop || NULL == Color)
    {
        Status = EFI_INVALID_PARAMETER;
        goto Exit;
    }

    // For performance (and visual) reasons, it's better to render four individual "line" blits rather than a solid rectangle fill.
    //
    mUITSWM->BltWindow (mUITSWM,
                        mClientImageHandle,
                        Color,
                        EfiBltVideoFill,
                        0,
                        0,
                        OrigX,
                        OrigY,
                        Width,
                        LineWidth,
                        0
                       );

    mUITSWM->BltWindow (mUITSWM,
                        mClientImageHandle,
                        Color,
                        EfiBltVideoFill,
                        0,
                        0,
                        OrigX,
                        (OrigY + Height - LineWidth),
                        Width,
                        LineWidth,
                        0
                       );

    mUITSWM->BltWindow (mUITSWM,
                        mClientImageHandle,
                        Color,
                        EfiBltVideoFill,
                        0,
                        0,
                        OrigX,
                        OrigY,
                        LineWidth,
                        Height,
                        0
                       );

    mUITSWM->BltWindow (mUITSWM,
                        mClientImageHandle,
                        Color,
                        EfiBltVideoFill,
                        0,
                        0,
                        (OrigX + Width - LineWidth),
                        OrigY,
                        LineWidth,
                        Height,
                        0
                       );

Exit:

    return Status;
}


/**
    Returns a copy of the FONT_INFO structure.

    @param[in]      FontInfo            Pointer to callers FONT_INFO.

    @retval         NewFontInfo         Copy of FONT_INFO.  Caller must free.
                    NULL                No memory resources available
**/
EFI_FONT_INFO *
EFIAPI
DupFontInfo (IN EFI_FONT_INFO *FontInfo)
{

    EFI_FONT_INFO  *NewFontInfo;
    UINTN           FontNameSize;

    if (NULL == FontInfo) {
        FontNameSize = 0;
    } else {
        FontNameSize = StrnLenS (FontInfo->FontName, MAX_FONT_NAME_SIZE) * sizeof(FontInfo->FontName[0]);
        if (FontNameSize > MAX_FONT_NAME_SIZE) {
            FontNameSize = 0;
        }
    }

   NewFontInfo =  AllocatePool (sizeof (EFI_FONT_INFO) + FontNameSize);

   CopyMem (NewFontInfo, FontInfo, sizeof (EFI_FONT_INFO) + FontNameSize);
   if (FontNameSize <= sizeof(FontInfo->FontName[0])) {
      NewFontInfo->FontName[0] = '\0';
   }

   return NewFontInfo;
}

/**
    Returns a new FontDisplayInfo populated with callers FontInfo.

    @param[in]      FontInfo             Pointer to callers FONT_INFO.

    @retval         NewFontDisplayInfo   New FontDisplayIfo with font from FontInfo
                                         Caller must free.
                    NULL                 No Resources available
**/
EFI_FONT_DISPLAY_INFO *
EFIAPI
BuildFontDisplayInfoFromFontInfo (IN EFI_FONT_INFO *FontInfo)
{

    EFI_FONT_DISPLAY_INFO  *NewFontDisplayInfo;
    UINTN                   FontNameSize;

    if (NULL == FontInfo) {
        FontNameSize = 0;
    } else {
        FontNameSize = StrnLenS (FontInfo->FontName, MAX_FONT_NAME_SIZE) * sizeof(FontInfo->FontName[0]);
        if (FontNameSize > MAX_FONT_NAME_SIZE) {
            FontNameSize = 0;
        }
    }

   NewFontDisplayInfo =  AllocateZeroPool (sizeof (EFI_FONT_DISPLAY_INFO) + FontNameSize);
   if (NULL != NewFontDisplayInfo) {
       CopyMem (&NewFontDisplayInfo->FontInfo, FontInfo, sizeof (EFI_FONT_INFO) + FontNameSize);
       if (FontNameSize <= sizeof(FontInfo->FontName[0])) {
           NewFontDisplayInfo->FontInfo.FontName[0] = L'\0';
       }
   }

   return NewFontDisplayInfo;
}