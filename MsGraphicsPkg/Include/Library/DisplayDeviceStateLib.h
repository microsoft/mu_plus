/** @file
Library used to display Device States set by DeviceStateLib

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DEVICE_DISPLAY_STATE_LIB_H__
#define __DEVICE_DISPLAY_STATE_LIB_H__

#include <Protocol/GraphicsOutput.h>


/**
Function to Display all Active Device States

@param FrameBufferBase   - Address of point 0,0 in the frame buffer
@param PixelsPerScanLine - Number of pixels per scan line.
@param PixelFormat       - An enum that tells use what format the pixel are in
@param PixelFormatBitMap - A pointer to the exact layout of the pixels
@param WidthInPixels     - Number of Columns in FrameBuffer
@param HeightInPixels    - Number of Rows in FrameBuffer
**/
VOID
EFIAPI
DisplayDeviceState(
  IN  UINT8*  FrameBufferBase,
  IN  INT32  PixelsPerScanLine,
  IN EFI_GRAPHICS_PIXEL_FORMAT  PixelFormat,
  IN EFI_PIXEL_BITMASK* PixelFormatBitMap,
  IN  INT32  WidthInPixels,
  IN  INT32  HeightInPixels
);


#endif
