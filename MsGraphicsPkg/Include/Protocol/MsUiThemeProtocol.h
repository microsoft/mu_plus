/** @file
  Defines the UI Theme settings.

  This protocol provides the fonts and settigns to be used by the settings UI.

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

#ifndef __MS_UI_THEME_PROTOCOL_H__
#define __MS_UI_THEME_PROTOCOL_H__


// Font package definition.
//
// NOTE - The Theme structure is used in both PEI and DXE, and must be correct for both
//        32 bit and 64 bit modes.


#pragma pack (push, 1)
typedef struct _SWM_FONT_PACKAGE_tag_
{
    EFI_HII_FONT_PACKAGE_HDR FontHeader;
    CHAR16 FontFamilyNameContd[35];
} MS_UI_FONT_PACKAGE_HEADER;

#define FONT_PTR_GET    (MS_UI_FONT_DESCRIPTION *) (UINTN)
#define FONT_PTR_SET    (EFI_PHYSICAL_ADDRESS) (UINTN)

#define PACKAGE_PTR_GET (MS_UI_FONT_PACKAGE_HEADER *) (UINTN)
#define PACKAGE_PTR_SET (EFI_PHYSICAL_ADDRESS) (UINTN)

#define GLYPH_PTR_GET   (CHAR8 * ) (UINTN)
#define GLYPH_PTR_SET   (EFI_PHYSICAL_ADDRESS) (UINTN)

typedef struct {
    UINT16                     CellHeight;
    UINT16                     CellWidth;
    UINT16                     MaxAdvance;
    UINT32                     PackageSize;
    UINT32                     GlyphsSize;
    EFI_PHYSICAL_ADDRESS       Package;
    EFI_PHYSICAL_ADDRESS       Glyphs;
} MS_UI_FONT_DESCRIPTION;

#pragma pack (pop)

#define MS_UI_THEME_PROTOCOL_SIGNATURE SIGNATURE_64('U', 'I', ' ', 'T', 'H', 'E', 'M', 'E')
#define MS_UI_THEME_PROTOCOL_VERSION   1

// MsUiTheme Protocol structure is the same as
// MsUiTheme Ppi

typedef struct {
    UINT64                   Signature;         // Force alignment for proper pointers.
    UINT32                   Version;
    // General Puropose regsion of the Theme
    UINT16                   Scale;
    UINT16                   Reserved1;         // Value is % * 100 (ie 25% == 25)
    // Fonts for this theme.
    EFI_PHYSICAL_ADDRESS    FixedFont;          // Access font pointers as (FONT_PTR mThemePtr->FixedFont)
    EFI_PHYSICAL_ADDRESS    SmallOSKFont;       // For OSK on 800x600 display
    EFI_PHYSICAL_ADDRESS    SmallFont;
    EFI_PHYSICAL_ADDRESS    StandardFont;
    EFI_PHYSICAL_ADDRESS    MediumFont;
    EFI_PHYSICAL_ADDRESS    LargeFont;

    // Control specifics for Themes would go here.
    //

    // Part 1 of themes is to apply different fonts to different platforms

} MS_UI_THEME_DESCRIPTION;

extern EFI_GUID gMsUiThemeProtocolGuid;
extern EFI_GUID gMsUiThemePpiGuid;
extern EFI_GUID gMsUiThemeHobGuid;

#endif // __MS_UI_THEME_PROTOCOL_H__
