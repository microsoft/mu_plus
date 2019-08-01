/**

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MS_UI_THEME_COPY_LIB_H_
#define _MS_UI_THEME_COPY_LIB_H_

#include <Uefi.h>
#include <Protocol/MsUiThemeProtocol.h>

UINT32
EFIAPI
MsThemeGetSize(
    IN CONST MS_UI_THEME_DESCRIPTION *Theme
    );

EFI_STATUS
EFIAPI
MsThemeCopy(
    OUT MS_UI_THEME_DESCRIPTION *       Dest,
    UINT32                              DestBytes,
    IN CONST MS_UI_THEME_DESCRIPTION *  Source
    );


#endif  // _MS_UI_THEME_COPY_LIB_H_

