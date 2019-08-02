/**
Module Name:

    MsUiThemeProtocol.c

Abstract:

 *  This module published the UI Fonts and Theme settings.

Environment:

    UEFI pre-boot Driver Execution Environment (DXE).

  Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Uefi.h>                                     // UEFI base types
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Protocol/MsUiThemeProtocol.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  Entry to MsUiThemeProtocol

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.
  @return Status         From internal routine or boot object

 **/
EFI_STATUS
EFIAPI
MsUiThemeProtocolEntry(
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE    *SystemTable
    ) {
    EFI_STATUS              Status = EFI_NOT_FOUND;
    EFI_HOB_GUID_TYPE      *GuidHob;


    // Locate the MsUeiThemePpi data.
    //
    GuidHob = GetFirstGuidHob(&gMsUiThemeHobGuid);
    if (NULL != GuidHob) {
        //
        mPlatformTheme = * ((MS_UI_THEME_DESCRIPTION **) (UINTN)(GET_GUID_HOB_DATA(GuidHob)));
        if (mPlatformTheme != NULL) {
            ASSERT (mPlatformTheme->Signature == MS_UI_THEME_PROTOCOL_SIGNATURE );
            Status = gBS->InstallProtocolInterface (&ImageHandle,& gMsUiThemeProtocolGuid, EFI_NATIVE_INTERFACE, mPlatformTheme);
        }
    }
    DEBUG((DEBUG_ERROR,"Unable to find Theme, or install theme protocol\n"));
    ASSERT_EFI_ERROR( Status );
    return Status;
}
