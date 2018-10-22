/** @file

  Implements common structures and constants for the Surface graphics compositor.

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

#ifndef _RENDERING_ENGINE_INTERNAL_H_
#define _RENDERING_ENGINE_INTERNAL_H_

#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/RenderingEngine.h>
#include <Protocol/SimpleWindowManager.h>


// ****** Preprocessor constants ******
//


// ****** Function prototypes ******
//

/**
    Checks whether the specified controller has the GOP protocol installed on it.

    @param[in] This                 Pointer to the instance of this driver.
    @param[in] Controller           Controller handle to be checked.
    @param[in] RemainingDevicePath  Ignored.

    @retval EFI_SUCCESS             Found the controller we want to support.
    @retval EFI_UNSUPPORTED         Unsupported controller.

**/
EFI_STATUS
EFIAPI
SREDriverSupported (IN EFI_DRIVER_BINDING_PROTOCOL  *This,
                    IN EFI_HANDLE                   Controller,
                    IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath);


/**
    Connects exclusively to the controller then manufactures a new version of the GOP
    for other drivers to attach to.

    @param[in] This                 Pointer to the instance of this driver.
    @param[in] Controller           Handle of controller to be managed.
    @param[in] RemainingDevicePath  Ignored.

    @retval EFI_SUCCESS             Successfully connected to the controller.
    @retval EFI_UNSUPPORTED         Failed to connect.

**/
EFI_STATUS
EFIAPI
SREDriverStart (IN EFI_DRIVER_BINDING_PROTOCOL      *This,
                IN EFI_HANDLE                       Controller,
                IN EFI_DEVICE_PATH_PROTOCOL         *RemainingDevicePath);


/**
    Stop filtering GOP calls.

    @param[in] This                 Pointer to the instance of this driver.
    @param[in] Controller           Handle of controller being managed.
    @param[in] NumberOfChildren     Ignored.
    @param[in] ChildHandleBuffer    Ignored.

    @retval EFI_SUCCESS             Successfully removed child devices and stopped managing the controller.

**/
EFI_STATUS
EFIAPI
SREDriverStop (IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
               IN  EFI_HANDLE                      Controller,
               IN  UINTN                           NumberOfChildren,
               IN  EFI_HANDLE                      *ChildHandleBuffer);


// ****** Common data structures ******
//

typedef struct _SRE_SURFACE_LIST_tag
{
    BOOLEAN                             Active;             // TRUE == currently active and processing events.
    BOOLEAN                             PaintNotify;        // TRUE == client needs to be notified to paint their surface.
    BOOLEAN                             BlittingSurface;    // TRUE == currently blitting this surface.
    SWM_RECT                            FrameRect;          // Clients on-screen window frame rectangle (used for hit detection).
    UINT32                              FrameChecksum;      // Simple checksum from a sampling of surface frame pixels (used to detect surface changes from someone accessing the framebuffer directly).
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *pCaptureBuffer;    // Buffer for capturing screen contents underlying the client's window area.
    EFI_HANDLE                          ImageHandle;        // Image handle associated with the surface context.
    struct _SRE_SURFACE_LIST_tag        *PreviousActive;    // Previous ACTIVE Surface
    struct _SRE_SURFACE_LIST_tag        *pNext;             // Next surface in the list.
    struct _SRE_SURFACE_LIST_tag        *pPrev;             // Previous surface in the list.

} SRE_SURFACE_LIST;


// Rendering Engine context.
//
typedef struct _RENDERING_ENGINE_CONTEXT_tag_
{
    UINTN                           Signature;      // Context signature.

    // Mouse pointer-related members.
    //
    BOOLEAN                         ShowingMousePointer;
    UINT32                          *MousePointerBitmap;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *MousePointerBltBuffer;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *MousePointerBackBuffer;
    UINTN                           MousePointerWidth;
    UINTN                           MousePointerHeight;
    UINTN                           MousePointerBpp;
    UINTN                           MousePointerOrigX;
    UINTN                           MousePointerOrigY;

    // List of surfaces being managed.
    //
    SRE_SURFACE_LIST                *Surfaces;

    // Protocols.
    //
    EFI_GRAPHICS_OUTPUT_PROTOCOL    Gop;
    MS_RENDERING_ENGINE_PROTOCOL    SREProtocol;

} RENDERING_ENGINE_CONTEXT;


// ****** Preprocessor macros ******
//


// ****** External definitions ******
//
extern EFI_GUID gGopOverrideProtocolGuid;

#endif  // _RENDERING_ENGINE_INTERNAL_H_.

