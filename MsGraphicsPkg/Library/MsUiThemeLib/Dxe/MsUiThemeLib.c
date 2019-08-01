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
#include <Library/MsUiThemeLib.h>
#include <Library/UefiBootServicesTableLib.h>

extern MS_UI_THEME_DESCRIPTION *gPlatformTheme;

/**
  Constructor fo MsUiThemeLib

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.
  @return Status         From internal routine or boot object

 **/
EFI_STATUS
EFIAPI
MsUiThemeLibConstructor (
    IN EFI_HANDLE                            ImageHandle,
    IN EFI_SYSTEM_TABLE                      *SystemTable
    ) {
    EFI_STATUS                          Status;


    Status = gBS->LocateProtocol (&gMsUiThemeProtocolGuid, NULL, (VOID **) &gPlatformTheme);
    ASSERT_EFI_ERROR (Status);

    return EFI_SUCCESS;
}

