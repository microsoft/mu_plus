/**
Module Name:

    MsUiThemeLibCommon.c

Abstract:

    This module will provide routines to access the MsUiThemeProtocol

Environment:

    UEFI pre-boot Driver Execution Environment (DXE).

  Copyright (c) 2016 - 2018, Microsoft Corporation

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

#include <Uefi.h>                                     // UEFI base types
#include <Protocol/MsUiThemeProtocol.h>
#include <Library/DebugLib.h>
#include <Library/MsUiThemeLib.h>
#include <Library/UefiBootServicesTableLib.h>

MS_UI_THEME_DESCRIPTION *gPlatformTheme;



#define FONT_DEBUG EFI_DEBUG
#if FONT_DEBUG
VOID DumpFontInfo (MS_UI_FONT_DESCRIPTION *Font) {
    DEBUG((DEBUG_VERBOSE,"CellH=%d, CellW=%d, Advance=%d\n",
                Font->CellHeight,
                Font->CellWidth,
                Font->MaxAdvance));
    DEBUG((DEBUG_VERBOSE,"Package Size=%d, GlyphsSize=%d\n",
                Font->PackageSize,
                Font->GlyphsSize));
    DebugDumpMemory (DEBUG_VERBOSE,PACKAGE_PTR_GET Font->Package,64,DEBUG_DM_PRINT_ADDRESS);
    DebugDumpMemory (DEBUG_VERBOSE,PACKAGE_PTR_GET Font->Glyphs,64,DEBUG_DM_PRINT_ADDRESS);
}

#endif
/**
 * For controls that scale to text size, this function scales
 * the number of pixels to the current theme scale.
 *
 *   uses the formula:
 *
 *     return = ((PixelCount * Scale) + 50)) / 100;
 *
 * @param PixelCount
 *
 * @return UINTN PixelCount adjusted to current scale
 */
UINT32
EFIAPI
MsUiScaleByTheme (UINT32 PixelCount) {

    return (UINT32) (((PixelCount * gPlatformTheme->Scale) + 50) / 100);
}

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
MsUiGetSmallOSKFontHeight (VOID) {

    return (FONT_PTR_GET gPlatformTheme->SmallOSKFont)->CellHeight;
}

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
MsUiGetSmallOSKFontWidth (VOID) {

    return (FONT_PTR_GET gPlatformTheme->SmallOSKFont)->CellWidth;
}

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
MsUiGetSmallOSKFontMaxAdvance (VOID) {

    return (FONT_PTR_GET gPlatformTheme->SmallOSKFont)->MaxAdvance;
}

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
MsUiGetSmallOSKFontGlyphs (VOID) {

    return (UINT8 *)(GLYPH_PTR_GET (FONT_PTR_GET gPlatformTheme->SmallOSKFont)->Glyphs);
}

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
MsUiGetSmallFontHeight (VOID) {

    return (FONT_PTR_GET gPlatformTheme->SmallFont)->CellHeight;
}

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
MsUiGetSmallFontWidth (VOID) {

    return (FONT_PTR_GET gPlatformTheme->SmallFont)->CellWidth;
}

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
MsUiGetSmallFontMaxAdvance (VOID) {

    return (FONT_PTR_GET gPlatformTheme->SmallFont)->MaxAdvance;
}

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
MsUiGetSmallFontGlyphs (VOID) {

    return (UINT8 *)(GLYPH_PTR_GET (FONT_PTR_GET gPlatformTheme->SmallFont)->Glyphs);
}

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
MsUiGetStandardFontHeight (VOID) {

    return (FONT_PTR_GET gPlatformTheme->StandardFont)->CellHeight;
}

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
MsUiGetStandardFontMWidth (VOID) {

    return (FONT_PTR_GET gPlatformTheme->StandardFont)->CellWidth;
}

/**
 * MsUiGetStandardFontWidth
 *
 *  Returns the font Width
 *
 *
 * @return Standard Font CellWidth
 */
UINT16
EFIAPI
MsUiGetStandardFontMaxAdvance (VOID) {

    return (FONT_PTR_GET gPlatformTheme->StandardFont)->MaxAdvance;
}

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
MsUiGetStandardFontGlyphs (VOID) {

    return (UINT8 *)(GLYPH_PTR_GET (FONT_PTR_GET gPlatformTheme->StandardFont)->Glyphs);
}

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
MsUiGetMediumFontHeight (VOID) {

    return (FONT_PTR_GET gPlatformTheme->MediumFont)->CellHeight;
}

/**
 * MsUiGetMediumFontWidth
 *
 *  Returns the Width
 *
 *
 * @return Medium Font Width
 */
UINT16
EFIAPI
MsUiGetMediumFontWidth (VOID) {

    return (FONT_PTR_GET gPlatformTheme->MediumFont)->CellWidth;
}

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
MsUiGetMediumFontMaxAdvance (VOID) {

    return (FONT_PTR_GET gPlatformTheme->MediumFont)->MaxAdvance;
}

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
MsUiGetMediumFontGlyphs (VOID) {

    return (UINT8 *)(GLYPH_PTR_GET (FONT_PTR_GET gPlatformTheme->MediumFont)->Glyphs);
}

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
MsUiGetLargeFontHeight (VOID) {

    return (FONT_PTR_GET gPlatformTheme->LargeFont)->CellHeight;
}

/**
 * MsUiGetLargeFontCellWidth
 *
 *  Returns the CellWidth
 *
 *
 * @return Large Font Width
 */
UINT16
EFIAPI
MsUiGetLargeFontWidth (VOID) {

    return (FONT_PTR_GET gPlatformTheme->LargeFont)->CellWidth;
}

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
MsUiGetLargeFontMaxAdvance (VOID) {

    return (FONT_PTR_GET gPlatformTheme->LargeFont)->MaxAdvance;
}

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
MsUiGetLargeFontGlyphs (VOID) {

    return (UINT8 *)(GLYPH_PTR_GET (FONT_PTR_GET gPlatformTheme->LargeFont)->Glyphs);
}

/**
 * MsUiGetFixedFontHeight
 *
 *  Returns the cell height of the fixed font
 *
 *
 * @return Fixed Font Height
 */
UINT16
EFIAPI
MsUiGetFixedFontHeight (VOID) {

    return (FONT_PTR_GET gPlatformTheme->FixedFont)->CellHeight;
}

/**
 * MsUiGetFixedFontWidth
 *
 *  Returns the CellWidth
 *
 *
 * @return Fixed Font Width
 */
UINT16
EFIAPI
MsUiGetFixedFontWidth (VOID) {

    return (FONT_PTR_GET gPlatformTheme->FixedFont)->CellWidth;
}

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
MsUiGetFixedFontMaxAdvance (VOID) {

    return (FONT_PTR_GET gPlatformTheme->FixedFont)->MaxAdvance;
}

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
MsUiGetFixedFontGlyphs (VOID) {

    return (UINT8 *)(GLYPH_PTR_GET (FONT_PTR_GET gPlatformTheme->FixedFont)->Glyphs);
}

/**
 *  Returns the Platform Theme
 *
 *
 * @return MS_UI_THEME_DESCROPTION
 */
MS_UI_THEME_DESCRIPTION *
EFIAPI
MsUiGetPlatformTheme (VOID) {

#if FONT_DEBUG

    static BOOLEAN FirstTime = TRUE;

    ASSERT (gPlatformTheme != NULL);
    if (FirstTime) {

        DebugDumpMemory (DEBUG_VERBOSE, gPlatformTheme, sizeof(MS_UI_THEME_DESCRIPTION),DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
        DEBUG((DEBUG_VERBOSE,__FUNCTION__ " Theme information\n"));
        DEBUG((DEBUG_VERBOSE,"Scale = %d\n",gPlatformTheme->Scale));
        DEBUG((DEBUG_VERBOSE,"Fixed Font\n"));
        DumpFontInfo (FONT_PTR_GET gPlatformTheme->FixedFont);
        DEBUG((DEBUG_VERBOSE,"Small Font\n"));
        DumpFontInfo (FONT_PTR_GET gPlatformTheme->SmallFont);
        DEBUG((DEBUG_VERBOSE,"Standard Font\n"));
        DumpFontInfo (FONT_PTR_GET gPlatformTheme->StandardFont);
        DEBUG((DEBUG_VERBOSE,"Medium Font\n"));
        DumpFontInfo (FONT_PTR_GET gPlatformTheme->MediumFont);
        DEBUG((DEBUG_VERBOSE,"Large Font\n"));
        DumpFontInfo (FONT_PTR_GET gPlatformTheme->LargeFont);
    }
    FirstTime = FALSE;

#endif

    return gPlatformTheme;    // PlatformTheme
}

