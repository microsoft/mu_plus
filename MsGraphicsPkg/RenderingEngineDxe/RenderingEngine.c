/** @file

  Simple Rendering Engine (SRE) implementation.

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

#include "RenderingEngineInternal.h"


// ****** Preprocessor constants ******
//
#define SURFACE_FRAME_SAMPLE_REFRESH_INTERVAL       (200 * 10 * 1000)       // Sample surface frames: 200ms in 100ns units
#define SURFACE_FRAME_SAMPLE_PIXEL_SPACING          50                      // Pixel spacing when checking whether the surface frame has been changed.


// ****** Global variables ******
//
EFI_HANDLE                      mImageHandle;
EFI_HANDLE                      mSREGopHandle;
EFI_GRAPHICS_OUTPUT_PROTOCOL    *mParentGop;
RENDERING_ENGINE_CONTEXT        mSRE;
EFI_EVENT                       mSampleSurfaceFrameTimerEvent;
EFI_GUID                       *mMsGopOverrideProtocolGuid;

// ****** Typedefs and structures ******
//

// Rendering Engine driver binding protocol support.
static
EFI_DRIVER_BINDING_PROTOCOL     mSREDriverBinding =
{
    SREDriverSupported,
    SREDriverStart,
    SREDriverStop,
    0x12,     // TODO
    NULL,
    NULL
};

// ****** Local function prototypes ******
//
static
EFI_STATUS
SREShowMousePointer (IN  MS_RENDERING_ENGINE_PROTOCOL    *This,
                     IN  BOOLEAN                         ShowPointer);

static
UINT32
CalculateSurfaceFrameChecksum (IN  SRE_SURFACE_LIST     *Surface);


VOID DisplaySurfaceList (VOID) {

SRE_SURFACE_LIST *Surface;

    Surface = mSRE.Surfaces;
    while (Surface != NULL)
    {
        DEBUG((DEBUG_INFO, "            - ImageHandle=0x%x, Active=%s, Surface=L[%d]:R[%d]:T[%d]:B[%d]\r\n", (UINTN)Surface->ImageHandle,
                                                                                                             (Surface->Active ? L"YES" : L"NO"),
                                                                                                             Surface->FrameRect.Left,
                                                                                                             Surface->FrameRect.Right,
                                                                                                             Surface->FrameRect.Top,
                                                                                                             Surface->FrameRect.Bottom));

        Surface = Surface->pNext;
    }
}

// ****** External definitions ******
//

// ****** Function declarations ******
//

static
EFI_STATUS
DrawMousePointer(IN BOOLEAN     ShowPointer,
                 IN UINTN       NewOrigX,
                 IN UINTN       NewOrigY)
{
    EFI_STATUS  Status  = EFI_SUCCESS;
    UINTN       Index;


    // Restore the location where the mouse pointer currently resides with the original screen content.
    //
    if (TRUE == mSRE.ShowingMousePointer)
    {
        mParentGop->Blt (mParentGop,
                         mSRE.MousePointerBackBuffer,
                         EfiBltBufferToVideo,
                         0,
                         0,
                         mSRE.MousePointerOrigX,
                         mSRE.MousePointerOrigY,
                         mSRE.MousePointerWidth,
                         mSRE.MousePointerHeight,
                         0
                        );
    }

    // If we don't need to show the mouse pointer, we're done.
    //
    if (FALSE == ShowPointer)
    {
        goto Exit;
    }

    // Otherwise capture screen contents at the new location.
    //
    mParentGop->Blt(mParentGop,
                    mSRE.MousePointerBackBuffer,
                    EfiBltVideoToBltBuffer,
                    NewOrigX,
                    NewOrigY,
                    0,
                    0,
                    mSRE.MousePointerWidth,
                    mSRE.MousePointerHeight,
                    0
                   );

    // Proceed to draw the mouse pointer at the new location.
    //
    mParentGop->Blt(mParentGop,
                    mSRE.MousePointerBltBuffer,
                    EfiBltVideoToBltBuffer,
                    NewOrigX,
                    NewOrigY,
                    0,
                    0,
                    mSRE.MousePointerWidth,
                    mSRE.MousePointerHeight,
                    0
                   );

    // Logically "OR" the mouse pointer into the blt buffer
    //
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pPointer = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)mSRE.MousePointerBitmap;

    for (Index = 0 ; Index < (mSRE.MousePointerWidth * mSRE.MousePointerHeight) ; Index++)
    {
        // If the mouse pointer pixel isn't black, copy it to the blt buffer.
        //
        if (mSRE.MousePointerBitmap[Index] != 0)
        {
            mSRE.MousePointerBltBuffer[Index] = pPointer[Index];
        }
    }

    // Blt the result to the screen
    //
    mParentGop->Blt(mParentGop,
                    mSRE.MousePointerBltBuffer,
                    EfiBltBufferToVideo,
                    0,
                    0,
                    NewOrigX,
                    NewOrigY,
                    mSRE.MousePointerWidth,
                    mSRE.MousePointerHeight,
                    0
                   );

Exit:

    return Status;
}


static
BOOLEAN
ValueInRange(IN UINT32 Value,
             IN UINT32 Min,
             IN UINT32 Max)
{
    return (Value >= Min) && (Value <= Max);
}


static
BOOLEAN
RectsOverlap (IN SWM_RECT   A,
              IN SWM_RECT   B)
{
    BOOLEAN XOverlap = ValueInRange(A.Left, B.Left, B.Right) ||
                       ValueInRange(B.Left, A.Left, A.Right);

    BOOLEAN YOverlap = ValueInRange(A.Top, B.Top, B.Bottom) ||
                       ValueInRange(B.Top, A.Top, A.Bottom);

    return XOverlap && YOverlap;
}


static
EFI_STATUS
SREBlt (IN  EFI_GRAPHICS_OUTPUT_PROTOCOL            *This,
        IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL           *BltBuffer,
        IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION       BltOperation,
        IN  UINTN                                   SourceX,
        IN  UINTN                                   SourceY,
        IN  UINTN                                   DestinationX,
        IN  UINTN                                   DestinationY,
        IN  UINTN                                   Width,
        IN  UINTN                                   Height,
        IN  UINTN                                   Delta)
{
    EFI_STATUS          Status      = EFI_SUCCESS;
    EFI_TPL             PreviousTPL = 0;
    SRE_SURFACE_LIST    *Surface;
    SWM_RECT            BltRect;
    SWM_RECT            PointerRect;
    UINT32              FrameWidth, FrameHeight;
    BOOLEAN             MousePointerState = mSRE.ShowingMousePointer;


    // Current blit operation bounding rectangle.
    //
    BltRect.Left        = (UINT32)(DestinationX);
    BltRect.Top         = (UINT32)(DestinationY);
    BltRect.Right       = (UINT32)(DestinationX + Width  - 1);
    BltRect.Bottom      = (UINT32)(DestinationY + Height - 1);

    // Raise the TPL to avoid interrupting rendering and framebuffer capture.
    //
    PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Current mouse pointer bounding rectangle.
    //
    PointerRect.Left    = (UINT32)(mSRE.MousePointerOrigX);
    PointerRect.Top     = (UINT32)(mSRE.MousePointerOrigY);
    PointerRect.Right   = (UINT32)(mSRE.MousePointerOrigX + mSRE.MousePointerWidth  - 1);
    PointerRect.Bottom  = (UINT32)(mSRE.MousePointerOrigY + mSRE.MousePointerHeight - 1);

    // If the blit intersects with the mouse, we need to temporarily hide the mouse pointer.
    //
    if ((TRUE == mSRE.ShowingMousePointer) && (TRUE == RectsOverlap(PointerRect, BltRect)))
    {
        SREShowMousePointer (&mSRE.SREProtocol,
                             FALSE
                            );
    }

    // First see if the blit intersects with one of the active surfaces.  If it does, restore surface back buffer contents first.  We ignore
    // a surface if the blitting flag is set so that drawing to a surface doesn't trigger a self-refresh.
    //
    Surface = mSRE.Surfaces;
    while ((NULL != Surface) && (EfiBltVideoToBltBuffer != BltOperation))
    {
        if (TRUE  == Surface->Active &&
            FALSE == Surface->BlittingSurface &&
            (TRUE  == RectsOverlap(Surface->FrameRect, BltRect) || Surface->FrameChecksum != CalculateSurfaceFrameChecksum (Surface)))
        {
            FrameWidth  = (Surface->FrameRect.Right - Surface->FrameRect.Left + 1);
            FrameHeight = (Surface->FrameRect.Bottom - Surface->FrameRect.Top + 1);

            // Remember that we need to notify the client to redraw.
            //
            Surface->PaintNotify = TRUE;

            // If restoring the screen under the surface intersects with the mouse, we need to temporarily hide the mouse pointer.
            //
            if ((TRUE == mSRE.ShowingMousePointer) && (TRUE == RectsOverlap(PointerRect, Surface->FrameRect)))
            {
                SREShowMousePointer (&mSRE.SREProtocol,
                                     FALSE
                                    );
            }

            // Restore the contents to the framebuffer.
            //
            mParentGop->Blt(mParentGop,
                            Surface->pCaptureBuffer,
                            EfiBltBufferToVideo,
                            0,
                            0,
                            Surface->FrameRect.Left,
                            Surface->FrameRect.Top,
                            FrameWidth,
                            FrameHeight,
                            0
                           );

            // Re-calculate the surface frame checksum.
            //
            Surface->FrameChecksum = CalculateSurfaceFrameChecksum (Surface);
        }

        Surface = Surface->pNext;
    }

    // Perform the caller's requested blit operation.
    //
    mParentGop->Blt (mParentGop,
                     BltBuffer,
                     BltOperation,
                     SourceX,
                     SourceY,
                     DestinationX,
                     DestinationY,
                     Width,
                     Height,
                     Delta
                    );


    // Now that we've finished the caller's requested blitting, recapture the contents underlying any active client surface that intersected with the blit rectangle.
    // Note that we ignore video to blit buffer operations since these don't affect the framebuffer.
    //
    Surface = mSRE.Surfaces;
    while ((NULL != Surface) && (EfiBltVideoToBltBuffer != BltOperation))
    {
        // Capture screen contents for any surfaces that intersected with the blit operation.  Again, we can ignore any surfaces which are marked
        // with the blitting flag in order to avoid triggering a refresh.
        //
        if (TRUE  == Surface->Active)
        {
            if (FALSE == Surface->BlittingSurface &&
                TRUE  == RectsOverlap(Surface->FrameRect, BltRect))
            {
                FrameWidth  = (Surface->FrameRect.Right - Surface->FrameRect.Left + 1);
                FrameHeight = (Surface->FrameRect.Bottom - Surface->FrameRect.Top + 1);

                // Save the contents of the framebuffer to this capture buffer.
                //
                mParentGop->Blt(mParentGop,
                                Surface->pCaptureBuffer,
                                EfiBltVideoToBltBuffer,
                                Surface->FrameRect.Left,
                                Surface->FrameRect.Top,
                                0,
                                0,
                                FrameWidth,
                                FrameHeight,
                                FrameWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                               );
            }

            // Re-calculate the surface frame checksum.
            //
            Surface->FrameChecksum = CalculateSurfaceFrameChecksum (Surface);
        }

        Surface = Surface->pNext;
    }

    // If the mouse pointer should be shown but it's not currently, enable it here.
    //
    if ((TRUE == MousePointerState) && (FALSE == mSRE.ShowingMousePointer))
    {
        SREShowMousePointer (&mSRE.SREProtocol,
                             TRUE
                            );
    }

    // Restore the TPL.
    //
    if (PreviousTPL)
    {
        gBS->RestoreTPL (PreviousTPL);
    }


    return Status;
}


static
EFI_STATUS
SREQueryMode (IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
              IN  UINT32                                ModeNumber,
              OUT UINTN                                 *SizeOfInfo,
              OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info)
{

    return mParentGop->QueryMode (mParentGop,
                                  ModeNumber,
                                  SizeOfInfo,
                                  Info
                                 );
}


static
EFI_STATUS
SRESetMode (IN  EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
            IN  UINT32                       ModeNumber)
{
    EFI_STATUS  Status;
    EFI_TPL     PreviousTPL;

     // Raise the TPL to avoid getting interrupted while we access shared data structures.
    //
    PreviousTPL = gBS->RaiseTPL (TPL_CALLBACK);

    Status = mParentGop->SetMode (mParentGop,
                                  ModeNumber
                                  );
    // Restore the TPL.
    //
    gBS->RestoreTPL (PreviousTPL);

    return Status;
}


static
EFI_STATUS
SRESetMousePointer (IN  MS_RENDERING_ENGINE_PROTOCOL    *This,
                    IN  const UINT32                    *MouseBitmap,
                    IN  UINT32                          Width,
                    IN  UINT32                          Height,
                    IN  UINT32                          Bpp)
{
    EFI_STATUS  Status      = EFI_SUCCESS;
    UINTN       BuffSize    = 0;
    EFI_TPL     PreviousTPL = 0;


    // Validate function parameters.
    //
    if (0 != (Bpp % 8))
    {
        DEBUG ((DEBUG_ERROR, "ERROR [SRE]: SRESetMousePointer: pointer bitmap bpp should be an integral number of bytes (Bpp=%d).\r\n", Bpp));
        Status = EFI_INVALID_PARAMETER;
        goto Exit;
    }

    // Raise the TPL to avoid anyone interrupting rendering and framebuffer capture.
    //
    PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Free the existing mouse pointer buffers if they exist.
    //
    if (NULL != mSRE.MousePointerBitmap)
    {
        FreePool(mSRE.MousePointerBitmap);
    }

    if (NULL != mSRE.MousePointerBltBuffer)
    {
        FreePool(mSRE.MousePointerBltBuffer);
    }

    if (NULL != mSRE.MousePointerBackBuffer)
    {
        FreePool(mSRE.MousePointerBackBuffer);
    }

    mSRE.MousePointerWidth  = 0;
    mSRE.MousePointerHeight = 0;
    mSRE.MousePointerBpp    = 0;

    // Allocate a buffer to hold a copy of the mouse pointer bitmap.
    //
    BuffSize = (Width * Height * (Bpp / 8));
    mSRE.MousePointerBitmap = AllocateZeroPool(BuffSize);

    ASSERT(NULL != mSRE.MousePointerBitmap);
    if (NULL == mSRE.MousePointerBitmap)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Save the mouse pointer information being provided.
    //
    mSRE.MousePointerWidth  = Width;
    mSRE.MousePointerHeight = Height;
    mSRE.MousePointerBpp    = Bpp;

    CopyMem(mSRE.MousePointerBitmap, MouseBitmap, BuffSize);

    // Allocate a working buffer for blitting the mouse pointer.
    //
    BuffSize = (Width * Height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    mSRE.MousePointerBltBuffer = AllocateZeroPool(BuffSize);

    ASSERT(NULL != mSRE.MousePointerBltBuffer);
    if (NULL == mSRE.MousePointerBltBuffer)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Allocate a backing buffer for the mouse pointer (used to restore underlying screen contents).
    //
    mSRE.MousePointerBackBuffer = AllocateZeroPool(BuffSize);

    ASSERT(NULL != mSRE.MousePointerBackBuffer);
    if (NULL == mSRE.MousePointerBackBuffer)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

Exit:

    // Restore the TPL.
    //
    if (PreviousTPL)
    {
        gBS->RestoreTPL (PreviousTPL);
    }

    return Status;
}


static
EFI_STATUS
SREShowMousePointer (IN  MS_RENDERING_ENGINE_PROTOCOL    *This,
                     IN  BOOLEAN                         ShowPointer)
{
    EFI_STATUS  Status = EFI_SUCCESS;


    // Refresh the mouse pointer region (to hide or show it) if the state changed.
    //
    if (ShowPointer != mSRE.ShowingMousePointer)
    {
        Status = DrawMousePointer(ShowPointer,
                                  mSRE.MousePointerOrigX,
                                  mSRE.MousePointerOrigY
                                 );

        if (EFI_SUCCESS == Status)
        {
            // Capture the show-hide state.
            //
            mSRE.ShowingMousePointer = ShowPointer;
        }
    }

    return Status;
}


static
EFI_STATUS
SREMoveMousePointer (IN  MS_RENDERING_ENGINE_PROTOCOL    *This,
                     IN  UINT32                          OrigX,
                     IN  UINT32                          OrigY)
{
    EFI_STATUS  Status = EFI_SUCCESS;
    EFI_TPL     PreviousTPL = 0;


    // If the mouse pointer isn't being displayed, simply exit.  We won't even try to capture
    // the updated location.
    //
    if (FALSE == mSRE.ShowingMousePointer)
    {
        goto Exit;
    }

    // Raise the TPL to avoid anyone interrupting rendering and framebuffer capture.
    //
    PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Refresh the mouse pointer position if the location changed.
    //
    if (OrigX != mSRE.MousePointerOrigX || OrigY != mSRE.MousePointerOrigY)
    {
        Status = DrawMousePointer(mSRE.ShowingMousePointer,
                                  OrigX,
                                  OrigY
                                 );
    }

    // Capture the new position.
    //
    mSRE.MousePointerOrigX = OrigX;
    mSRE.MousePointerOrigY = OrigY;

Exit:

    // Restore the TPL.
    //
    if (PreviousTPL)
    {
        gBS->RestoreTPL (PreviousTPL);
    }

    return Status;
}


VOID
EFIAPI
CheckForPendingPaintRequest (IN  EFI_EVENT    Event,
                             IN  VOID         *Context)
{
    SRE_SURFACE_LIST    *Surface    = (SRE_SURFACE_LIST *)Context;

    if (TRUE == Surface->PaintNotify)
    {
        Surface->PaintNotify = FALSE;
        gBS->SignalEvent (Event);
    }
}


static
UINT32
CalculateSurfaceFrameChecksum (IN  SRE_SURFACE_LIST     *Surface)
{
    UINT32                          Width           = (Surface->FrameRect.Right - Surface->FrameRect.Left + 1);
    UINT32                          Height          = (Surface->FrameRect.Bottom - Surface->FrameRect.Top + 1);
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *SurfaceOrigin;
    UINT32                          Offset;
    UINT32                          Checksum        = 0;


    // Sample top edge.
    //
    SurfaceOrigin  = ((EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)mParentGop->Mode->FrameBufferBase + (Surface->FrameRect.Top * mParentGop->Mode->Info->PixelsPerScanLine) + Surface->FrameRect.Left);
    for (Offset = 0 ; Offset < Width ; Offset += SURFACE_FRAME_SAMPLE_PIXEL_SPACING)
    {
        Checksum += *(UINT32 *)(SurfaceOrigin + Offset);
    }

    // Sample left and right edges and midpoint bisecting line.
    //
    for (Offset = 0 ; Offset < Height ; Offset += SURFACE_FRAME_SAMPLE_PIXEL_SPACING)
    {
        // Left edge.
        Checksum += *(UINT32 *)(SurfaceOrigin + (Offset * mParentGop->Mode->Info->PixelsPerScanLine));

        // Midpoint bisecting line.
        Checksum += *(UINT32 *)(SurfaceOrigin + (Offset * mParentGop->Mode->Info->PixelsPerScanLine) + ((Width - 1) /2));

        // Right edge.
        Checksum += *(UINT32 *)(SurfaceOrigin + (Offset * mParentGop->Mode->Info->PixelsPerScanLine) + (Width - 1));
    }

    // Sample bottom edge.
    //
    SurfaceOrigin  = ((EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)mParentGop->Mode->FrameBufferBase + ((Surface->FrameRect.Bottom - 1) * mParentGop->Mode->Info->PixelsPerScanLine) + Surface->FrameRect.Left);
    for (Offset = 0 ; Offset < Width ; Offset += SURFACE_FRAME_SAMPLE_PIXEL_SPACING)
    {
        Checksum += *(UINT32 *)(SurfaceOrigin + Offset);
    }


    return Checksum;
}


VOID
EFIAPI
SampleSurfaceFrameTimerCallback (IN EFI_EVENT  Event,
                                 IN VOID       *Context)
{
    SRE_SURFACE_LIST    *Surface;


    // Raise the TPL to avoid getting interrupted while we access shared data structures.
    //
    EFI_TPL  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Check whether any active surface's frame has been altered.
    //
    Surface = mSRE.Surfaces;
    while (NULL != Surface)
    {
        if (TRUE == Surface->Active && Surface->FrameChecksum != CalculateSurfaceFrameChecksum (Surface))
        {
            Surface->PaintNotify = TRUE;
        }
        Surface = Surface->pNext;
    }

    // Restore the TPL.
    //
    gBS->RestoreTPL (PreviousTPL);
}


static
EFI_STATUS
SRECreateSurface (IN  MS_RENDERING_ENGINE_PROTOCOL    *This,
                  IN  EFI_HANDLE                      ImageHandle,
                  IN  SWM_RECT                        FrameRect,
                  OUT EFI_EVENT                       *PaintEvent)
{
    EFI_STATUS          Status      = EFI_SUCCESS;
    SRE_SURFACE_LIST    *Surface;
    SRE_SURFACE_LIST    *pTemp;
    EFI_TPL             PreviousTPL = 0;


    DEBUG((DEBUG_INFO, "INFO [SRE]: Creating a new surface (ImageHandle=0x%x).\r\n", (UINTN)ImageHandle));

    // Raise the TPL to avoid getting interrupted while we access shared data structures.
    //
    PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Check whether this image handle has already been used to register a surface.
    //
    Surface = mSRE.Surfaces;
    while (NULL != Surface)
    {
        if (Surface->ImageHandle == ImageHandle)
        {
            Status = EFI_ALREADY_STARTED;
            goto Exit;
        }
        Surface = Surface->pNext;
    }

    // Allocate a new node for this surface.
    //
    pTemp = (SRE_SURFACE_LIST *) AllocateZeroPool(sizeof(SRE_SURFACE_LIST));

    ASSERT (NULL != pTemp);
    if (NULL == pTemp)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Capture surface information.
    //
    pTemp->pNext         = mSRE.Surfaces;
    pTemp->pPrev         = NULL;
    pTemp->ImageHandle   = ImageHandle;
    pTemp->Active        = FALSE;
    pTemp->PaintNotify   = FALSE;
    pTemp->PreviousActive = NULL;

    CopyMem(&pTemp->FrameRect, &FrameRect, sizeof(SWM_RECT));

    // Allocate storage for the backing buffer.
    //
    UINT32 Width  = (FrameRect.Right - FrameRect.Left + 1);
    UINT32 Height = (FrameRect.Bottom - FrameRect.Top + 1);

    // Allocate a capture buffer for the surface.
    //
    pTemp->pCaptureBuffer = AllocatePool (Width * Height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

    ASSERT (NULL != pTemp->pCaptureBuffer);
    if (NULL == pTemp->pCaptureBuffer)
    {
        FreePool(pTemp);
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Create a custom paint event for this client with EVT_NOTIFY_WAIT so we're called with the client's
    // context whenever the client waits on it.
    //
    Status = gBS->CreateEvent (EVT_NOTIFY_WAIT,
                               TPL_NOTIFY,
                               CheckForPendingPaintRequest,
                               (VOID *)pTemp,
                               PaintEvent
                              );

    if (EFI_ERROR (Status))
    {
        DEBUG ((DEBUG_ERROR, "ERROR [SRE]: Failed to create event for notifying client of a surface paint request (%r).\r\n", Status));
        FreePool(pTemp->pCaptureBuffer);
        FreePool(pTemp);
        goto Exit;
    }

    // Attach it to the list
    //
    if (mSRE.Surfaces != NULL)
    {
        mSRE.Surfaces->pPrev = pTemp;
    }
    mSRE.Surfaces           = pTemp;

Exit:

    // Restore the TPL
    //
    if (PreviousTPL)
    {
        gBS->RestoreTPL (PreviousTPL);
    }

    // Display client list for debugging purposes.
    //
    DEBUG((DEBUG_INFO, "INFO [SRE]: Surface list:\r\n"));
    DisplaySurfaceList ();

    return Status;
}


static
EFI_STATUS
SREResizeSurface (IN  MS_RENDERING_ENGINE_PROTOCOL    *This,
                  IN  EFI_HANDLE                      ImageHandle,
                  IN  SWM_RECT                        *FrameRect)
{
    EFI_STATUS          Status              = EFI_SUCCESS;
    SRE_SURFACE_LIST    *Surface;
    UINT32              Width, Height;
    BOOLEAN             MousePointerState   = mSRE.ShowingMousePointer;


    DEBUG ((DEBUG_INFO, "INFO [SRE]: Resizing surface (ImageHandle=0x%x).\r\n", (UINTN)ImageHandle));

    // Raise the TPL to avoid getting interrupted while we access shared data structures.
    //
    EFI_TPL  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Hide the mouse pointer.
    //
    if (TRUE == MousePointerState)
    {
        SREShowMousePointer (&mSRE.SREProtocol,
                             FALSE
                            );
    }

    // Set active state for the specified surface (note this assumes we'll find a match for ImageHandle).
    //
    Surface = mSRE.Surfaces;
    while (NULL != Surface)
    {
        if (Surface->ImageHandle == ImageHandle)
        {
            // Check whether a framebuffer already exists.  If so, free it.
            //
            if (NULL != Surface->pCaptureBuffer)
            {
                // If the surface is active, restore the backing buffer before resizing.
                //
                if (TRUE == Surface->Active)
                {
                    Width  = (Surface->FrameRect.Right - Surface->FrameRect.Left + 1);
                    Height = (Surface->FrameRect.Bottom - Surface->FrameRect.Top + 1);

                    // Restore the contents to the framebuffer.
                    //
                    mParentGop->Blt(mParentGop,
                                    Surface->pCaptureBuffer,
                                    EfiBltBufferToVideo,
                                    0,
                                    0,
                                    Surface->FrameRect.Left,
                                    Surface->FrameRect.Top,
                                    Width,
                                    Height,
                                    0
                                   );
                }

                FreePool (Surface->pCaptureBuffer);
                Surface->pCaptureBuffer = NULL;
            }

            // Capture the new frame rectangle.
            //
            CopyMem(&Surface->FrameRect, FrameRect, sizeof(SWM_RECT));

            // Allocate storage for the backing buffer.
            //
            Width  = (FrameRect->Right - FrameRect->Left + 1);
            Height = (FrameRect->Bottom - FrameRect->Top + 1);
            Surface->pCaptureBuffer = AllocatePool (Width * Height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

            ASSERT (NULL != Surface->pCaptureBuffer);
            if (NULL == Surface->pCaptureBuffer)
            {
                Status = EFI_OUT_OF_RESOURCES;
                break;
            }

            // If the surface is active, cature screen contents to the new buffer.
            //
            if (TRUE == Surface->Active)
            {
                // Save the contents of the framebuffer to this capture buffer.
                //
                mParentGop->Blt(mParentGop,
                                Surface->pCaptureBuffer,
                                EfiBltVideoToBltBuffer,
                                Surface->FrameRect.Left,
                                Surface->FrameRect.Top,
                                0,
                                0,
                                Width,
                                Height,
                                0
                               );

                // Compute the surface frame checksum.
                //
                Surface->FrameChecksum = CalculateSurfaceFrameChecksum (Surface);
            }
        }

        Surface = Surface->pNext;
    }

    // Restore the mouse pointer if it was displayed before.
    //
    if (TRUE == MousePointerState)
    {
        SREShowMousePointer (&mSRE.SREProtocol,
                             TRUE
                            );
    }

    // Restore the TPL.
    //
    gBS->RestoreTPL (PreviousTPL);


    return Status;
}


static
EFI_STATUS
SREActivateSurface (IN  MS_RENDERING_ENGINE_PROTOCOL    *This,
                    IN  EFI_HANDLE                      ImageHandle,
                    IN  BOOLEAN                         MakeActive)
{
    EFI_STATUS          Status              = EFI_NOT_FOUND;
    SRE_SURFACE_LIST    *Surface;
    SRE_SURFACE_LIST    *ActiveSurface;
    UINT32              FrameWidth, FrameHeight;
    BOOLEAN             MousePointerState   = mSRE.ShowingMousePointer;


    DEBUG ((DEBUG_INFO, "INFO [SRE]: Setting surface active (ImageHandle=0x%x, MakeActive=%s).\r\n", (UINTN)ImageHandle, (TRUE == MakeActive ? L"TRUE" : L"FALSE")));

    // Raise the TPL to avoid getting interrupted while we access shared data structures.
    //
    EFI_TPL  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Hide the mouse pointer.
    //
    if (TRUE == MousePointerState)
    {
        SREShowMousePointer (&mSRE.SREProtocol,
                             FALSE
                            );
    }

    // Set active state for the specified surface (note this assumes we'll find a match for ImageHandle).
    //
    ActiveSurface = NULL;
    Surface = mSRE.Surfaces;

    // Find previous Active Surface
    while (NULL != Surface)
    {
        if (Surface->Active)
        {
            ActiveSurface = Surface;
            break;
        }
        Surface = Surface->pNext;
    }

    Surface = mSRE.Surfaces;
    while (NULL != Surface)
    {
        if (Surface->ImageHandle == ImageHandle)
        {
            Surface->Active    = MakeActive;

            FrameWidth  = (Surface->FrameRect.Right - Surface->FrameRect.Left + 1);
            FrameHeight = (Surface->FrameRect.Bottom - Surface->FrameRect.Top + 1);

            if (TRUE == MakeActive)
            {
                Surface->PreviousActive = ActiveSurface;
                if (ActiveSurface != NULL)
                {
                    ActiveSurface->Active = FALSE;
                }

                // Save the contents of the framebuffer to this capture buffer.
                //
                mParentGop->Blt(mParentGop,
                                Surface->pCaptureBuffer,
                                EfiBltVideoToBltBuffer,
                                Surface->FrameRect.Left,
                                Surface->FrameRect.Top,
                                0,
                                0,
                                FrameWidth,
                                FrameHeight,
                                0
                               );
            }
            else
            {
                if (Surface->PreviousActive != NULL)
                {
                    Surface->PreviousActive->Active = TRUE;
                    Surface->PreviousActive = NULL;
                }
                // Restore the contents to the framebuffer.
                //
                mParentGop->Blt(mParentGop,
                                Surface->pCaptureBuffer,
                                EfiBltBufferToVideo,
                                0,
                                0,
                                Surface->FrameRect.Left,
                                Surface->FrameRect.Top,
                                FrameWidth,
                                FrameHeight,
                                0
                               );
            }

            // Compute the surface frame checksum.
            //
            Surface->FrameChecksum = CalculateSurfaceFrameChecksum (Surface);

            Status = EFI_SUCCESS;
            break;
        }

        Surface = Surface->pNext;
    }

    // Restore the mouse pointer if it was displayed before.
    //
    if (TRUE == MousePointerState)
    {
        SREShowMousePointer (&mSRE.SREProtocol,
                             TRUE
                            );
    }

    // Restore the TPL.
    //
    gBS->RestoreTPL (PreviousTPL);

    DEBUG((DEBUG_INFO,"INFO [SRE]: Activate surface h=%p, Active=%d\n",ImageHandle, MakeActive));
    DisplaySurfaceList();

    return Status;
}


static
EFI_STATUS
SREDeleteSurface (IN  MS_RENDERING_ENGINE_PROTOCOL    *This,
                  IN  EFI_HANDLE                      ImageHandle)
{
    EFI_STATUS          Status      = EFI_SUCCESS;
    SRE_SURFACE_LIST    *Surface;


    DEBUG((DEBUG_INFO, "INFO [SRE]: Deleting surface (ImageHandle=0x%x).\r\n", (UINTN)ImageHandle));

    // Raise the TPL to avoid getting interrupted while we access shared data structures.
    //
    EFI_TPL  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Search for the specified image handle's client (note this assumes we'll find a match for ImageHandle).
    //
    Surface = mSRE.Surfaces;
    while (NULL != Surface)
    {
        if (Surface->ImageHandle == ImageHandle)
        {
            // If a screen capture buffer for the client's surface area exists, free it.
            //
            if (NULL != Surface->pCaptureBuffer)
            {
                FreePool(Surface->pCaptureBuffer);
                Surface->pCaptureBuffer = NULL;
            }

            // Unlink the current client node and free it.
            //
            if (NULL == Surface->pPrev)
            {
                mSRE.Surfaces = Surface->pNext;
                if (NULL != mSRE.Surfaces)
                {
                    mSRE.Surfaces->pPrev = NULL;
                }
            }
            else
            {
                Surface->pPrev->pNext = Surface->pNext;
                if (NULL != Surface->pNext)
                {
                    Surface->pNext->pPrev = Surface->pPrev;
                }
            }

            {
              // When deleting a Surface, clean up any other Surfaces that
              // saved a "Previous Active" pointer to this Surface.
              SRE_SURFACE_LIST    *Surface2;
              Surface2 = mSRE.Surfaces;
              while (NULL != Surface2) {
                if (Surface2->PreviousActive == Surface) {
                  Surface2->PreviousActive = Surface->PreviousActive;
                }
                Surface2 = Surface2->pNext;
              }
            }

            FreePool(Surface);

            break;
        }

        Surface = Surface->pNext;
    }

    // Restore the TPL.
    //
    gBS->RestoreTPL (PreviousTPL);

    // Print out a debug message if we didn't remove anything.
    //
    if (NULL == Surface)
    {
        DEBUG ((DEBUG_WARN, "WARN [SRE]: Failed to delete surface registered by image handle %d.\r\n", (UINTN)ImageHandle));
    }


    // Display surface list for debugging purposes.
    //
    DEBUG((DEBUG_INFO, "INFO [SRE]: Surface list:\r\n"));
    DisplaySurfaceList ();

    return Status;
}


static
EFI_STATUS
SRESetModeSurface (IN  MS_RENDERING_ENGINE_PROTOCOL     *This,
                   IN  EFI_HANDLE                       ImageHandle,
                   IN  MS_SRE_SURFACE_MODE              Mode)
{
    EFI_STATUS          Status      = EFI_SUCCESS;
    SRE_SURFACE_LIST    *Surface;


    // Raise the TPL to avoid getting interrupted while we access shared data structures.
    //
    EFI_TPL  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Search for the specified image handle's client (note this assumes we'll find a match for ImageHandle).
    //
    Surface = mSRE.Surfaces;
    while (NULL != Surface)
    {
        if (Surface->ImageHandle == ImageHandle)
        {
            switch (Mode)
            {
            case PAINT_BEGIN:
                Surface->BlittingSurface = TRUE;
                break;
            case PAINT_END:
                Surface->BlittingSurface = FALSE;
                break;
            default:
                DEBUG((DEBUG_ERROR, "ERROR [SRE]: Unrecognized surface mode (Mode=%d).\r\n", (UINT32)Mode));
                Status = EFI_INVALID_PARAMETER;
                ASSERT (FALSE);
                break;
            }

            break;
        }

        Surface = Surface->pNext;
    }

    // Restore the TPL.
    //
    gBS->RestoreTPL (PreviousTPL);

    return Status;
}


static EFI_STATUS
InitializeRenderingEngine (VOID)
{
    EFI_STATUS  Status  = EFI_SUCCESS;


    DEBUG ((DEBUG_INFO, "INFO [SRE]: Initializing the Rendering Engine.\r\n"));

    // Configure initial Rendering Engine context.
    //
    mSRE.ShowingMousePointer    = FALSE;
    mSRE.MousePointerOrigX      = (mParentGop->Mode->Info->HorizontalResolution / 2);     // Default X position (may be adjusted later for display mode switch).
    mSRE.MousePointerOrigY      = (mParentGop->Mode->Info->VerticalResolution   / 2);     // Default Y position (may be adjusted later for display mode switch).

    // Install our own GOP handlers.
    //
    mSRE.Gop.Blt                        = SREBlt;
    mSRE.Gop.QueryMode                  = SREQueryMode;
    mSRE.Gop.SetMode                    = SRESetMode;
    mSRE.Gop.Mode                       = mParentGop->Mode;     // Reference our parent's Mode structure directly.

    // Install our Rendering Engine protocol.
    //
    mSRE.SREProtocol.SetMousePointer    = SRESetMousePointer;
    mSRE.SREProtocol.ShowMousePointer   = SREShowMousePointer;
    mSRE.SREProtocol.MoveMousePointer   = SREMoveMousePointer;

    mSRE.SREProtocol.CreateSurface      = SRECreateSurface;
    mSRE.SREProtocol.ResizeSurface      = SREResizeSurface;
    mSRE.SREProtocol.ActivateSurface    = SREActivateSurface;
    mSRE.SREProtocol.DeleteSurface      = SREDeleteSurface;
    mSRE.SREProtocol.SetModeSurface     = SRESetModeSurface;

    Status = gBS->InstallMultipleProtocolInterfaces (&mSREGopHandle,
                                                     &gEfiGraphicsOutputProtocolGuid,
                                                     (VOID**) &mSRE.Gop,
                                                     &gMsSREProtocolGuid,
                                                     (VOID**) &mSRE.SREProtocol,
                                                     NULL,
                                                     NULL
                                                    );

    ASSERT_EFI_ERROR(Status);

    if (EFI_ERROR (Status))
    {
        DEBUG ((DEBUG_ERROR, "ERROR [SRE]: Failed to install GOP (%r).\r\n", Status));
        goto Exit;
    }

    DEBUG ((DEBUG_INFO, "INFO [SRE]: Registered our own GOP protocol, Handle=0x%x, Status: %r\r\n", mSREGopHandle, Status));


    // Create a timer event to regularly sample active surface frames and confirm someone hasn't used the framebuffer pointer directly to step on the surface.
    //
    Status = gBS->CreateEvent (EVT_TIMER | EVT_NOTIFY_SIGNAL,
                               TPL_CALLBACK,
                               SampleSurfaceFrameTimerCallback,
                               NULL,
                               &mSampleSurfaceFrameTimerEvent
                              );

    if (EFI_ERROR (Status))
    {
        DEBUG ((DEBUG_ERROR, "ERROR [SRE]: Failed to create timer event for sampling surface frame (%r).\r\n", Status));
        goto Exit;
    }

    // Start a periodic timer to sample active surface frames.
    //
    Status = gBS->SetTimer (mSampleSurfaceFrameTimerEvent,
                            TimerPeriodic,
                            SURFACE_FRAME_SAMPLE_REFRESH_INTERVAL
                           );

Exit:

    return Status;
}


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
                    IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath)
{
    EFI_STATUS                      Status  = EFI_SUCCESS;
    EFI_GRAPHICS_OUTPUT_PROTOCOL    *Gop;


    // If we've already loaded or are trying to connect to our own published protocol, skip.
    //
    if (NULL != mSREGopHandle || Controller == mImageHandle || Controller == mSREGopHandle)
    {
        Status = EFI_UNSUPPORTED;
        goto Exit;
    }

    // Check for the GOP on the controller's handle.
    //
    Status = gBS->OpenProtocol (Controller,
                                mMsGopOverrideProtocolGuid,
                                (VOID **) &Gop,
                                This->DriverBindingHandle,
                                Controller,
                                EFI_OPEN_PROTOCOL_BY_DRIVER
                               );

    if (EFI_ERROR (Status))
    {
        goto Exit;
    }

    // Close the parent GOP.
    //
    gBS->CloseProtocol (Controller,
                        mMsGopOverrideProtocolGuid,
                        This->DriverBindingHandle,
                        Controller
                       );

Exit:

    return Status;
}


/**
    Connects to the controller then manufactures a new version of the GOP for other drivers to attach to.

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
                IN EFI_DEVICE_PATH_PROTOCOL         *RemainingDevicePath)
{
    EFI_STATUS  Status  = EFI_SUCCESS;


    DEBUG((DEBUG_INFO, "INFO [SRE]: Driver start (Controller=0x%x).\r\n", (UINTN)Controller));

    // Locate the parent (real) GOP.
    //
    Status = gBS->OpenProtocol (Controller,
                                mMsGopOverrideProtocolGuid,
                                (VOID **) &mParentGop,
                                This->DriverBindingHandle,
                                NULL,
                                EFI_OPEN_PROTOCOL_GET_PROTOCOL
                               );

    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [SRE]: Failed to open GOP (%r).\r\n", Status));
        goto Exit;
    }

    // Manufacture a new GOP and a RenderingEngine Protocol.
    //
    mSREGopHandle = Controller;
    Status = InitializeRenderingEngine ();

Exit:

    DEBUG((DEBUG_INFO, "INFO [SRE]: Driver start Exit (%r).\r\n", Status));

    return Status;
}


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
SREDriverStop (IN EFI_DRIVER_BINDING_PROTOCOL     *This,
               IN EFI_HANDLE                      Controller,
               IN UINTN                           NumberOfChildren,
               IN EFI_HANDLE                      *ChildHandleBuffer)
{
    EFI_STATUS          Status = EFI_SUCCESS;


    DEBUG((DEBUG_INFO, "INFO [SRE]: Driver stop Entry (Controller=0x%x).\r\n", (UINTN)Controller));


    // Cancel the surface frame sampling timer.
    //
    gBS->SetTimer (mSampleSurfaceFrameTimerEvent,
                   TimerCancel,
                   0
                  );

    // Uninstall protocol interfaces.
    //
    Status = gBS->UninstallMultipleProtocolInterfaces (mImageHandle,
                                                       &gEfiGraphicsOutputProtocolGuid,
                                                       (VOID**)&mSRE.Gop,
                                                       &gMsSREProtocolGuid,
                                                       (VOID**)&mSRE.SREProtocol,
                                                       NULL
                                                      );

    if (EFI_ERROR (Status))
    {
        goto Exit;
    }

    // Delete all surfaces.
    //
    while (NULL != mSRE.Surfaces)
    {
        SREDeleteSurface (&mSRE.SREProtocol,
                          mSRE.Surfaces->ImageHandle
                         );
    }

    // Close the parent (real) GOP.
    //
    gBS->CloseProtocol (Controller,
                        mMsGopOverrideProtocolGuid,
                        This->DriverBindingHandle,
                        Controller
                       );

    mParentGop      = NULL;
    mSREGopHandle   = NULL;

Exit:

    DEBUG((DEBUG_INFO, "INFO [SRE]: Driver stop Exit (%r).\r\n", Status));

    return Status;
}


/**
  Main entry point for this driver.

  @param[in] ImageHandle        Image handle for this driver.
  @param[in] SystemTable        Pointer to the system table.

  @retval EFI_SUCCESS           Initialization completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Insufficient resources to initialize.

**/
EFI_STATUS
EFIAPI
DriverInit (IN EFI_HANDLE         ImageHandle,
            IN EFI_SYSTEM_TABLE  *SystemTable)
{
    EFI_STATUS      Status = EFI_SUCCESS;

    // Save the image handle for later.
    //
    mImageHandle = ImageHandle;

    mMsGopOverrideProtocolGuid = PcdGetPtr(PcdMsGopOverrideProtocolGuid);

    // Install the Driver Binding Protocol.
    //
    Status = EfiLibInstallDriverBindingComponentName2 (ImageHandle,
                                                       gST,
                                                       &mSREDriverBinding,
                                                       ImageHandle,
                                                       NULL,
                                                       NULL
                                                       );

    ASSERT_EFI_ERROR (Status);

    return Status;
}


/**
  Driver unload handler.

  @param[in] ImageHandle    Image handle this driver.

  @retval EFI_SUCCESS       This function always complete successfully.

**/
EFI_STATUS
EFIAPI
DriverUnload (IN EFI_HANDLE  ImageHandle)
{
    EFI_STATUS    Status = EFI_SUCCESS;

    // TODO

    return Status;
}

