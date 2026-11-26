/** @file

  Simple Rendering Engine (SRE) implementation.

Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RenderingEngineInternal.h"

// ****** Preprocessor constants ******
//
#define SURFACE_FRAME_SAMPLE_REFRESH_INTERVAL  (200 * 10 * 1000)            // Sample surface frames: 200ms in 100ns units
#define SURFACE_FRAME_SAMPLE_PIXEL_SPACING     50                           // Pixel spacing when checking whether the surface frame has been changed.

// ****** Global variables ******
//
EFI_HANDLE                    mImageHandle;
EFI_HANDLE                    mParentHandle;                // Handle of the parent GOP.
EFI_HANDLE                    mSREGopHandle;                // SRE-owned child handle carrying the proxy GOP and the SRE protocol.
EFI_GRAPHICS_OUTPUT_PROTOCOL  *mParentGop;
RENDERING_ENGINE_CONTEXT      mSRE;
EFI_EVENT                     mSampleSurfaceFrameTimerEvent;
EFI_EVENT                     mPreExitBootServicesEvent;
BOOLEAN                       mPreExitBootServices = FALSE;

// The parent GOP's original Blt, which SREBlt replaces and wraps.
EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT  mOriginalGopBlt;
BOOLEAN                           mInSREBlt = FALSE;

// ****** Typedefs and structures ******
//

// Rendering Engine driver binding protocol support.
static
EFI_DRIVER_BINDING_PROTOCOL  mSREDriverBinding =
{
  SREDriverSupported,
  SREDriverStart,
  SREDriverStop,
  0x12,       // TODO
  NULL,
  NULL
};

// ****** Local function prototypes ******
//
static
EFI_STATUS
EFIAPI
SREShowMousePointer (
  IN  MS_RENDERING_ENGINE_PROTOCOL  *This,
  IN  BOOLEAN                       ShowPointer
  );

static
UINT32
CalculateSurfaceFrameChecksum (
  IN  SRE_SURFACE_LIST  *Surface
  );

VOID
ShutdownTimer (
  VOID
  );

VOID
DisplaySurfaceList (
  VOID
  )
{
  SRE_SURFACE_LIST  *Surface;

  Surface = mSRE.Surfaces;
  while (Surface != NULL) {
    DEBUG ((
      DEBUG_INFO,
      "            - ImageHandle=0x%x, Active=%s, Surface=L[%d]:R[%d]:T[%d]:B[%d]\r\n",
      (UINTN)Surface->ImageHandle,
      (Surface->Active ? L"YES" : L"NO"),
      Surface->FrameRect.Left,
      Surface->FrameRect.Right,
      Surface->FrameRect.Top,
      Surface->FrameRect.Bottom
      ));

    Surface = Surface->pNext;
  }
}

// ****** External definitions ******
//

// ****** Function declarations ******
//

/**
  Checks that the parent GOP is still installed on the controller.

  The parent GOP can be removed without this driver being notified, so this must be checked
  before every use of the parent GOP.

  @retval TRUE   The parent GOP can be used.
  @retval FALSE  The parent GOP is not available.

**/
static
BOOLEAN
IsParentGopValid (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop;

  if ((mParentGop == NULL) || (mParentHandle == NULL)) {
    return FALSE;
  }

  Status = gBS->OpenProtocol (
                  mParentHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&Gop,
                  mImageHandle,
                  mParentHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  return !EFI_ERROR (Status) && (Gop == mParentGop);
}

/**
  Copies the parent GOP's mode data into the proxy GOP's own mode structures.

  The parent GOP must be valid.

**/
static
VOID
SyncProxyMode (
  VOID
  )
{
  if ((mParentGop->Mode == NULL) || (mParentGop->Mode->Info == NULL)) {
    return;
  }

  CopyMem (&mSRE.GopModeInfo, mParentGop->Mode->Info, sizeof (mSRE.GopModeInfo));
  mSRE.GopMode.MaxMode         = mParentGop->Mode->MaxMode;
  mSRE.GopMode.Mode            = mParentGop->Mode->Mode;
  mSRE.GopMode.Info            = &mSRE.GopModeInfo;
  mSRE.GopMode.SizeOfInfo      = sizeof (mSRE.GopModeInfo);
  mSRE.GopMode.FrameBufferBase = mParentGop->Mode->FrameBufferBase;
  mSRE.GopMode.FrameBufferSize = mParentGop->Mode->FrameBufferSize;
}

/**
  Frees the mouse pointer buffers and clears the pointer description.

**/
static
VOID
FreeMousePointerBuffers (
  VOID
  )
{
  if (NULL != mSRE.MousePointerBitmap) {
    FreePool (mSRE.MousePointerBitmap);
    mSRE.MousePointerBitmap = NULL;
  }

  if (NULL != mSRE.MousePointerBltBuffer) {
    FreePool (mSRE.MousePointerBltBuffer);
    mSRE.MousePointerBltBuffer = NULL;
  }

  if (NULL != mSRE.MousePointerBackBuffer) {
    FreePool (mSRE.MousePointerBackBuffer);
    mSRE.MousePointerBackBuffer = NULL;
  }

  mSRE.MousePointerWidth  = 0;
  mSRE.MousePointerHeight = 0;
  mSRE.MousePointerBpp    = 0;
}

static
EFI_STATUS
DrawMousePointer (
  IN BOOLEAN  ShowPointer,
  IN UINTN    NewOrigX,
  IN UINTN    NewOrigY
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINTN       Index;

  if (!IsParentGopValid () ||
      (mSRE.MousePointerBitmap == NULL) ||
      (mSRE.MousePointerBltBuffer == NULL) ||
      (mSRE.MousePointerBackBuffer == NULL))
  {
    return EFI_NOT_READY;
  }

  // Restore the location where the mouse pointer currently resides with the original screen content.
  //
  if (TRUE == mSRE.ShowingMousePointer) {
    mOriginalGopBlt (
      mParentGop,
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
  if (FALSE == ShowPointer) {
    goto Exit;
  }

  // Otherwise capture screen contents at the new location.
  //
  mOriginalGopBlt (
    mParentGop,
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
  mOriginalGopBlt (
    mParentGop,
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
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pPointer = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)mSRE.MousePointerBitmap;

  for (Index = 0; Index < (mSRE.MousePointerWidth * mSRE.MousePointerHeight); Index++) {
    // If the mouse pointer pixel isn't black, copy it to the blt buffer.
    //
    if (mSRE.MousePointerBitmap[Index] != 0) {
      mSRE.MousePointerBltBuffer[Index] = pPointer[Index];
    }
  }

  // Blt the result to the screen
  //
  mOriginalGopBlt (
    mParentGop,
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
ValueInRange (
  IN UINT32  Value,
  IN UINT32  Min,
  IN UINT32  Max
  )
{
  return (Value >= Min) && (Value <= Max);
}

static
BOOLEAN
RectsOverlap (
  IN SWM_RECT  A,
  IN SWM_RECT  B
  )
{
  BOOLEAN  XOverlap = ValueInRange (A.Left, B.Left, B.Right) ||
                      ValueInRange (B.Left, A.Left, A.Right);

  BOOLEAN  YOverlap = ValueInRange (A.Top, B.Top, B.Bottom) ||
                      ValueInRange (B.Top, A.Top, A.Bottom);

  return XOverlap && YOverlap;
}

static
EFI_STATUS
EFIAPI
SREBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta
  )
{
  EFI_STATUS        Status      = EFI_SUCCESS;
  EFI_TPL           PreviousTPL = 0;
  SRE_SURFACE_LIST  *Surface;
  SWM_RECT          BltRect;
  SWM_RECT          PointerRect;
  UINT32            FrameWidth, FrameHeight;
  BOOLEAN           MousePointerState = mSRE.ShowingMousePointer;

  // Only reachable through the parent GOP, so just check that SRE was not released while still hooked.
  //
  if (mOriginalGopBlt == NULL) {
    return EFI_NOT_READY;
  }

  // Current blit operation bounding rectangle.
  //
  SWM_RECT_INIT2 (
    BltRect,
    (UINT32)DestinationX,
    (UINT32)DestinationY,
    (UINT32)Width,
    (UINT32)Height
    );

  // Raise the TPL to avoid interrupting rendering and framebuffer capture.
  //
  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

  // The parent's Blt can call back into the hooked Blt, which must go straight through.
  //
  if (mInSREBlt) {
    Status = mOriginalGopBlt (
               mParentGop,
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
    gBS->RestoreTPL (PreviousTPL);
    return Status;
  }

  mInSREBlt = TRUE;

  // Current mouse pointer bounding rectangle.
  //
  SWM_RECT_INIT2 (
    PointerRect,
    (UINT32)mSRE.MousePointerOrigX,
    (UINT32)mSRE.MousePointerOrigY,
    (UINT32)mSRE.MousePointerWidth,
    (UINT32)mSRE.MousePointerHeight
    );

  // If the blit intersects with the mouse, we need to temporarily hide the mouse pointer.
  //
  if ((TRUE == mSRE.ShowingMousePointer) && (TRUE == RectsOverlap (PointerRect, BltRect))) {
    SREShowMousePointer (
      &mSRE.SREProtocol,
      FALSE
      );
  }

  // First see if the blit intersects with one of the active surfaces.  If it does, restore surface back buffer contents first.  We ignore
  // a surface if the blitting flag is set so that drawing to a surface doesn't trigger a self-refresh.
  //
  Surface = mSRE.Surfaces;
  while ((NULL != Surface) && (EfiBltVideoToBltBuffer != BltOperation)) {
    if ((TRUE  == Surface->Active) &&
        (FALSE == Surface->BlittingSurface) &&
        ((TRUE  == RectsOverlap (Surface->FrameRect, BltRect)) || (Surface->FrameChecksum != CalculateSurfaceFrameChecksum (Surface))))
    {
      FrameWidth  = SWM_RECT_WIDTH (Surface->FrameRect);
      FrameHeight = SWM_RECT_HEIGHT (Surface->FrameRect);

      // Remember that we need to notify the client to redraw.
      //
      Surface->PaintNotify = TRUE;

      // If restoring the screen under the surface intersects with the mouse, we need to temporarily hide the mouse pointer.
      //
      if ((TRUE == mSRE.ShowingMousePointer) && (TRUE == RectsOverlap (PointerRect, Surface->FrameRect))) {
        SREShowMousePointer (
          &mSRE.SREProtocol,
          FALSE
          );
      }

      // Restore the contents to the framebuffer.
      //
      mOriginalGopBlt (
        mParentGop,
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
  mOriginalGopBlt (
    mParentGop,
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
  while ((NULL != Surface) && (EfiBltVideoToBltBuffer != BltOperation)) {
    // Capture screen contents for any surfaces that intersected with the blit operation.  Again, we can ignore any surfaces which are marked
    // with the blitting flag in order to avoid triggering a refresh.
    //
    if (TRUE  == Surface->Active) {
      if ((FALSE == Surface->BlittingSurface) &&
          (TRUE  == RectsOverlap (Surface->FrameRect, BltRect)))
      {
        FrameWidth  = SWM_RECT_WIDTH (Surface->FrameRect);
        FrameHeight = SWM_RECT_HEIGHT (Surface->FrameRect);

        // Save the contents of the framebuffer to this capture buffer.
        //
        mOriginalGopBlt (
          mParentGop,
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
  if ((TRUE == MousePointerState) && (FALSE == mSRE.ShowingMousePointer)) {
    SREShowMousePointer (
      &mSRE.SREProtocol,
      TRUE
      );
  }

  mInSREBlt = FALSE;

  // Restore the TPL.
  //
  if (PreviousTPL) {
    gBS->RestoreTPL (PreviousTPL);
  }

  return Status;
}

static
EFI_STATUS
EFIAPI
SREQueryMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
{
  if (!IsParentGopValid ()) {
    return EFI_NOT_READY;
  }

  SyncProxyMode ();

  return mParentGop->QueryMode (
                       mParentGop,
                       ModeNumber,
                       SizeOfInfo,
                       Info
                       );
}

static
EFI_STATUS
EFIAPI
SRESetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
  IN  UINT32                        ModeNumber
  )
{
  EFI_STATUS  Status;
  EFI_TPL     PreviousTPL;

  if (!IsParentGopValid ()) {
    return EFI_NOT_READY;
  }

  // Raise the TPL to avoid getting interrupted while we access shared data structures.
  //
  PreviousTPL = EfiGetCurrentTpl ();
  if (PreviousTPL < TPL_CALLBACK) {
    PreviousTPL = gBS->RaiseTPL (TPL_CALLBACK);
  }

  Status = mParentGop->SetMode (
                         mParentGop,
                         ModeNumber
                         );
  if (!EFI_ERROR (Status)) {
    SyncProxyMode ();
  }

  // Restore the TPL.
  //
  gBS->RestoreTPL (PreviousTPL);

  return Status;
}

static
EFI_STATUS
EFIAPI
SRESetMousePointer (
  IN  MS_RENDERING_ENGINE_PROTOCOL  *This,
  IN  const UINT32                  *MouseBitmap,
  IN  UINT32                        Width,
  IN  UINT32                        Height,
  IN  UINT32                        Bpp
  )
{
  EFI_STATUS  Status      = EFI_SUCCESS;
  UINTN       BuffSize    = 0;
  EFI_TPL     PreviousTPL = 0;

  // Validate function parameters.
  //
  if (0 != (Bpp % 8)) {
    DEBUG ((DEBUG_ERROR, "ERROR [SRE]: SRESetMousePointer: pointer bitmap bpp should be an integral number of bytes (Bpp=%d).\r\n", Bpp));
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  // Raise the TPL to avoid anyone interrupting rendering and framebuffer capture.
  //
  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

  // Free the existing mouse pointer buffers if they exist.
  //
  FreeMousePointerBuffers ();

  // Allocate a buffer to hold a copy of the mouse pointer bitmap.
  //
  BuffSize                = (Width * Height * (Bpp / 8));
  mSRE.MousePointerBitmap = AllocateZeroPool (BuffSize);

  ASSERT (NULL != mSRE.MousePointerBitmap);
  if (NULL == mSRE.MousePointerBitmap) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  // Save the mouse pointer information being provided.
  //
  mSRE.MousePointerWidth  = Width;
  mSRE.MousePointerHeight = Height;
  mSRE.MousePointerBpp    = Bpp;

  CopyMem (mSRE.MousePointerBitmap, MouseBitmap, BuffSize);

  // Allocate a working buffer for blitting the mouse pointer.
  //
  BuffSize                   = (Width * Height * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  mSRE.MousePointerBltBuffer = AllocateZeroPool (BuffSize);

  ASSERT (NULL != mSRE.MousePointerBltBuffer);
  if (NULL == mSRE.MousePointerBltBuffer) {
    FreeMousePointerBuffers ();
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  // Allocate a backing buffer for the mouse pointer (used to restore underlying screen contents).
  //
  mSRE.MousePointerBackBuffer = AllocateZeroPool (BuffSize);

  ASSERT (NULL != mSRE.MousePointerBackBuffer);
  if (NULL == mSRE.MousePointerBackBuffer) {
    FreeMousePointerBuffers ();
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

Exit:

  // Restore the TPL.
  //
  if (PreviousTPL) {
    gBS->RestoreTPL (PreviousTPL);
  }

  return Status;
}

static
EFI_STATUS
EFIAPI
SREShowMousePointer (
  IN  MS_RENDERING_ENGINE_PROTOCOL  *This,
  IN  BOOLEAN                       ShowPointer
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  // Refresh the mouse pointer region (to hide or show it) if the state changed.
  //
  if (ShowPointer != mSRE.ShowingMousePointer) {
    Status = DrawMousePointer (
               ShowPointer,
               mSRE.MousePointerOrigX,
               mSRE.MousePointerOrigY
               );

    if (EFI_SUCCESS == Status) {
      // Capture the show-hide state.
      //
      mSRE.ShowingMousePointer = ShowPointer;
    }
  }

  return Status;
}

static
EFI_STATUS
EFIAPI
SREMoveMousePointer (
  IN  MS_RENDERING_ENGINE_PROTOCOL  *This,
  IN  UINT32                        OrigX,
  IN  UINT32                        OrigY
  )
{
  EFI_STATUS  Status      = EFI_SUCCESS;
  EFI_TPL     PreviousTPL = 0;

  // If the mouse pointer isn't being displayed, simply exit.  We won't even try to capture
  // the updated location.
  //
  if (FALSE == mSRE.ShowingMousePointer) {
    goto Exit;
  }

  // Raise the TPL to avoid anyone interrupting rendering and framebuffer capture.
  //
  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

  // Refresh the mouse pointer position if the location changed.
  //
  if ((OrigX != mSRE.MousePointerOrigX) || (OrigY != mSRE.MousePointerOrigY)) {
    Status = DrawMousePointer (
               mSRE.ShowingMousePointer,
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
  if (PreviousTPL) {
    gBS->RestoreTPL (PreviousTPL);
  }

  return Status;
}

VOID
EFIAPI
CheckForPendingPaintRequest (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  SRE_SURFACE_LIST  *Surface = (SRE_SURFACE_LIST *)Context;
  SRE_SURFACE_LIST  *Node;

  // The client can still hold this event after the surface was deleted, so confirm the context is still a live surface.
  //
  Node = mSRE.Surfaces;
  while ((NULL != Node) && (Node != Surface)) {
    Node = Node->pNext;
  }

  if (NULL == Node) {
    return;
  }

  if (TRUE == Surface->PaintNotify) {
    Surface->PaintNotify = FALSE;
    gBS->SignalEvent (Event);
  }
}

static
UINT32
CalculateSurfaceFrameChecksum (
  IN  SRE_SURFACE_LIST  *Surface
  )
{
  UINT32                         Width  = SWM_RECT_WIDTH (Surface->FrameRect);
  UINT32                         Height = SWM_RECT_HEIGHT (Surface->FrameRect);
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *SurfaceOrigin;
  UINT32                         Offset;
  UINT32                         Checksum = 0;

  if ((mParentGop == NULL) ||
      (mParentGop->Mode == NULL) ||
      (mParentGop->Mode->Info == NULL) ||
      (mParentGop->Mode->FrameBufferBase == 0) ||
      (mParentGop->Mode->Info->PixelFormat == PixelBltOnly) ||
      (Surface->FrameRect.Right >= mParentGop->Mode->Info->HorizontalResolution) ||
      (Surface->FrameRect.Bottom >= mParentGop->Mode->Info->VerticalResolution))
  {
    return 0;
  }

  // Sample top edge.
  //
  SurfaceOrigin = ((EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)mParentGop->Mode->FrameBufferBase + (Surface->FrameRect.Top * mParentGop->Mode->Info->PixelsPerScanLine) + Surface->FrameRect.Left);
  for (Offset = 0; Offset < Width; Offset += SURFACE_FRAME_SAMPLE_PIXEL_SPACING) {
    Checksum += *(UINT32 *)(SurfaceOrigin + Offset);
  }

  // Sample left and right edges and midpoint bisecting line.
  //
  for (Offset = 0; Offset < Height; Offset += SURFACE_FRAME_SAMPLE_PIXEL_SPACING) {
    // Left edge.
    Checksum += *(UINT32 *)(SurfaceOrigin + (Offset * mParentGop->Mode->Info->PixelsPerScanLine));

    // Midpoint bisecting line.
    Checksum += *(UINT32 *)(SurfaceOrigin + (Offset * mParentGop->Mode->Info->PixelsPerScanLine) + ((Width - 1) /2));

    // Right edge.
    Checksum += *(UINT32 *)(SurfaceOrigin + (Offset * mParentGop->Mode->Info->PixelsPerScanLine) + (Width - 1));
  }

  // Sample bottom edge.
  //
  SurfaceOrigin = ((EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)mParentGop->Mode->FrameBufferBase + ((Surface->FrameRect.Bottom - 1) * mParentGop->Mode->Info->PixelsPerScanLine) + Surface->FrameRect.Left);
  for (Offset = 0; Offset < Width; Offset += SURFACE_FRAME_SAMPLE_PIXEL_SPACING) {
    Checksum += *(UINT32 *)(SurfaceOrigin + Offset);
  }

  return Checksum;
}

VOID
EFIAPI
SampleSurfaceFrameTimerCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  SRE_SURFACE_LIST  *Surface;

  // Raise the TPL to avoid getting interrupted while we access shared data structures.
  //
  EFI_TPL  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

  // Stop sampling once the parent GOP is gone since the framebuffer is no longer valid.
  //
  if (!IsParentGopValid ()) {
    gBS->SetTimer (Event, TimerCancel, 0);
    gBS->RestoreTPL (PreviousTPL);
    return;
  }

  // The parent's mode can change without going through the proxy.
  //
  SyncProxyMode ();

  // Check whether any active surface's frame has been altered.
  //
  Surface = mSRE.Surfaces;
  while (NULL != Surface) {
    if ((TRUE == Surface->Active) && (Surface->FrameChecksum != CalculateSurfaceFrameChecksum (Surface))) {
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
EFIAPI
SRECreateSurface (
  IN  MS_RENDERING_ENGINE_PROTOCOL  *This,
  IN  EFI_HANDLE                    ImageHandle,
  IN  SWM_RECT                      FrameRect,
  OUT EFI_EVENT                     *PaintEvent
  )
{
  EFI_STATUS        Status = EFI_SUCCESS;
  SRE_SURFACE_LIST  *Surface;
  SRE_SURFACE_LIST  *pTemp;
  EFI_TPL           PreviousTPL = 0;

  DEBUG ((DEBUG_INFO, "INFO [SRE]: Creating a new surface (ImageHandle=0x%x).\r\n", (UINTN)ImageHandle));

  // Raise the TPL to avoid getting interrupted while we access shared data structures.
  //
  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

  // Check whether this image handle has already been used to register a surface.
  //
  Surface = mSRE.Surfaces;
  while (NULL != Surface) {
    if (Surface->ImageHandle == ImageHandle) {
      Status = EFI_ALREADY_STARTED;
      goto Exit;
    }

    Surface = Surface->pNext;
  }

  // Allocate a new node for this surface.
  //
  pTemp = (SRE_SURFACE_LIST *)AllocateZeroPool (sizeof (SRE_SURFACE_LIST));

  ASSERT (NULL != pTemp);
  if (NULL == pTemp) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  // Capture surface information.
  //
  pTemp->pNext          = mSRE.Surfaces;
  pTemp->pPrev          = NULL;
  pTemp->ImageHandle    = ImageHandle;
  pTemp->Active         = FALSE;
  pTemp->PaintNotify    = FALSE;
  pTemp->PreviousActive = NULL;

  CopyMem (&pTemp->FrameRect, &FrameRect, sizeof (SWM_RECT));

  // Allocate storage for the backing buffer.
  //
  UINT32  Width  = SWM_RECT_WIDTH (FrameRect);
  UINT32  Height = SWM_RECT_HEIGHT (FrameRect);

  // Allocate a capture buffer for the surface.
  //
  pTemp->pCaptureBuffer = AllocatePool (Width * Height * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

  ASSERT (NULL != pTemp->pCaptureBuffer);
  if (NULL == pTemp->pCaptureBuffer) {
    FreePool (pTemp);
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  // Create a custom paint event for this client with EVT_NOTIFY_WAIT so we're called with the client's
  // context whenever the client waits on it.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  CheckForPendingPaintRequest,
                  (VOID *)pTemp,
                  PaintEvent
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [SRE]: Failed to create event for notifying client of a surface paint request (%r).\r\n", Status));
    FreePool (pTemp->pCaptureBuffer);
    FreePool (pTemp);
    goto Exit;
  }

  // Attach it to the list
  //
  if (mSRE.Surfaces != NULL) {
    mSRE.Surfaces->pPrev = pTemp;
  }

  mSRE.Surfaces = pTemp;

Exit:

  // Restore the TPL
  //
  if (PreviousTPL) {
    gBS->RestoreTPL (PreviousTPL);
  }

  // Display client list for debugging purposes.
  //
  DEBUG ((DEBUG_INFO, "INFO [SRE]: Surface list:\r\n"));
  DisplaySurfaceList ();

  return Status;
}

static
EFI_STATUS
EFIAPI
SREResizeSurface (
  IN  MS_RENDERING_ENGINE_PROTOCOL  *This,
  IN  EFI_HANDLE                    ImageHandle,
  IN  SWM_RECT                      *FrameRect
  )
{
  EFI_STATUS        Status = EFI_SUCCESS;
  SRE_SURFACE_LIST  *Surface;
  UINT32            Width, Height;
  BOOLEAN           MousePointerState = mSRE.ShowingMousePointer;

  DEBUG ((DEBUG_INFO, "INFO [SRE]: Resizing surface (ImageHandle=0x%x).\r\n", (UINTN)ImageHandle));

  if (!IsParentGopValid ()) {
    return EFI_NOT_READY;
  }

  // Raise the TPL to avoid getting interrupted while we access shared data structures.
  //
  EFI_TPL  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

  // Hide the mouse pointer.
  //
  if (TRUE == MousePointerState) {
    SREShowMousePointer (
      &mSRE.SREProtocol,
      FALSE
      );
  }

  // Set active state for the specified surface (note this assumes we'll find a match for ImageHandle).
  //
  Surface = mSRE.Surfaces;
  while (NULL != Surface) {
    if (Surface->ImageHandle == ImageHandle) {
      // Check whether a framebuffer already exists.  If so, free it.
      //
      if (NULL != Surface->pCaptureBuffer) {
        // If the surface is active, restore the backing buffer before resizing.
        //
        if (TRUE == Surface->Active) {
          Width  = SWM_RECT_WIDTH (Surface->FrameRect);
          Height = SWM_RECT_HEIGHT (Surface->FrameRect);

          // Restore the contents to the framebuffer.
          //
          mOriginalGopBlt (
            mParentGop,
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
      CopyMem (&Surface->FrameRect, FrameRect, sizeof (SWM_RECT));

      // Allocate storage for the backing buffer.
      //
      Width                   = SWM_RECT_WIDTH (*FrameRect);
      Height                  = SWM_RECT_HEIGHT (*FrameRect);
      Surface->pCaptureBuffer = AllocatePool (Width * Height * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

      ASSERT (NULL != Surface->pCaptureBuffer);
      if (NULL == Surface->pCaptureBuffer) {
        Surface->Active = FALSE;
        Status          = EFI_OUT_OF_RESOURCES;
        break;
      }

      // If the surface is active, capture screen contents to the new buffer.
      //
      if (TRUE == Surface->Active) {
        // Save the contents of the framebuffer to this capture buffer.
        //
        mOriginalGopBlt (
          mParentGop,
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
  if (TRUE == MousePointerState) {
    SREShowMousePointer (
      &mSRE.SREProtocol,
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
EFIAPI
SREActivateSurface (
  IN  MS_RENDERING_ENGINE_PROTOCOL  *This,
  IN  EFI_HANDLE                    ImageHandle,
  IN  BOOLEAN                       MakeActive
  )
{
  EFI_STATUS                     Status = EFI_NOT_FOUND;
  SRE_SURFACE_LIST               *Surface;
  SRE_SURFACE_LIST               *ActiveSurface;
  UINT32                         FrameWidth, FrameHeight;
  BOOLEAN                        MousePointerState = mSRE.ShowingMousePointer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  BlackPixel        = { 0, 0, 0, 0 };

  DEBUG ((DEBUG_INFO, "INFO [SRE]: Setting surface active (ImageHandle=0x%x, MakeActive=%s).\r\n", (UINTN)ImageHandle, (TRUE == MakeActive ? L"TRUE" : L"FALSE")));

  if (!IsParentGopValid ()) {
    return EFI_NOT_READY;
  }

  // Raise the TPL to avoid getting interrupted while we access shared data structures.
  //
  EFI_TPL  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

  // Hide the mouse pointer.
  //
  if (TRUE == MousePointerState) {
    SREShowMousePointer (
      &mSRE.SREProtocol,
      FALSE
      );
  }

  // Set active state for the specified surface (note this assumes we'll find a match for ImageHandle).
  //
  ActiveSurface = NULL;
  Surface       = mSRE.Surfaces;

  // Find previous Active Surface
  while (NULL != Surface) {
    if (Surface->Active) {
      ActiveSurface = Surface;
      break;
    }

    Surface = Surface->pNext;
  }

  Surface = mSRE.Surfaces;
  while (NULL != Surface) {
    if (Surface->ImageHandle == ImageHandle) {
      // A surface without a capture buffer (failed resize) can't be activated or deactivated.
      //
      if (NULL == Surface->pCaptureBuffer) {
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }

      Surface->Active = MakeActive;

      FrameWidth  = SWM_RECT_WIDTH (Surface->FrameRect);
      FrameHeight = SWM_RECT_HEIGHT (Surface->FrameRect);

      if (TRUE == MakeActive) {
        Surface->PreviousActive = ActiveSurface;
        if (ActiveSurface != NULL) {
          ActiveSurface->Active = FALSE;
        }

        // Save the contents of the framebuffer to this capture buffer.
        //
        mOriginalGopBlt (
          mParentGop,
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
      } else {
        if (Surface->PreviousActive != NULL) {
          Surface->PreviousActive->Active = TRUE;
          Surface->PreviousActive         = NULL;
        }

        // Restore the contents to the framebuffer.
        //

        if (mPreExitBootServices) {
          mOriginalGopBlt (
            mParentGop,
            &BlackPixel,
            EfiBltVideoFill,
            0,
            0,
            Surface->FrameRect.Left,
            Surface->FrameRect.Top,
            FrameWidth,
            FrameHeight,
            0
            );
        } else {
          mOriginalGopBlt (
            mParentGop,
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
      }

      // Compute the surface frame checksum.
      //
      Surface->FrameChecksum = CalculateSurfaceFrameChecksum (Surface);

      Status = EFI_SUCCESS;
      break;
    }

    Surface = Surface->pNext;
  }

  // Restore the mouse pointer if it was displayed before, and
  // not after PreExitbootServices
  //
  if ((MousePointerState) && (!mPreExitBootServices)) {
    SREShowMousePointer (
      &mSRE.SREProtocol,
      TRUE
      );
  }

  // Restore the TPL.
  //
  gBS->RestoreTPL (PreviousTPL);

  DEBUG ((DEBUG_INFO, "INFO [SRE]: Activate surface h=%p, Active=%d\n", ImageHandle, MakeActive));
  DisplaySurfaceList ();

  return Status;
}

static
EFI_STATUS
EFIAPI
SREDeleteSurface (
  IN  MS_RENDERING_ENGINE_PROTOCOL  *This,
  IN  EFI_HANDLE                    ImageHandle
  )
{
  EFI_STATUS        Status = EFI_SUCCESS;
  SRE_SURFACE_LIST  *Surface;

  DEBUG ((DEBUG_INFO, "INFO [SRE]: Deleting surface (ImageHandle=0x%x).\r\n", (UINTN)ImageHandle));

  // Raise the TPL to avoid getting interrupted while we access shared data structures.
  //
  EFI_TPL  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

  // Search for the specified image handle's client (note this assumes we'll find a match for ImageHandle).
  //
  Surface = mSRE.Surfaces;
  while (NULL != Surface) {
    if (Surface->ImageHandle == ImageHandle) {
      // If a screen capture buffer for the client's surface area exists, free it.
      //
      if (NULL != Surface->pCaptureBuffer) {
        FreePool (Surface->pCaptureBuffer);
        Surface->pCaptureBuffer = NULL;
      }

      // Unlink the current client node and free it.
      //
      if (NULL == Surface->pPrev) {
        mSRE.Surfaces = Surface->pNext;
        if (NULL != mSRE.Surfaces) {
          mSRE.Surfaces->pPrev = NULL;
        }
      } else {
        Surface->pPrev->pNext = Surface->pNext;
        if (NULL != Surface->pNext) {
          Surface->pNext->pPrev = Surface->pPrev;
        }
      }

      {
        // When deleting a Surface, clean up any other Surfaces that
        // saved a "Previous Active" pointer to this Surface.
        SRE_SURFACE_LIST  *Surface2;
        Surface2 = mSRE.Surfaces;
        while (NULL != Surface2) {
          if (Surface2->PreviousActive == Surface) {
            Surface2->PreviousActive = Surface->PreviousActive;
          }

          Surface2 = Surface2->pNext;
        }
      }

      FreePool (Surface);

      break;
    }

    Surface = Surface->pNext;
  }

  // Restore the TPL.
  //
  gBS->RestoreTPL (PreviousTPL);

  // Print out a debug message if we didn't remove anything.
  //
  if (NULL == Surface) {
    DEBUG ((DEBUG_WARN, "WARN [SRE]: Failed to delete surface registered by image handle %d.\r\n", (UINTN)ImageHandle));
  }

  // Display surface list for debugging purposes.
  //
  DEBUG ((DEBUG_INFO, "INFO [SRE]: Surface list:\r\n"));
  DisplaySurfaceList ();

  return Status;
}

static
EFI_STATUS
EFIAPI
SRESetModeSurface (
  IN  MS_RENDERING_ENGINE_PROTOCOL  *This,
  IN  EFI_HANDLE                    ImageHandle,
  IN  MS_SRE_SURFACE_MODE           Mode
  )
{
  EFI_STATUS        Status = EFI_SUCCESS;
  SRE_SURFACE_LIST  *Surface;

  // Raise the TPL to avoid getting interrupted while we access shared data structures.
  //
  EFI_TPL  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

  // Search for the specified image handle's client (note this assumes we'll find a match for ImageHandle).
  //
  Surface = mSRE.Surfaces;
  while (NULL != Surface) {
    if (Surface->ImageHandle == ImageHandle) {
      switch (Mode) {
        case PAINT_BEGIN:
          Surface->BlittingSurface = TRUE;
          break;
        case PAINT_END:
          Surface->BlittingSurface = FALSE;
          break;
        default:
          DEBUG ((DEBUG_ERROR, "ERROR [SRE]: Unrecognized surface mode (Mode=%d).\r\n", (UINT32)Mode));
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

/**
  Destroys the child handle carrying the proxy GOP and the SRE protocol.

  Every driver that opened the proxy GOP or the SRE protocol BY_DRIVER (e.g., GraphicsConsole, SWM) is
  explicitly disconnected first so the subsequent uninstall is deterministic rather than depending on
  whatever order the core happens to visit agents when the parent GOP is itself disconnected.

  @param[in] This  Pointer to the instance of this driver.

  @retval EFI_SUCCESS  The child handle was destroyed.
  @retval Others       The protocols are still installed and nothing was released.

**/
static
EFI_STATUS
DestroyChildHandle (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This
  )
{
  EFI_STATUS  Status;

  gBS->DisconnectController (mSREGopHandle, NULL, NULL);

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  mSREGopHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  &mSRE.Gop,
                  &gMsSREProtocolGuid,
                  &mSRE.SREProtocol,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  // The child must be closed on the parent GOP before its last protocol is uninstalled since that deletes the handle.
  //
  gBS->CloseProtocol (
         mParentHandle,
         &gEfiGraphicsOutputProtocolGuid,
         This->DriverBindingHandle,
         mSREGopHandle
         );

  Status = gBS->UninstallProtocolInterface (
                  mSREGopHandle,
                  &gEfiDevicePathProtocolGuid,
                  mSRE.ChildDevicePath
                  );
  if (!EFI_ERROR (Status)) {
    FreePool (mSRE.ChildDevicePath);
  }

  mSRE.ChildDevicePath = NULL;
  mSREGopHandle        = NULL;

  return EFI_SUCCESS;
}

static EFI_STATUS
InitializeRenderingEngine (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This
  )
{
  EFI_STATUS                Status = EFI_SUCCESS;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  VENDOR_DEVICE_PATH        SreNode;
  VOID                      *Interface;

  DEBUG ((DEBUG_INFO, "INFO [SRE]: Initializing the Rendering Engine.\r\n"));

  if (!IsParentGopValid () ||
      (mParentGop->Mode == NULL) ||
      (mParentGop->Mode->Info == NULL) ||
      (mParentGop->Blt == NULL) ||
      (mParentGop->Blt == SREBlt))
  {
    DEBUG ((DEBUG_ERROR, "ERROR [SRE]: Parent GOP is not usable during initialization.\r\n"));
    return EFI_NOT_READY;
  }

  // The child's device path extends the parent's so console selection by device path still matches.
  //
  Status = gBS->HandleProtocol (mParentHandle, &gEfiDevicePathProtocolGuid, (VOID **)&ParentDevicePath);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [SRE]: Parent GOP handle has no device path (%r).\r\n", Status));
    return Status;
  }

  ZeroMem (&SreNode, sizeof (SreNode));
  SreNode.Header.Type    = HARDWARE_DEVICE_PATH;
  SreNode.Header.SubType = HW_VENDOR_DP;
  SetDevicePathNodeLength (&SreNode.Header, sizeof (SreNode));
  CopyGuid (&SreNode.Guid, &gMsSREProtocolGuid);

  mSRE.ChildDevicePath = AppendDevicePathNode (ParentDevicePath, &SreNode.Header);
  if (mSRE.ChildDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Configure initial Rendering Engine context.
  //
  mOriginalGopBlt          = mParentGop->Blt;
  mSRE.ShowingMousePointer = FALSE;
  mSRE.MousePointerOrigX   = (mParentGop->Mode->Info->HorizontalResolution / 2);          // Default X position (may be adjusted later for display mode switch).
  mSRE.MousePointerOrigY   = (mParentGop->Mode->Info->VerticalResolution   / 2);          // Default Y position (may be adjusted later for display mode switch).

  // Configure the proxy GOP. It uses its own mode data so it stays valid when the parent goes away.
  //
  mSRE.Gop.QueryMode = SREQueryMode;
  mSRE.Gop.SetMode   = SRESetMode;
  mSRE.Gop.Blt       = SREBlt;
  mSRE.Gop.Mode      = &mSRE.GopMode;
  SyncProxyMode ();

  // Install our Rendering Engine protocol.
  //
  mSRE.SREProtocol.SetMousePointer  = SRESetMousePointer;
  mSRE.SREProtocol.ShowMousePointer = SREShowMousePointer;
  mSRE.SREProtocol.MoveMousePointer = SREMoveMousePointer;

  mSRE.SREProtocol.CreateSurface   = SRECreateSurface;
  mSRE.SREProtocol.ResizeSurface   = SREResizeSurface;
  mSRE.SREProtocol.ActivateSurface = SREActivateSurface;
  mSRE.SREProtocol.DeleteSurface   = SREDeleteSurface;
  mSRE.SREProtocol.SetModeSurface  = SRESetModeSurface;

  mSREGopHandle = NULL;
  Status        = gBS->InstallMultipleProtocolInterfaces (
                         &mSREGopHandle,
                         &gEfiDevicePathProtocolGuid,
                         mSRE.ChildDevicePath,
                         &gEfiGraphicsOutputProtocolGuid,
                         &mSRE.Gop,
                         &gMsSREProtocolGuid,
                         &mSRE.SREProtocol,
                         NULL
                         );

  ASSERT_EFI_ERROR (Status);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [SRE]: Failed to install the child handle protocols (%r).\r\n", Status));
    mSREGopHandle = NULL;
    goto Exit;
  }

  // Register the child on the parent GOP so the driver model stops it before the parent goes away.
  //
  Status = gBS->OpenProtocol (
                  mParentHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  &Interface,
                  This->DriverBindingHandle,
                  mSREGopHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [SRE]: Failed to register the child handle (%r).\r\n", Status));
    goto RemoveChild;
  }

  DEBUG ((DEBUG_INFO, "INFO [SRE]: Registered the proxy GOP and Rendering Engine protocol, Handle=0x%x, Status: %r\r\n", mSREGopHandle, Status));

  // Create a timer event to regularly sample active surface frames and confirm someone hasn't used the framebuffer pointer directly to step on the surface.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  SampleSurfaceFrameTimerCallback,
                  NULL,
                  &mSampleSurfaceFrameTimerEvent
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [SRE]: Failed to create timer event for sampling surface frame (%r).\r\n", Status));
    goto RemoveChild;
  }

  // Start a periodic timer to sample active surface frames.
  //
  Status = gBS->SetTimer (
                  mSampleSurfaceFrameTimerEvent,
                  TimerPeriodic,
                  SURFACE_FRAME_SAMPLE_REFRESH_INTERVAL
                  );

  if (!EFI_ERROR (Status)) {
    // Hook the parent GOP last so everything SREBlt relies on is ready. Callers that use the parent directly then also go through SRE.
    //
    mParentGop->Blt = SREBlt;
    goto Exit;
  }

  DEBUG ((DEBUG_ERROR, "ERROR [SRE]: Failed to start timer for sampling surface frame (%r).\r\n", Status));

RemoveChild:

  // Don't leave the child handle installed without the timer that supports it.
  //
  ShutdownTimer ();
  DestroyChildHandle (This);

Exit:

  return Status;
}

/**
  Delete all surfaces registered with the rendering engine.

**/
VOID
DeleteAllSurfaces (
  VOID
  )
{
  while (mSRE.Surfaces != NULL) {
    SREDeleteSurface (
      &mSRE.SREProtocol,
      mSRE.Surfaces->ImageHandle
      );
  }
}

/**
  Cancel the surface frame sampling timer.

**/
VOID
ShutdownTimer (
  VOID
  )
{
  if (mSampleSurfaceFrameTimerEvent == NULL) {
    return;
  }

  gBS->SetTimer (
         mSampleSurfaceFrameTimerEvent,
         TimerCancel,
         0
         );
  gBS->CloseEvent (mSampleSurfaceFrameTimerEvent);
  mSampleSurfaceFrameTimerEvent = NULL;
}

/**
  Release everything held since SREDriverStart() so the driver can be started again.

  The child handle must already be destroyed. The parent GOP is only accessed to unhook Blt if it's still installed.

  @param[in] This  Pointer to the instance of this driver.

**/
static
VOID
ReleaseRenderingEngine (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This
  )
{
  ShutdownTimer ();

  if (IsParentGopValid () && (mParentGop->Blt == SREBlt)) {
    mParentGop->Blt = mOriginalGopBlt;
  }

  DeleteAllSurfaces ();
  FreeMousePointerBuffers ();

  if (mSRE.ChildDevicePath != NULL) {
    FreePool (mSRE.ChildDevicePath);
  }

  gBS->CloseProtocol (
         mParentHandle,
         &gEfiGraphicsOutputProtocolGuid,
         This->DriverBindingHandle,
         mParentHandle
         );

  ZeroMem (&mSRE, sizeof (mSRE));
  mOriginalGopBlt = NULL;
  mInSREBlt       = FALSE;
  mParentGop      = NULL;
  mParentHandle   = NULL;
  mSREGopHandle   = NULL;
}

/**
  Stops the Rendering Engine, unhooking the parent GOP and releasing everything.

  @param[in] This  Pointer to the instance of this driver.

  @retval EFI_SUCCESS        The Rendering Engine was stopped or was not started.
  @retval EFI_ACCESS_DENIED  Another driver hooked Blt after SRE so SRE can't be removed from the chain.
  @retval Others             The child handle couldn't be destroyed. Nothing was released.

**/
static
EFI_STATUS
StopRenderingEngine (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This
  )
{
  EFI_STATUS  Status;

  if (mParentHandle == NULL) {
    return EFI_SUCCESS;
  }

  if (IsParentGopValid () && (mParentGop->Blt != SREBlt)) {
    return EFI_ACCESS_DENIED;
  }

  if (mSREGopHandle != NULL) {
    Status = DestroyChildHandle (This);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  ReleaseRenderingEngine (This);

  return EFI_SUCCESS;
}

/**
    Checks whether the specified controller has a GOP that the Rendering Engine can sit above.

    @param[in] This                 Pointer to the instance of this driver.
    @param[in] Controller           Controller handle to be checked.
    @param[in] RemainingDevicePath  Ignored.

    @retval EFI_SUCCESS             Found the controller we want to support.
    @retval EFI_UNSUPPORTED         Unsupported controller.

**/
EFI_STATUS
EFIAPI
SREDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                    Status = EFI_SUCCESS;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop;
  VOID                          *Interface;

  // Skip the console splitter's virtual GOP, ourselves, and handles that already carry the Rendering Engine protocol.
  //
  if ((Controller == gST->ConsoleOutHandle) ||
      (Controller == mImageHandle) ||
      !EFI_ERROR (gBS->HandleProtocol (Controller, &gMsSREProtocolGuid, &Interface)))
  {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  // Only one controller is supported. A parent that went away without notification is released by Start.
  //
  if ((NULL != mParentHandle) && IsParentGopValid ()) {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  // Check for the GOP on the controller's handle. Ownership is only taken by Start.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&Gop,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  // The child handle needs a device path to extend, and the GOP needs mode data and a Blt to wrap.
  //
  if ((Gop->Blt == NULL) ||
      (Gop->Blt == SREBlt) ||
      (Gop->Mode == NULL) ||
      (Gop->Mode->Info == NULL) ||
      EFI_ERROR (gBS->OpenProtocol (Controller, &gEfiDevicePathProtocolGuid, NULL, This->DriverBindingHandle, Controller, EFI_OPEN_PROTOCOL_TEST_PROTOCOL)))
  {
    Status = EFI_UNSUPPORTED;
  }

Exit:

  return Status;
}

/**
    Takes ownership of the controller's GOP and publishes a proxy GOP and the Rendering Engine protocol on a child handle.
    The parent's Blt is also hooked so callers that use the parent directly go through the Rendering Engine.

    @param[in] This                 Pointer to the instance of this driver.
    @param[in] Controller           Handle of controller to be managed.
    @param[in] RemainingDevicePath  Ignored.

    @retval EFI_SUCCESS             Successfully connected to the controller.
    @retval EFI_UNSUPPORTED         Failed to connect.

**/
EFI_STATUS
EFIAPI
SREDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  DEBUG ((DEBUG_INFO, "INFO [SRE]: Driver start (Controller=0x%x).\r\n", (UINTN)Controller));

  // A GOP that was removed without notification must be released before taking over a new one.
  //
  Status = StopRenderingEngine (This);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  // Take ownership of the parent (real) GOP. Any driver already using it is disconnected and reconnects to the child below.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&mParentGop,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER | EFI_OPEN_PROTOCOL_EXCLUSIVE
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [SRE]: Failed to open GOP (%r).\r\n", Status));
    mParentGop = NULL;
    goto Exit;
  }

  mParentHandle = Controller;
  Status        = InitializeRenderingEngine (This);

  if (EFI_ERROR (Status)) {
    ReleaseRenderingEngine (This);
    goto Exit;
  }

  // Let the GOP consumers attach to the child handle.
  //
  gBS->ConnectController (mSREGopHandle, NULL, NULL, TRUE);

Exit:

  DEBUG ((DEBUG_INFO, "INFO [SRE]: Driver start Exit (%r).\r\n", Status));

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
SREDriverStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  DEBUG ((DEBUG_INFO, "INFO [SRE]: Driver stop Entry (Controller=0x%x).\r\n", (UINTN)Controller));

  // Stop() is called with the child handle and then again without it. Everything is released on the first call.
  //
  Status = StopRenderingEngine (This);

  DEBUG ((DEBUG_INFO, "INFO [SRE]: Driver stop Exit (%r).\r\n", Status));

  return Status;
}

/**
    Change Deactivate to ignore the original background, and only paint black at PreExitBootServices.

    @param    Event           Not Used.
    @param    Context         Not Used.

   @retval   none
 **/
VOID
EFIAPI
OnPreExitBootServicesNotification (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mPreExitBootServices = TRUE;
  gBS->CloseEvent (Event);
  mPreExitBootServicesEvent = NULL;
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
DriverInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  // Save the image handle for later.
  //
  mImageHandle = ImageHandle;

  // Install the Driver Binding Protocol.
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             gST,
             &mSREDriverBinding,
             ImageHandle,
             NULL,
             NULL
             );

  ASSERT_EFI_ERROR (Status);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Register notify function to modify any window deactivate to a black background at PreExitBootServices.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  OnPreExitBootServicesNotification,
                  gImageHandle,
                  &gEfiEventBeforeExitBootServicesGuid,
                  &mPreExitBootServicesEvent
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Create Event Ex for ExitBootServices. Code = %r\n", __FUNCTION__, Status));

    // The image is freed when the entry point fails so the driver binding can't stay installed.
    //
    gBS->UninstallMultipleProtocolInterfaces (
           ImageHandle,
           &gEfiDriverBindingProtocolGuid,
           &mSREDriverBinding,
           NULL
           );
  }

  return Status;
}

/**
  Driver unload handler.

  @param[in] ImageHandle    Image handle this driver.

  @retval EFI_SUCCESS       The driver was unloaded.

**/
EFI_STATUS
EFIAPI
DriverUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  // Stop managing the controller so the timer, surfaces and parent GOP are released.
  //
  if (mParentHandle != NULL) {
    Status = gBS->DisconnectController (mParentHandle, ImageHandle, NULL);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ImageHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &mSREDriverBinding,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (mPreExitBootServicesEvent != NULL) {
    gBS->CloseEvent (mPreExitBootServicesEvent);
    mPreExitBootServicesEvent = NULL;
  }

  return Status;
}
