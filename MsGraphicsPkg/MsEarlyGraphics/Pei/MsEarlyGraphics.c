/** @file

  This file contains functions to capture the video mode information for
  the Dxe MsEarlyGraphics driver.

  Copyright (c) 2015 - 2018, Microsoft Corporation.

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

#include "MsEarlyGraphics.h"


// Global variables.
//

MS_EARLY_GRAPHICS_PROTOCOL                            mEarlyGraphicsProtocol;
MS_UI_THEME_DESCRIPTION                              *gPlatformTheme;

GLOBAL_REMOVE_IF_UNREFERENCED EFI_PEI_PPI_DESCRIPTOR  mMsEarlyGraphicsPpiList = {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gMsEarlyGraphicsProtocolGuid,
    &mEarlyGraphicsProtocol
};

/**
    UpdateFrameBufferAddress

    Used to update the buffer pointer after PCI Enumeration.
*/
EFI_STATUS
EFIAPI
    UpdateFrameBufferBase(MS_EARLY_GRAPHICS_PROTOCOL * this) {

    //PEI doesn't have to update the frame buffer base
    return EFI_SUCCESS;
}


/**
  Main entry point for this driver.

  @param    FileHandle      PEI file handle.
  @param    PeiServices     Pointer to an array of PEI Foundation service function pointers.

  @retval   EFI_STATUS      Initialization was successful.
  @retval   Others          The operation failed.

**/
EFI_STATUS
EFIAPI
MsEarlyGraphicsEntry (
    IN  EFI_PEI_FILE_HANDLE     FileHandle,
    IN  CONST EFI_PEI_SERVICES  **PeiServices
)
{
    EFI_STATUS                            Status = EFI_SUCCESS;
    EFI_HOB_GUID_TYPE                    *GuidHob;
    MS_EARLY_GRAPHICS_HOB_DATA           *HobData = NULL;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE    *Mode;

    gPlatformTheme  = MsUiGetPlatformTheme ();

    if (gPlatformTheme == NULL) {
        DEBUG((DEBUG_ERROR,"Unable to locate fonts for MsEarlyGraphics\n"));
        return EFI_NOT_FOUND;
    }

    Status = MsEarlyGraphicsGetFrameBufferInfo (&Mode);
    if (EFI_ERROR(Status )) {
        return Status;
    }

    // Create a HoB for passing the PEI graphics informaiont up to the DXE MsEarlyGraphics
    //
    Status = PeiServicesCreateHob (
                                  EFI_HOB_TYPE_GUID_EXTENSION,
                                  (UINT16)(sizeof(EFI_HOB_GUID_TYPE) +
                                           sizeof(MS_EARLY_GRAPHICS_HOB_DATA)),
                                  (VOID **) &GuidHob
                                  );
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Failed to create HoB for passing PEI debug log to DXE: %r \n", Status));
        return Status;
    }

    GuidHob->Name = gMsEarlyGraphicsHobGuid;

    HobData = (MS_EARLY_GRAPHICS_HOB_DATA *)(GuidHob+1);

    // The GOP structures are not suitable for use in DXE due to use of UINTN and pointers.
    // So, copy the fields to properly size fields for DXE and send that in the Hob.
    HobData->MaxMode                       = Mode->MaxMode;
    HobData->Mode                          = Mode->Mode;
    HobData->SizeOfInfo                    = Mode->SizeOfInfo;
    HobData->FrameBufferBase               = Mode->FrameBufferBase;
    HobData->FrameBufferSize               = Mode->FrameBufferSize;
    HobData->Version                       = Mode->Info->Version;
    HobData->HorizontalResolution          = Mode->Info->HorizontalResolution;
    HobData->VerticalResolution            = Mode->Info->VerticalResolution;
    HobData->PixelFormat                   = (UINT32) Mode->Info->PixelFormat;
    HobData->PixelInformation.RedMask      = Mode->Info->PixelInformation.RedMask;
    HobData->PixelInformation.GreenMask    = Mode->Info->PixelInformation.GreenMask;
    HobData->PixelInformation.BlueMask     = Mode->Info->PixelInformation.BlueMask;
    HobData->PixelInformation.ReservedMask = (UINT32)Mode->Info->PixelInformation.ReservedMask;
    HobData->PixelsPerScanLine             = Mode->Info->PixelsPerScanLine;

    DEBUG((DEBUG_INFO,"Mode=%p, Info=%p, FrameBfr=%p\n",Mode, Mode->Info,Mode->FrameBufferBase));
    GuidHob->Name = gMsEarlyGraphicsHobGuid;

    // Publish the early graphics PPI
    mEarlyGraphicsProtocol.Signature             = MS_EARLY_GRAPHICS_PROTOCOL_SIGNATURE;
    mEarlyGraphicsProtocol.Version               = MS_EARLY_GRAPHICS_VERSION;
    mEarlyGraphicsProtocol.Maxrows               = Mode->Info->VerticalResolution / GetCellHeight();
    mEarlyGraphicsProtocol.Maxcolumns            = Mode->Info->HorizontalResolution / GetCellWidth();
    mEarlyGraphicsProtocol.UpdateFrameBufferBase = UpdateFrameBufferBase;
    mEarlyGraphicsProtocol.SimpleBlt             = SimpleBlt;
    mEarlyGraphicsProtocol.SimpleFill            = SimpleFill;
    mEarlyGraphicsProtocol.PrintLn               = PrintLn;
    mEarlyGraphicsProtocol.Mode                  = Mode;

    Status = PeiServicesInstallPpi(&mMsEarlyGraphicsPpiList);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Failed to publish the EarlyGraphics PPI: %r \n", Status));
        Status = EFI_SUCCESS;
    }

    return Status;
}

