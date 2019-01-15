/**
Module Name:

    MsUiThemeProtocol.c

Abstract:

 *  This module published the UI Fonts and Theme settings.

Environment:

    UEFI pre-boot Driver Execution Environment (DXE).

  Copyright (C) 2016 - 2018, Microsoft Corporation.

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
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Protocol/MsUiThemeProtocol.h>
#include <Library/UefiBootServicesTableLib.h>

#if FixedPcdGetBool(PcdUiThemeInDxe)

#include <Library/PlatformThemeLib.h>
#include <Library/MsUiThemeCopyLib.h>

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
    )
{
    EFI_STATUS                  Status;
    MS_UI_THEME_DESCRIPTION *   OriginalTheme;
    MS_UI_THEME_DESCRIPTION *   ThemeCopy;
    UINTN                       PageCount;
    UINT32                      ThemeSize;

    OriginalTheme = PlatformThemeGet();
    ASSERT(OriginalTheme != NULL);
    ASSERT (OriginalTheme->Signature == MS_UI_THEME_PROTOCOL_SIGNATURE );

    ThemeSize = MsThemeGetSize(OriginalTheme);
    ASSERT(ThemeSize > 0);

    PageCount = EFI_SIZE_TO_PAGES(ThemeSize);

    Status = gBS->AllocatePages(AllocateAnyPages, EfiBootServicesData, PageCount, (EFI_PHYSICAL_ADDRESS *)&ThemeCopy);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    Status = MsThemeCopy(ThemeCopy, ThemeSize, OriginalTheme);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
    {
        gBS->FreePages((EFI_PHYSICAL_ADDRESS)ThemeCopy, PageCount);
        return Status;
    }

    Status = gBS->InstallProtocolInterface (&ImageHandle,& gMsUiThemeProtocolGuid, EFI_NATIVE_INTERFACE, ThemeCopy);
    ASSERT_EFI_ERROR(Status);
    
    return Status;
}

#else

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
    MS_UI_THEME_DESCRIPTION *PlatformTheme;

    // Locate the MsUeiThemePpi data.
    //
    GuidHob = GetFirstGuidHob(&gMsUiThemeHobGuid);
    if (NULL != GuidHob) {
        //
        PlatformTheme = * ((MS_UI_THEME_DESCRIPTION **) (UINTN)(GET_GUID_HOB_DATA(GuidHob)));
        if (PlatformTheme != NULL) {
            ASSERT (PlatformTheme->Signature == MS_UI_THEME_PROTOCOL_SIGNATURE );
            Status = gBS->InstallProtocolInterface (&ImageHandle,& gMsUiThemeProtocolGuid, EFI_NATIVE_INTERFACE, PlatformTheme);
        }
    }
    ASSERT_EFI_ERROR( Status );
    return Status;
}

#endif