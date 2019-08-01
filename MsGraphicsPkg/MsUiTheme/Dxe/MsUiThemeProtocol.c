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