/** @file

  Implements a Simple UI Toolkit utility functions.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UIT_UTILITIES_H_
#define _UIT_UTILITIES_H_

// Preprocessor constants.
//
#define UIT_INVALID_SELECTION              (UINT32)-1
#define MAX_FONT_NAME_SIZE                 256


/**
Calculates the bitmap width and height of the specified text string based on the current font size & style.

@param[in]     pString                        The string to measure.
@param[in]     FontInfo                   Font information (defines size, style, etc.).
@param[in]     BoundsLimit              TRUE == bounding rectangle restriction, FALSE == no restrction (only limit is the total screen size).
@param[in out] Bounds                 On entry (if NoBounds == FALSE), contains the absolute bounds to be imposed on the string.  On exit, contains the actual string bounds.
@param[out]    MaxFontGlyphDescent  Maximum font glyph descent (pixels) for the selected font.

@retval EFI_SUCCESS     The operation completed successfully.

**/
EFI_STATUS
EFIAPI
GetTextStringBitmapSize (IN     CHAR16           *pString,
                         IN     EFI_FONT_INFO    *FontInfo,
                         IN     BOOLEAN           BoundsLimit,
                         IN     EFI_HII_OUT_FLAGS HiiFlags,
                         IN OUT SWM_RECT         *Bounds,
                         OUT    UINT32           *MaxFontGlyphDescent);


// Given two canvas, find the "control" that is "InThisList" that is the eqivalent control
// "InOtherList"
UIT_CANVAS_CHILD_CONTROL *
EFIAPI
GetEquivalentControl (IN UIT_CANVAS_CHILD_CONTROL *Control,
                      IN Canvas                   *InThisList,
                      IN Canvas                   *InOtherList);


/**
    Draws a rectangular outline to the screen at location and in the size, line width, and color specified.

    @param[in]      this                Pointer to a ToggleSwitch.
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
                      IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Color);


/**
    Returns a copy of the FONT_INFO structure.

    @param[in]      FontInfo            Pointer to callers FONT_INFO.

    @retval         NewFontInfo         Copy of FONT_INFO.  Caller must free.

**/

EFI_FONT_INFO *
EFIAPI
DupFontInfo (IN EFI_FONT_INFO *FontInfo);

/**
    Returns a new FontDisplayInfo populated with callers FontInfo.

    @param[in]      FontInfo             Pointer to callers FONT_INFO.

    @retval         NewFontDisplayInfo   New FontDisplayIfo with font from FontInfo
                                         Caller must free.

**/

EFI_FONT_DISPLAY_INFO *
EFIAPI
BuildFontDisplayInfoFromFontInfo (IN EFI_FONT_INFO *FontInfo);

#endif  // _UIT_UTILITIES_H_.
