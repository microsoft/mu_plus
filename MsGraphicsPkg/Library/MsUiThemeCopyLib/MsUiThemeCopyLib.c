/**

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
