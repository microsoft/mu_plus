/**

Module Name:

    MsUiThemeLib.c

Abstract:

    This module will provide routines to access the MsUiThemeProtocol

Environment:

    UEFI pre-boot Driver Execution Environment (DXE).

  Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>                                     // UEFI base types
#include <Protocol/MsUiThemeProtocol.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MsUiThemeLib.h>


extern MS_UI_THEME_DESCRIPTION *gPlatformTheme;


/**
  The constructor function locates the Theme PPI

  @param  FileHandle   The handle of FFS header the loaded driver.
  @param  PeiServices  The pointer to the PEI services.

  @retval EFI_SUCCESS  The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
MsUiThemeLibConstructor (
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
    EFI_HOB_GUID_TYPE                    *GuidHob;

    GuidHob = GetFirstGuidHob (&gMsUiThemeHobGuid);
    ASSERT (GuidHob != NULL);
    gPlatformTheme = * ((MS_UI_THEME_DESCRIPTION **) (UINTN)(GET_GUID_HOB_DATA(GuidHob)));
    ASSERT (gPlatformTheme != NULL );
    if (gPlatformTheme != 0) {
        ASSERT (gPlatformTheme->Signature == MS_UI_THEME_PROTOCOL_SIGNATURE );
        if (gPlatformTheme->Signature != MS_UI_THEME_PROTOCOL_SIGNATURE) {
            gPlatformTheme = NULL;
        }
    }

    return EFI_SUCCESS;
}

