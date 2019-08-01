/**

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/MsUiThemeCopyLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

/* COPY FONT generates this code for each font

    NewFonts->SmallFont = FONT_PTR_SET FontCopy;
    CopyMem (FontCopy, (FONT_PTR_GET mPlatformTheme->SmallFont), sizeof(MS_UI_FONT_DESCRIPTION));
    FontCopy += sizeof(MS_UI_FONT_DESCRIPTION);
    CopyMem (FontCopy, PACKAGE_PTR_GET (FONT_PTR_GET mPlatformTheme->SmallFont)->Package, (FONT_PTR_GET mPlatformTheme->SmallFont)->PackageSize);
    (FONT_PTR_GET NewFonts->SmallFont)->Package = PACKAGE_PTR_SET FontCopy;
    FontCopy += (FONT_PTR_GET mPlatformTheme->SmallFont)->PackageSize;
    CopyMem (FontCopy, GLYPH_PTR_GET (FONT_PTR_GET mPlatformTheme->SmallFont)->Glyphs, (FONT_PTR_GET mPlatformTheme->SmallFont)->PackageSize);
    FontCopy += (FONT_PTR_GET mPlatformTheme->SmallFont)->GlyphsSize;

*/

#define COPY_FONT(target,bufptr,source,font) \
    target->font = FONT_PTR_SET bufptr; \
    CopyMem (bufptr, (FONT_PTR_GET source->font), sizeof(MS_UI_FONT_DESCRIPTION)); \
    bufptr += sizeof(MS_UI_FONT_DESCRIPTION); \
    (FONT_PTR_GET target->font )->Package = PACKAGE_PTR_SET bufptr; \
    CopyMem (bufptr, PACKAGE_PTR_GET (FONT_PTR_GET source->font)->Package, (FONT_PTR_GET source->font)->PackageSize); \
    bufptr += (FONT_PTR_GET source->font)->PackageSize; \
    (FONT_PTR_GET target->font)->Glyphs = GLYPH_PTR_SET bufptr; \
    CopyMem (bufptr, GLYPH_PTR_GET (FONT_PTR_GET source->font)->Glyphs, (FONT_PTR_GET source->font)->GlyphsSize); \
    bufptr += (FONT_PTR_GET source->font)->GlyphsSize;

UINT32
EFIAPI
MsThemeGetSize(
    IN CONST MS_UI_THEME_DESCRIPTION *Theme
)
{
    UINT32 FontSize;

    ASSERT(Theme != NULL);

    FontSize = sizeof(MS_UI_THEME_DESCRIPTION) +

               sizeof(MS_UI_FONT_DESCRIPTION) +
               (FONT_PTR_GET Theme->FixedFont)->PackageSize + (FONT_PTR_GET Theme->FixedFont)->GlyphsSize +

               sizeof(MS_UI_FONT_DESCRIPTION) +
               (FONT_PTR_GET Theme->SmallOSKFont)->PackageSize + (FONT_PTR_GET Theme->SmallOSKFont)->GlyphsSize +

               sizeof(MS_UI_FONT_DESCRIPTION) +
               (FONT_PTR_GET Theme->SmallFont)->PackageSize + (FONT_PTR_GET Theme->SmallFont)->GlyphsSize +

               sizeof(MS_UI_FONT_DESCRIPTION) +
               (FONT_PTR_GET Theme->StandardFont)->PackageSize + (FONT_PTR_GET Theme->StandardFont)->GlyphsSize +

               sizeof(MS_UI_FONT_DESCRIPTION) +
               (FONT_PTR_GET Theme->MediumFont)->PackageSize + (FONT_PTR_GET Theme->MediumFont)->GlyphsSize +

               sizeof(MS_UI_FONT_DESCRIPTION) +
               (FONT_PTR_GET Theme->LargeFont)->PackageSize + (FONT_PTR_GET Theme->LargeFont)->GlyphsSize;

    return FontSize;
}

EFI_STATUS
EFIAPI
MsThemeCopy(
    OUT MS_UI_THEME_DESCRIPTION *       Dest,
    UINT32                              DestBytes,
    IN CONST MS_UI_THEME_DESCRIPTION *  Source
)
{
    UINT32  ThemeSize;
    CHAR8 * ThemeCopy;

    ThemeSize = MsThemeGetSize(Source);
    if (DestBytes < ThemeSize)
        return EFI_INVALID_PARAMETER;

    ThemeCopy = (CHAR8 *) Dest;
    CopyMem (ThemeCopy, Source, sizeof(MS_UI_THEME_DESCRIPTION));
    ThemeCopy += sizeof(MS_UI_THEME_DESCRIPTION);

    COPY_FONT(Dest,ThemeCopy,Source,FixedFont);
    COPY_FONT(Dest,ThemeCopy,Source,SmallOSKFont);
    COPY_FONT(Dest,ThemeCopy,Source,SmallFont);
    COPY_FONT(Dest,ThemeCopy,Source,StandardFont);
    COPY_FONT(Dest,ThemeCopy,Source,MediumFont);
    COPY_FONT(Dest,ThemeCopy,Source,LargeFont);

    return EFI_SUCCESS;
}
