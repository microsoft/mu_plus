/** @file
  Defines the UI Theme settings.

  This library provides the fonts to be used by the settings UI.

  Copyright (c) 2016 - 2018, Microsoft Corporation.

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

#ifndef __MS_UI_THEME_LIB_H__
#define __MS_UI_THEME_LIB_H__

#include <Protocol/MsUiThemeProtocol.h>

/**
 * MsUiScaleByTheme
 *
 * For controls that scale to text size, this function scales
 * the number of pixels to the current theme scale.
 *
 *   uses the formula:
 *
 *     return = ((PixelCount * Scale) + (scale/2)) / 100;
 *
 * @param PixelCount
 *
 * @return PixelCount adjusted to current scale
 */
UINT32
EFIAPI
MsUiScaleByTheme (
    IN UINT32  PixelCount );

/**
 * MsUiGetSmallFontSize
 *
 *  Returns the cell height of the small font
 *
 *
 * @return Small Font Height
 */
UINT16
EFIAPI
MsUiGetSmallOSKFontHeight (VOID);


/**
 * MsUiGetSmallFontCellWidth
 *
 *  Returns the CellWidth
 *
 *
 * @return Small Font CellWidth
 */
UINT16
EFIAPI
MsUiGetSmallOSKFontWidth (VOID);

/**
 * MsUiGetSmallFontMaxAdvance
 *
 *  Returns the pointer to the glyphs
 *
 *
 * @return Small Font Glyphs
 */
UINT16
EFIAPI
MsUiGetSmallOSKFontMaxAdvance (VOID);

/**
 * MsUiGetSmallFontGlyphs
 *
 *  Returns the pointer to the glyphs
 *
 *
 * @return Small Font Glyphs
 */
UINT8 *
EFIAPI
MsUiGetSmallOSKFontGlyphs (VOID);

/**
 * MsUiGetSmallFontSize
 *
 *  Returns the cell height of the small font
 *
 *
 * @return Small Font Height
 */
UINT16
EFIAPI
MsUiGetSmallFontHeight (VOID);


/**
 * MsUiGetSmallFontCellWidth
 *
 *  Returns the CellWidth
 *
 *
 * @return Small Font CellWidth
 */
UINT16
EFIAPI
MsUiGetSmallFontWidth (VOID);

/**
 * MsUiGetSmallFontMaxAdvance
 *
 *  Returns the pointer to the glyphs
 *
 *
 * @return Small Font Glyphs
 */
UINT16
EFIAPI
MsUiGetSmallFontMaxAdvance (VOID);

/**
 * MsUiGetSmallFontGlyphs
 *
 *  Returns the pointer to the glyphs
 *
 *
 * @return Small Font Glyphs
 */
UINT8 *
EFIAPI
MsUiGetSmallFontGlyphs (VOID);

/**
 * MsUiGetStandardFontSize
 *
 *  Returns the cell height of the standard font
 *
 *
 * @return Standard Font Height
 */
UINT16
EFIAPI
MsUiGetStandardFontHeight (VOID);

/**
 * MsUiGetStandardFontCellWidth
 *
 *  Returns the CellWidth
 *
 *
 * @return Standard Font CellWidth
 */
UINT16
EFIAPI
MsUiGetStandardFontWidth (VOID);

/**
 * MsUiGetStandardFontMaxAdvance
 *
 *  Returns the font MaxAdvance
 *
 *
 * @return Standard Font MaxAdvance
 */
UINT16
EFIAPI
MsUiGetStandardFontMaxAdvance (VOID);

/**
 * MsUiGetStandardFontGlyphs
 *
 *  Returns the pointer to the glyphs
 *
 *
 * @return Standard Font Glyphs
 */
UINT8 *
EFIAPI
MsUiGetStandardFontGlyphs (VOID);

/**
 * MsUiGetMediumFontSize
 *
 *  Returns the cell height of the medium font
 *
 *
 * @return Medium Font Height
 */
UINT16
EFIAPI
MsUiGetMediumFontHeight (VOID);

/**
 * MsUiGetMediumFontCellWidth
 *
 *  Returns the CellWidth
 *
 *
 * @return Medium Font CellWidth
 */
UINT16
EFIAPI
MsUiGetMediumFontWidth (VOID);

/**
 * MsUiGetMediumFontMaxAdvance
 *
 *  Returns the font MaxAdvance
 *
 *
 * @return Medium Font MaxAdvance
 */
UINT16
EFIAPI
MsUiGetMediumFontMaxAdvance (VOID);

/**
 * MsUiGetMediumFontGlyphs
 *
 *  Returns the pointer to the glyphs
 *
 *
 * @return Medium Font Glyphs
 */
UINT8 *
EFIAPI
MsUiGetMediumFontGlyphs (VOID);

/**
 * MsUiGetLargeFontSize
 *
 *  Returns the cell height of the large font
 *
 *
 * @return Large Font Height
 */
UINT16
EFIAPI
MsUiGetLargeFontHeight (VOID);

/**
 * MsUiGetLargeFontCellWidth
 *
 *  Returns the CellWidth
 *
 *
 * @return Large Font CellWidth
 */
UINT16
EFIAPI
MsUiGetLargeFontWidth (VOID);

/**
 * MsUiGetLargeFontMaxAdvance
 *
 *  Returns the font MaxAdvance
 *
 *
 * @return Large Font MaxAdvance
 */
UINT16
EFIAPI
MsUiGetLargeFontMaxAdvance (VOID);

/**
 * MsUiGetLargeFontGlyphs
 *
 *  Returns the pointer to the glyphs
 *
 *
 * @return Large Font Glyphs
 */
UINT8 *
EFIAPI
MsUiGetLargeFontGlyphs (VOID);

/**
 * MsUiGetFixedFontSize
 *
 *  Returns the cell height of the fixed font
 *
 *
 * @return Fixed Font Height
 */
UINT16
EFIAPI
MsUiGetFixedFontHeight (VOID);

/**
 * MsUiGetFixedFontCellWidth
 *
 *  Returns the CellWidth
 *
 *
 * @return Fixed Font CellWidth
 */
UINT16
EFIAPI
MsUiGetFixedFontWidth (VOID);

/**
 * MsUiGetFixedFontMaxAdvance
 *
 *  Returns the font MaxAdvance
 *
 *
 * @return Fixed Font MaxAdvance
 */
UINT16
EFIAPI
MsUiGetFixedFontMaxAdvance (VOID);

/**
 * MsUiGetFixedFontGlyphs
 *
 *  Returns the pointer to the glyphs
 *
 *
 * @return Fixed Font Glyphs
 */
UINT8 *
EFIAPI
MsUiGetFixedFontGlyphs (VOID);

/**
 *  Returns the Platform Theme
 *
 *
 * @return MS_UI_THEME_DESCROPTION
 */
MS_UI_THEME_DESCRIPTION *
EFIAPI
MsUiGetPlatformTheme (VOID);

#endif // __MS_UI_THEME_LIB_H__
