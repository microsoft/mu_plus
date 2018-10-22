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
#include <Library/HobLib.h>
#include <Library/MsUiThemeLib.h>


extern MS_UI_THEME_DESCRIPTION *gPlatformTheme = NULL;


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

