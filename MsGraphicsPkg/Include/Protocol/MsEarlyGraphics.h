
// MsEarlyGraphicsProtocol

/** @file
  Header file for MsEarlyGraphics Protocol

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

#ifndef _MS_EARLY_DXE_GRAPHICS_H_
#define _MS_EARLY_DXE_GRAPHICS_H_

#include <Protocol/GraphicsOutput.h>

#define MS_EARLY_DXE_GRAPHICS_PROTOCOL_GUID                                         \
  {                                                                                 \
    /* 5b3db6e7-675a-4aa9-b637-7abcdda53ddb */                                      \
    0x5b3db6e7, 0x675a, 0x4aa9, { 0xb6, 0x37, 0x7a, 0xbc, 0xdd, 0xa5, 0x3d, 0xdb }  \
  }

#define MS_EARLY_GRAPHICS_PROTOCOL_SIGNATURE SIGNATURE_32 ('G', 'D', 'X', 'E')
#define MS_EARLY_GRAPHICS_VERSION 1

typedef struct _MS_EARLY_GRAPHICS_PROTOCOL MS_EARLY_GRAPHICS_PROTOCOL;

/**
    Performs a block copy (blit) to the early graphics framebuffer.

    @param[in]  This         Pointer to the instance of this protocol.
    @param[in]  Image        Image block of Pixels in frame buffer BPP @ WxH
    @param[in]  DestinationX The X coordinate of destination for the BltOperation.
    @param[in]  DestinationY The Y coordinate of destination for the BltOperation.
    @param[in]  Width        The width of a rectangle in the blt rectangle in pixels.
    @param[in]  Height       The height of a rectangle in the blt rectangle in pixels.

    @retval EFI_SUCCESS           BltBuffer was drawn to the graphics screen.
    @retval EFI_DEVICE_ERROR      The device had an error and could not complete the request.
    @retval EFI_NOT_SUPPORTED     The system

**/
typedef
EFI_STATUS
(EFIAPI *MS_EARLY_GRAPHICS_SIMPLE_BLT) (
    IN  MS_EARLY_GRAPHICS_PROTOCOL           *This,
    IN  CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Image,
    IN  UINT32                                DestinationX,
    IN  UINT32                                DestinationY,
    IN  UINT32                                Width,
    IN  UINT32                                Height);

/**
    SimpleFill
    Performs a fill of a block in the FrameBuffer.

    @param[in]  This         Pointer to the instance of this protocol.
    @param[in]  Image        Image block of Pixels in frame buffer BPP @ WxH
    @param[in]  DestinationX The X coordinate of destination for the BltOperation.
    @param[in]  DestinationY The Y coordinate of destination for the BltOperation.
    @param[in]  Width        The width of a rectangle in the blt rectangle in pixels.
    @param[in]  Height       The height of a rectangle in the blt rectangle in pixels.

    @retval EFI_SUCCESS           BltBuffer was drawn to the graphics screen.
    @retval EFI_DEVICE_ERROR      The device had an error and could not complete the request.
    @retval EFI_NOT_SUPPORTED     The system

**/
typedef
EFI_STATUS
(EFIAPI *MS_EARLY_GRAPHICS_SIMPLE_FILL)(
    IN  MS_EARLY_GRAPHICS_PROTOCOL    *This,
    IN  UINT32                         Color,
    IN  UINT32                         DestinationX,
    IN  UINT32                         DestinationY,
    IN  UINT32                         Width,
    IN  UINT32                         Height);

/**
 *  Print a line at the current row. There is no line wrapping,
 *  and \n and other special characters are not supported.

    @retval EFI_SUCCESS           String was written to display
*/
typedef
EFI_STATUS
(EFIAPI *MS_EARLY_GRAPHICS_PRINT_LINE) (
    IN  MS_EARLY_GRAPHICS_PROTOCOL      *This,
    IN  UINT32                          Row,
    IN  UINT32                          Column,
    IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ForegroundColor,
    IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   BackgroundColor,
    IN  CONST CHAR8                     *String);

/**
 *  Update FrameBufferBase

    Each call to the graphics adapter in DXE requires updating
    the display buffer address due to PCI Bus Enumeration

    @retval EFI_SUCCESS           String was written to display
*/
typedef
EFI_STATUS
(EFIAPI *MS_EARLY_GRAPHICS_UPDATE_FRAME_BUFFER_BASE)(
    IN  MS_EARLY_GRAPHICS_PROTOCOL    *This);

struct _MS_EARLY_GRAPHICS_PROTOCOL {
    UINT32                                     Signature;
    UINT32                                     Version;
    UINT32                                     Maxrows;
    UINT32                                     Maxcolumns;
    MS_EARLY_GRAPHICS_UPDATE_FRAME_BUFFER_BASE UpdateFrameBufferBase;
    MS_EARLY_GRAPHICS_SIMPLE_BLT               SimpleBlt;
    MS_EARLY_GRAPHICS_SIMPLE_FILL              SimpleFill;
    MS_EARLY_GRAPHICS_PRINT_LINE               PrintLn;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE          *Mode;
  };

extern EFI_GUID gMsEarlyGraphicsProtocolGuid;

#endif //  _MS_EARLY_DXE_GRAPHICS_H_


