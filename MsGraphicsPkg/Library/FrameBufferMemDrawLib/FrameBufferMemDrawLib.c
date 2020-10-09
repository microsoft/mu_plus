/** @file
Library used to display things on the Frame buffer by memory copying

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/FrameBufferMemDrawLib.h>
#include <Library/FrameBufferBltLib.h>

FRAME_BUFFER_CONFIGURE* mFrameBufferConfig = NULL;
UINTN mFrameBufferConfigSize = 0;
UINT32 mModeConfigredFor = 0xFFFFF; // set to a really high mode that likely won't be supported
EFI_GRAPHICS_OUTPUT_PROTOCOL *mGraphicsOutput = NULL;

VOID
FreeFrameBufferConfig(
    VOID
) {
    if (mFrameBufferConfig != NULL) {
        FreePool(mFrameBufferConfig);
        mFrameBufferConfig = NULL;
    }
}

/***
 Setups the mFrameBufferConfig
*/
EFI_STATUS
EFIAPI
SetupFrameBufferConfig (
    VOID
) {
    EFI_STATUS Status;
    
    //DEBUG((DEBUG_INFO, "[%a %a:%d] configure enter\n", __FILE__, __FUNCTION__, __LINE__));
    //
    // Try to open GOP first
    //
    if (mGraphicsOutput == NULL) {
        Status = gBS->HandleProtocol(gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID **)&mGraphicsOutput);
        if (EFI_ERROR (Status)) {
            DEBUG((DEBUG_WARN, "%a - Failed to find GOP on ConsoleOutHandle. %r\n", __FUNCTION__, Status));
            //failed on console out.  Try globally within system
            Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&mGraphicsOutput);
        }
        // if we don't find it
        if (EFI_ERROR (Status)) {
            DEBUG((DEBUG_ERROR, "%a - Failed to find GOP globally. %r\n", __FUNCTION__, Status));
            return Status;
        }
    }
    UINT32 currentMode = mGraphicsOutput->Mode->Mode;
    if (mFrameBufferConfig != NULL) {
        // Check if we need to update it
        //DEBUG((DEBUG_ERROR, "%a - Check if we need to reconfigure it.\n", __FUNCTION__));
        if (mModeConfigredFor == currentMode) {
            //DEBUG((DEBUG_ERROR, "%a - No need to reconfigure.\n", __FUNCTION__));
            return EFI_SUCCESS;
        }
    }

    // Configure the frame buffer information
    Status =  FrameBufferBltConfigure(
        (UINT8*)((UINTN)mGraphicsOutput->Mode->FrameBufferBase),
        mGraphicsOutput->Mode->Info,
        mFrameBufferConfig,
        &mFrameBufferConfigSize
    );
    // If we're too small, then we need allocate and then try again
    if (Status == RETURN_BUFFER_TOO_SMALL) {
        //DEBUG((DEBUG_INFO, "[%a %a:%d] Too small, trying again\n", __FILE__, __FUNCTION__, __LINE__));
        FreeFrameBufferConfig();
        mFrameBufferConfig = AllocatePool(mFrameBufferConfigSize);
        return SetupFrameBufferConfig();
    }
    // if there's some other error, then free what we allocated if needed
    else if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_INFO, "[%a %a:%d] error: %r\n", __FILE__, __FUNCTION__, __LINE__, Status));
        FreeFrameBufferConfig();
    }
    else {
        // set the number of pixels we're set to
        mModeConfigredFor = currentMode;
    }
    return Status;
}

/**
Function to draw a data buffer onto the frame buffer
We assume the data is in 32 bit RGB reserved format

@param DrawDataBuffer    - The data to draw
@param TopLeftXInPixels  - The top-left X coordinate in pixels
@param TopLeftYInPixels  - The top-left Y coordinate in pixels
@param WidthInPixels     - Number of Columns in DrawDataBuffers
@param HeightInPixels    - Number of Rows in DrawDataBuffers
**/
EFI_STATUS
EFIAPI
MemDrawOnFrameBuffer(
  IN  UINT32  *DrawDataBuffer,
  IN  INT32  TopLeftXInPixels,
  IN  INT32  TopLeftYInPixels,
  IN  INT32  WidthInPixels,
  IN  INT32  HeightInPixels
){
    EFI_STATUS Status; 
    // Check if the frame buffer config is out of date in terms of mode
    Status = SetupFrameBufferConfig();
    if (mFrameBufferConfig == NULL) {
        Status = EFI_NOT_READY;
    }
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_INFO, "[%a %a:%d] we aren't setup to draw. Error: %r\n", __FILE__, __FUNCTION__, __LINE__, Status));
        return Status;
    }
    // Try to draw onto the frame buffer
    Status = FrameBufferBlt(
        mFrameBufferConfig,
        (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)DrawDataBuffer, // this is a 32 bit struct, packed tight so 4 bytes. Same format as UINT32 standard color
        EfiBltBufferToVideo,
        0,
        0,
        TopLeftXInPixels,
        TopLeftYInPixels,
        WidthInPixels,
        HeightInPixels,
        0
    );
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_INFO, "[%a %a:%d] can't draw. Error: %r\n", __FILE__, __FUNCTION__, __LINE__, Status));
    }
    return Status;
}

/**
Function to draw a single color onto the frame buffer
We assume the data is in 32 bit RGB reserved format

@param DrawDataBuffer    - The data to draw
@param TopLeftXInPixels  - The top-left X coordinate in pixels
@param TopLeftYInPixels  - The top-left Y coordinate in pixels
@param WidthInPixels     - Number of Columns in DrawDataBuffers
@param HeightInPixels    - Number of Rows in DrawDataBuffers
**/
EFI_STATUS
EFIAPI
MemFillOnFrameBuffer(
  IN  UINT32 Color,
  IN  INT32  TopLeftXInPixels,
  IN  INT32  TopLeftYInPixels,
  IN  INT32  WidthInPixels,
  IN  INT32  HeightInPixels
){
    EFI_STATUS Status; 
    // Check if the frame buffer config is out of date in terms of mode
    Status = SetupFrameBufferConfig();
    if (mFrameBufferConfig == NULL) {
        Status = EFI_NOT_READY;
    }
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_INFO, "[%a %a:%d] we aren't setup to draw. Error: %r\n", __FILE__, __FUNCTION__, __LINE__, Status));
        return Status;
    }
    // Try to draw onto the frame buffer
    Status = FrameBufferBlt(
        mFrameBufferConfig,
        (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)&Color,
        EfiBltVideoFill,
        0,
        0,
        TopLeftXInPixels,
        TopLeftYInPixels,
        WidthInPixels,
        HeightInPixels,
        0
    );
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_INFO, "[%a %a:%d] can't draw. Error: %r\n", __FILE__, __FUNCTION__, __LINE__, Status));
    }
    return Status;
}

/**
    The destructor frees the frame buffer config
    @param  ImageHandle   The firmware allocated handle for the EFI image.
    @param  SystemTable   A pointer to the EFI System Table.
    @retval EFI_SUCCESS   The destructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
FrameBufferMemDrawLibDestructor (
    )
{
    // Free the buffer if we no longer need it
    DEBUG((DEBUG_INFO, "[%a %a:%d] Tearing down the frame buffer config data\n", __FILE__, __FUNCTION__, __LINE__));
    FreeFrameBufferConfig();
    return EFI_SUCCESS;
}