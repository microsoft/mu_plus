/**@file  MsEarlyDxeGraphics

This code is used to display Preboot information on the graphics console 
initialed by a silicon providers early graphics module

Copyright (C) 2018 Microsoft Corporation.

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
