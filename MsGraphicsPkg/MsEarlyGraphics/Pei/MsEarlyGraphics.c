/** @file

  This file contains functions to capture the video mode information for
  the Dxe MsEarlyGraphics driver.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MsEarlyGraphics.h"


// Global variables.
//

MS_EARLY_GRAPHICS_PROTOCOL                            mEarlyGraphicsProtocol;

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

    // Create a HoB for passing the PEI graphics information up to the DXE MsEarlyGraphics
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

