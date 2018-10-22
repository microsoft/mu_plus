/** @file
  Defines the Simple Rendering Engine (SRE) protocols.

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

#ifndef _SRE_PROTOCOL_H_
#define _SRE_PROTOCOL_H_

#include <Protocol/SimpleWindowManager.h>


// Global ID for the Rendering Engine Protocol
//
#define MS_RENDERING_ENGINE_PROTOCOL_GUID                                           \
  {                                                                                 \
    0x7768969c, 0x1d94, 0x4d19, { 0xb0, 0xcf, 0x9f, 0x66, 0xcc, 0x59, 0x34, 0xb4 }  \
  }

typedef struct  _MS_RENDERING_ENGINE_PROTOCOL  MS_RENDERING_ENGINE_PROTOCOL;

/**
  Configures the mouse pointer image.

  @param  This                 Protocol instance pointer.
  @param  MouseBitmap          Pointer to the 32bpp mouse pointer bitmap.
  @param  Width                Mouse pointer width in pixels.
  @param  Height               Mouse pointer height in pixels.

  @retval EFI_SUCCESS          The mouse pointer was successfully set.

**/
typedef
EFI_STATUS
(EFIAPI *MS_SRE_SET_MOUSE_POINTER)(
    IN  MS_RENDERING_ENGINE_PROTOCOL    *This,
    IN  const UINT32                    *MouseBitmap,
    IN  UINT32                          Width,
    IN  UINT32                          Height,
    IN  UINT32                          Bpp
);


/**
  Shows or hides the mouse pointer.

  @param  This                 Protocol instance pointer.
  @param  ShowPointer          TRUE == Show, FALSE == Hide.

  @retval EFI_SUCCESS          The mouse pointer was successfully shown or hidden.

**/
typedef
EFI_STATUS
(EFIAPI *MS_SRE_SHOW_MOUSE_POINTER)(
    IN  MS_RENDERING_ENGINE_PROTOCOL    *This,
    IN  BOOLEAN                         ShowPointer
);


/**
  Moves the mouse pointer.

  @param  This                 Protocol instance pointer.
  @param  OrigX                Pointer origin X coordinate.
  @param  OrigY                Pointer origin Y coordinate.

  @retval EFI_SUCCESS          The mouse pointer was successfully moved.

**/
typedef
EFI_STATUS
(EFIAPI *MS_SRE_MOVE_MOUSE_POINTER)(
    IN  MS_RENDERING_ENGINE_PROTOCOL    *This,
    IN  UINT32                          OrigX,
    IN  UINT32                          OrigY
);


typedef
EFI_STATUS
(EFIAPI *MS_SRE_CREATE_SURFACE)(
    IN  MS_RENDERING_ENGINE_PROTOCOL    *This,
    IN  EFI_HANDLE                      ImageHandle,
    IN  SWM_RECT                        SurfaceFrame,
    OUT EFI_EVENT                       *PaintEvent
);

typedef
EFI_STATUS
(EFIAPI *MS_SRE_RESIZE_SURFACE)(
    IN  MS_RENDERING_ENGINE_PROTOCOL    *This,
    IN  EFI_HANDLE                      ImageHandle,
    IN  SWM_RECT                        *SurfaceFrame
);

typedef
EFI_STATUS
(EFIAPI *MS_SRE_ACTIVATE_SURFACE)(
    IN  MS_RENDERING_ENGINE_PROTOCOL    *This,
    IN  EFI_HANDLE                      ImageHandle,
    IN  BOOLEAN                         Active
);

typedef
EFI_STATUS
(EFIAPI *MS_SRE_DELETE_SURFACE)(
    IN  MS_RENDERING_ENGINE_PROTOCOL    *This,
    IN  EFI_HANDLE                      ImageHandle
);

typedef enum
{
    PAINT_BEGIN = 1,
    PAINT_END
} MS_SRE_SURFACE_MODE;


typedef
EFI_STATUS
(EFIAPI *MS_SRE_SET_MODE_SURFACE)(
    IN  MS_RENDERING_ENGINE_PROTOCOL    *This,
    IN  EFI_HANDLE                      ImageHandle,
    IN  MS_SRE_SURFACE_MODE             Mode
);


// SRE protocol structure
//
struct _MS_RENDERING_ENGINE_PROTOCOL
{
    // Mouse pointer related functions.
    //
    MS_SRE_SET_MOUSE_POINTER            SetMousePointer;
    MS_SRE_SHOW_MOUSE_POINTER           ShowMousePointer;
    MS_SRE_MOVE_MOUSE_POINTER           MoveMousePointer;

    // Rendering surface related functions.
    //
    MS_SRE_CREATE_SURFACE               CreateSurface;
    MS_SRE_RESIZE_SURFACE               ResizeSurface;
    MS_SRE_ACTIVATE_SURFACE             ActivateSurface;
    MS_SRE_DELETE_SURFACE               DeleteSurface;
    MS_SRE_SET_MODE_SURFACE             SetModeSurface;
};


extern EFI_GUID     gMsSREProtocolGuid;

#endif      // _SRE_PROTOCOL_H_.
