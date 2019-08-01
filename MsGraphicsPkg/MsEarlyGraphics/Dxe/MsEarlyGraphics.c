/**@file  MsEarlyDxeGraphics

This code is used to display Preboot information on the graphics console 
initialed by a silicon providers early graphics module

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MsEarlyGraphics.h"

// GlobalVariables

EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE   *mMode = NULL;
MS_EARLY_GRAPHICS_PROTOCOL           mEarlyGraphicsProtocol;

/**
    UpdateFrameBufferAddress

    Used to update the buffer pointer after PCI Enumeration.
*/
EFI_STATUS
EFIAPI
UpdateFrameBufferBase(
IN MS_EARLY_GRAPHICS_PROTOCOL   *this
) {
    return MsEarlyGraphicsGetFrameBufferInfo(&this->Mode);
}

/**
  Entry to MsEarlyDxeGraphics.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.
  @return Status         From internal routine or boot object

**/
EFI_STATUS
EFIAPI
MsEarlyGraphicsEntry(
IN EFI_HANDLE                            ImageHandle,
IN EFI_SYSTEM_TABLE                      *SystemTable
) {
    EFI_STATUS  Status;

    //
    // First call will populate mMode including the frame buffer base
    //
    Status = MsEarlyGraphicsGetFrameBufferInfo(&mMode);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"Unable to initialize driver context. Status %r\n",Status));
        return EFI_DEVICE_ERROR;
    }

    mEarlyGraphicsProtocol.Signature             = MS_EARLY_GRAPHICS_PROTOCOL_SIGNATURE;
    mEarlyGraphicsProtocol.Version               = MS_EARLY_GRAPHICS_VERSION;
    mEarlyGraphicsProtocol.Maxrows               = mMode->Info->VerticalResolution / GetCellHeight();
    mEarlyGraphicsProtocol.Maxcolumns            = mMode->Info->HorizontalResolution / GetCellWidth();
    mEarlyGraphicsProtocol.UpdateFrameBufferBase = UpdateFrameBufferBase;
    mEarlyGraphicsProtocol.SimpleBlt             = SimpleBlt;
    mEarlyGraphicsProtocol.SimpleFill            = SimpleFill;
    mEarlyGraphicsProtocol.PrintLn               = PrintLn;
    mEarlyGraphicsProtocol.Mode                  = mMode;

    Status = gBS->InstallProtocolInterface(&ImageHandle,
                                           &gMsEarlyGraphicsProtocolGuid,
                                           EFI_NATIVE_INTERFACE,
                                           &mEarlyGraphicsProtocol);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"Unable to install EarlyGraphics protocol. Code=%r.\n",Status));
        return EFI_DEVICE_ERROR;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MsEarlyGraphicsUnload (IN EFI_HANDLE ImageHandle) {

    gBS->UninstallProtocolInterface(
        ImageHandle,
        &gMsEarlyGraphicsProtocolGuid,
        &mEarlyGraphicsProtocol
        );
    return EFI_SUCCESS;
}
