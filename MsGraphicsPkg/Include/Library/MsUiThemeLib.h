/** @file
  Defines the UI Theme settings.

  This library provides the fonts to be used by the settings UI.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
