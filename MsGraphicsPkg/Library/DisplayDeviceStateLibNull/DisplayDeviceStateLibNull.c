/** @file
Null library used to display Device State on screen using color bars.

See DeviceStateLib for code that is related to getting and setting the device state.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>



/**
Function to Display all Active Device States

@param FrameBufferBase   - Address of point 0,0 in the frame buffer
@param PixelsPerScanLine - Number of pixels per scan line.
@param WidthInPixels     - Number of Columns in FrameBuffer
@param HeightInPixels    - Number of Rows in FrameBuffer
**/
VOID
EFIAPI
DisplayDeviceState(
IN  UINT8* FrameBufferBase,
IN  INT32  PixelsPerScanLine,
IN  INT32  WidthInPixels,
IN  INT32  HeightInPixels
)
{
	return;
}