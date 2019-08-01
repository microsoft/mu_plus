/** @file

  Implements common structures and constants for the Surface graphics compositor.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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

