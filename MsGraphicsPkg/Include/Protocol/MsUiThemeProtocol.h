/** @file
  Defines the UI Theme settings.

  This protocol provides the fonts and settings to be used by the settings UI.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MS_UI_THEME_PROTOCOL_H__
#define __MS_UI_THEME_PROTOCOL_H__

#include <Uefi.h>


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
    // General Purpose regsion of the Theme
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
