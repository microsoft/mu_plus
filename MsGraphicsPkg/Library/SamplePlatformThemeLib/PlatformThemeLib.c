/**
Module Name:

    PlatformThemeLib.c

Abstract:

    This module will provide the fonts used in the  UI

Environment:

    UEFI

  Copyright (c) 2018, Microsoft Corporation

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
#include <Library/PlatformThemeLib.h>


#define FONT_DECL(TABLE, NAME ) \
  \
    static MS_UI_FONT_DESCRIPTION TABLE = { \
        MS_UI_CUSTOM_FONT_ ## NAME ## _CELL_HEIGHT, \
        MS_UI_CUSTOM_FONT_ ## NAME ## _CELL_WIDTH, \
        MS_UI_CUSTOM_FONT_ ## NAME ## _MAX_ADVANCE, \
        sizeof (mMsUiFontPackageHdr_ ## NAME), \
        sizeof (mMsUiFontPackageGlyphs_ ## NAME), \
        FONT_PTR_SET &mMsUiFontPackageHdr_ ## NAME, \
        GLYPH_PTR_SET &mMsUiFontPackageGlyphs_ ## NAME \
    };

// The fonts for this platform are:

#define SCALE 100

#include <Resources/FontPackage_Selawik_Regular_22pt.h>
FONT_DECL(FixedFont,            Selawik_Regular_22pt)

#include <Resources/FontPackage_Selawik_Regular_10pt.h>
FONT_DECL(SmallOSKFont,         Selawik_Regular_10pt)

#include <Resources/FontPackage_Selawik_Regular_24pt.h>
FONT_DECL(SmallFont,            Selawik_Regular_24pt)

#include <Resources/FontPackage_Selawik_Regular_28pt.h>
FONT_DECL(StandardFont,         Selawik_Regular_28pt)

#include <Resources/FontPackage_Selawik_Regular_36pt.h>
FONT_DECL(MediumFont,           Selawik_Regular_36pt)

#include <Resources/FontPackage_Selawik_Regular_48pt.h>
FONT_DECL(LargeFont,            Selawik_Regular_48pt)


static  MS_UI_THEME_DESCRIPTION gMsUiPlatformTheme = {
    MS_UI_THEME_PROTOCOL_SIGNATURE,
    MS_UI_THEME_PROTOCOL_VERSION,
    SCALE,
    0,
    FONT_PTR_SET &FixedFont,
    FONT_PTR_SET &SmallOSKFont,
    FONT_PTR_SET &SmallFont,
    FONT_PTR_SET &StandardFont,
    FONT_PTR_SET &MediumFont,
    FONT_PTR_SET &LargeFont
};

MS_UI_THEME_DESCRIPTION *
EFIAPI
PlatformThemeGet ( VOID ) {

    return &gMsUiPlatformTheme;
}



