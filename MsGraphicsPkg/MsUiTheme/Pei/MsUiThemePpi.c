/**
Module Name:

    MsUiThemePpi.c

Abstract:

 *  This module published the UI Fonts and Theme settings.

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
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PlatformThemeLib.h>

#include <Library/MsUiThemeCopyLib.h>

#if FeaturePcdGet(PcdUiThemeInDxe)
#error UiTheme configured to be in DXE - should not be building or using this PPI
#endif

MS_UI_THEME_DESCRIPTION *mPlatformTheme;

GLOBAL_REMOVE_IF_UNREFERENCED EFI_PEI_PPI_DESCRIPTOR  mMsUiThemePpiList = {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gMsUiThemePpiGuid,
    NULL
};

/**
  Entry to MsUiThemeProtocol

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.
  @return Status         From internal routine or boot object

 **/
EFI_STATUS
EFIAPI
MsUiThemePpiEntry(
    IN  EFI_PEI_FILE_HANDLE     FileHandle,
    IN  CONST EFI_PEI_SERVICES  **PeiServices
    ) {
    EFI_STATUS               Status;
    EFI_HOB_GUID_TYPE       *GuidHob;
    EFI_PHYSICAL_ADDRESS    *HobData;
    UINT32                   FontSize;
    EFI_PHYSICAL_ADDRESS     FontCopyPhys;
    MS_UI_THEME_DESCRIPTION *NewFonts;

    mPlatformTheme = PlatformThemeGet();
    DEBUG((DEBUG_INFO,"MsUiThemePpi started.  Table at %p for %d\n",mPlatformTheme,sizeof(MS_UI_THEME_DESCRIPTION)));

    DEBUG((DEBUG_VERBOSE,"Dumping static font table.  Table at %p for %d\n",mPlatformTheme,sizeof(MS_UI_THEME_DESCRIPTION)));
    DebugDumpMemory (DEBUG_VERBOSE, mPlatformTheme, sizeof(MS_UI_THEME_DESCRIPTION),DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    DebugDumpMemory (DEBUG_VERBOSE, (FONT_PTR_GET mPlatformTheme->FixedFont), sizeof (MS_UI_FONT_DESCRIPTION),DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    DebugDumpMemory (DEBUG_VERBOSE, PACKAGE_PTR_GET (FONT_PTR_GET mPlatformTheme->FixedFont)->Package, sizeof(MS_UI_FONT_PACKAGE_HEADER),DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    DebugDumpMemory (DEBUG_VERBOSE, PACKAGE_PTR_GET (FONT_PTR_GET mPlatformTheme->FixedFont)->Glyphs,  256,DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    DebugDumpMemory (DEBUG_VERBOSE, (FONT_PTR_GET mPlatformTheme->LargeFont), sizeof (MS_UI_FONT_DESCRIPTION),DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    DebugDumpMemory (DEBUG_VERBOSE, PACKAGE_PTR_GET (FONT_PTR_GET mPlatformTheme->LargeFont)->Package, sizeof(MS_UI_FONT_PACKAGE_HEADER),DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    DebugDumpMemory (DEBUG_VERBOSE, PACKAGE_PTR_GET (FONT_PTR_GET mPlatformTheme->LargeFont)->Glyphs,  256,DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);

    FontSize = MsThemeGetSize(mPlatformTheme);

    DEBUG((DEBUG_INFO,"Font Size=%d\n",FontSize));

    Status = PeiServicesAllocatePages (EfiBootServicesData, EFI_SIZE_TO_PAGES (FontSize), &FontCopyPhys);
    ASSERT_EFI_ERROR(Status);

    NewFonts = (MS_UI_THEME_DESCRIPTION *) (UINTN) FontCopyPhys;

    Status = MsThemeCopy(NewFonts, FontSize, mPlatformTheme);

    DEBUG((DEBUG_VERBOSE,"Font Stats Fp=%p, size=%d\n",NewFonts,FontSize));
    DEBUG((DEBUG_VERBOSE,"Dumping new font table.  Table at %p for %d\n",&mPlatformTheme,sizeof(MS_UI_THEME_DESCRIPTION)));
    DebugDumpMemory (DEBUG_VERBOSE, NewFonts, sizeof(MS_UI_THEME_DESCRIPTION),DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    DebugDumpMemory (DEBUG_VERBOSE, (FONT_PTR_GET NewFonts->FixedFont), sizeof (MS_UI_FONT_DESCRIPTION),DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    DebugDumpMemory (DEBUG_VERBOSE, PACKAGE_PTR_GET (FONT_PTR_GET NewFonts->FixedFont)->Package, sizeof(MS_UI_FONT_PACKAGE_HEADER),DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    DebugDumpMemory (DEBUG_VERBOSE, PACKAGE_PTR_GET (FONT_PTR_GET NewFonts->FixedFont)->Glyphs,  256,DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    DebugDumpMemory (DEBUG_VERBOSE, (FONT_PTR_GET NewFonts->LargeFont), sizeof (MS_UI_FONT_DESCRIPTION),DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    DebugDumpMemory (DEBUG_VERBOSE, PACKAGE_PTR_GET (FONT_PTR_GET NewFonts->LargeFont)->Package, sizeof(MS_UI_FONT_PACKAGE_HEADER),DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    DebugDumpMemory (DEBUG_VERBOSE, PACKAGE_PTR_GET (FONT_PTR_GET NewFonts->LargeFont)->Glyphs,  256,DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);

    // Create a HoB for passing the PEI font tables up to the DXE MsUiThemeProtocol
    //
    Status = PeiServicesCreateHob (
                                  EFI_HOB_TYPE_GUID_EXTENSION,
                                  (UINT16)(sizeof(EFI_HOB_GUID_TYPE) +
                                           sizeof(UINT64)),
                                  (VOID **) &GuidHob
                                  );
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Failed to create HoB for passing Font information to DXE: %r \n", Status));
        return Status;
    }
    DEBUG((DEBUG_VERBOSE,"Font Hob=%p\n",GuidHob));
    GuidHob->Name = gMsUiThemeHobGuid;

    HobData = (EFI_PHYSICAL_ADDRESS *) (GuidHob+1);
    *HobData = (EFI_PHYSICAL_ADDRESS) (UINTN) NewFonts;

    DEBUG((DEBUG_VERBOSE,"Font Hob=%p, HobData=%p NewFonts = *HobData = %p\n",GuidHob,HobData,*HobData));
    DebugDumpMemory (DEBUG_VERBOSE,GuidHob,sizeof(EFI_HOB_GUID_TYPE)+sizeof(UINT64) + 8 ,DEBUG_DM_PRINT_ADDRESS);

    // Publish the Ppi for MsEarlyGraphics
    mMsUiThemePpiList.Ppi = NewFonts;
    Status = PeiServicesInstallPpi(&mMsUiThemePpiList);

    ASSERT_EFI_ERROR(Status);

    return Status;
}
