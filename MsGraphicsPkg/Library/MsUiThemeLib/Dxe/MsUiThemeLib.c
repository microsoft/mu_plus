/**
Module Name:

    MsUiThemeLib.c

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

