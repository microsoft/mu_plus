
/** @file
Library used to draw directly onto the frame buffer

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FRAME_BUFFER_MEM_DRAW_LIB_H__
#define __FRAME_BUFFER_MEM_DRAW_LIB_H__

/**
Function to Display all Active Device States
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
  IN  UINT32 *DrawDataBuffer,
  IN  INT32  TopLeftXInPixels,
  IN  INT32  TopLeftYInPixels,
  IN  INT32  WidthInPixels,
  IN  INT32  HeightInPixels
);

/**
Function to draw a single color onto the frame buffer
We assume the data is in 32 bit RGB reserved format

@param Color             - The color to draw
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
);


#endif
