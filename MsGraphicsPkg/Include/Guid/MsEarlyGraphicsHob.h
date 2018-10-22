/** @file
  This file defines the Ms Early Graphics Hob

  Copyright (c) 2018,  Microsoft Corporation.

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

#ifndef __MS_EARLY_GRAPHICS_HOB_H__
#define __MS_EARLY_GRAPHICS_HOB_H__


extern EFI_GUID gMsEarlyGraphicsHobGuid;

typedef struct {
    ///------ The Mode Data
    ///
    /// The number of modes supported by QueryMode() and SetMode().
    ///
    UINT32                                 MaxMode;
    ///
    /// Current Mode of the graphics device. Valid mode numbers are 0 to MaxMode -1.
    ///
    UINT32                                 Mode;
    ///
    /// Pointer to read-only EFI_GRAPHICS_OUTPUT_MODE_INFORMATION data.
    ///
    // EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    ///
    /// Size of Info structure in bytes.
    ///
    UINT64                                 SizeOfInfo;
    ///
    /// Base address of graphics linear frame buffer.
    /// Offset zero in FrameBufferBase represents the upper left pixel of the display.
    ///
    EFI_PHYSICAL_ADDRESS                   FrameBufferBase;
    ///
    /// Amount of frame buffer needed to support the active mode as defined by
    /// PixelsPerScanLine xVerticalResolution x PixelElementSize.
    ///
    UINT64                                 FrameBufferSize;

    ///----- The Info Data
    ///
    /// The version of this data structure. A value of zero represents the
    /// EFI_GRAPHICS_OUTPUT_MODE_INFORMATION structure as defined in this specification.
    ///
    UINT32                     Version;
    ///
    /// The size of video screen in pixels in the X dimension.
    ///
    UINT32                     HorizontalResolution;
    ///
    /// The size of video screen in pixels in the Y dimension.
    ///
    UINT32                     VerticalResolution;
    ///
    /// Enumeration that defines the physical format of the pixel. A value of PixelBltOnly
    /// implies that a linear frame buffer is not available for this mode.
    ///
    UINT32                     PixelFormat;
    ///
    /// This bit-mask is only valid if PixelFormat is set to PixelPixelBitMask.
    /// A bit being set defines what bits are used for what purpose such as Red, Green, Blue, or Reserved.
    ///
    EFI_PIXEL_BITMASK          PixelInformation;
    ///
    /// Defines the number of pixel elements per video memory line.
    ///
    UINT32                     PixelsPerScanLine;

} MS_EARLY_GRAPHICS_HOB_DATA;

#endif  // __MS_EARLY_GRAPHICS_HOB_H__

