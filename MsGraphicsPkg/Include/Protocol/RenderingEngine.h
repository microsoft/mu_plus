/** @file
  Defines the Simple Rendering Engine (SRE) protocols.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
