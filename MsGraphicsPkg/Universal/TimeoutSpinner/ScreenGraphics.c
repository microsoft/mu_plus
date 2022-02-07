/** @file ScreenGraphics.c

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  UEFI Recovery On screen Graphics
  This makes it easy to control and customize the on screen notifications for UEFI recovery.

  If desired this can be moved to a library where all modules can share this logic.

**/

#include <PiDxe.h>

#include "ScreenGraphics.h"

STATIC CONST UINTN                   gStep            = 100 / STEPS_PER_ROTATION;
STATIC EFI_GRAPHICS_OUTPUT_PROTOCOL  *gGraphicsOutput = NULL;

/**
 GetBitmapFromFile

  Load Bitmap into Struct

  @param  FileGuid   - GUID name of bitmap file in FV
  @param  Toc        - Timeout Container
**/
EFI_STATUS
EFIAPI
GetBitmapFromFile (
  IN SPINNER_CONTAINER  *Spc

  )
{
  EFI_STATUS         Status;
  UINTN              BMPDataSize = 0;
  UINT8              *BMPData    = NULL;
  TIMEOUT_CONTAINER  *Toc;

  // Get the specified image from FV.
  //
  Status = GetSectionFromAnyFv (
             Spc->Icon,
             EFI_SECTION_RAW,
             0,
             (VOID **)&BMPData,
             &BMPDataSize
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to find bitmap file (GUID=%g) (%r).\r\n", __FUNCTION__, Spc->Icon, Status));
    goto Cleanup;
  }

  Toc = Spc->Toc;
  // Convert the bitmap from BMP format to a GOP frame buffer-compatible form.
  //
  Status = TranslateBmpToGopBlt (
             BMPData,
             BMPDataSize,
             &Toc->Icon->BitmapData,
             &Toc->Icon->BitmapBufferSize,
             &Toc->Icon->Height,
             &Toc->Icon->Width
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to convert bitmap file to GOP format (%r).\r\n", __FUNCTION__, Status));
    goto Cleanup;
  }

Cleanup:
  if (BMPData != NULL) {
    FreePool (BMPData);
  }

  return Status;
}

/**
  DisplayBitMap

  Render the BitMap on the display

  @param   Toc                   - Timeout Container
  @return  EFI_SUCCESS           - Bitmap rendered on display
           EFI_INVALID_PARAMETER - Invalid parameters
**/
EFI_STATUS
EFIAPI
DisplayBitmap (
  IN TIMEOUT_CONTAINER  *Toc
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  // Verify icon exists
  if (Toc == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Toc->Icon == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Render the bitmap into the pixel buffer
  if (gGraphicsOutput != NULL) {
    Status = gGraphicsOutput->Blt (
                                gGraphicsOutput,
                                Toc->Icon->BitmapData,
                                EfiBltBufferToVideo,
                                0,
                                0,
                                Toc->Icon->UpperLeft.X,
                                Toc->Icon->UpperLeft.Y,
                                Toc->Icon->Width,
                                Toc->Icon->Height,
                                0
                                );
  }

  return Status;
}

/**
  CaptureOriginalBackground

  Grab the original background rectangle using GOP before drawing progress spinner

  @param   Timeout Container

  @retval  EFI_SUCCESS            - Rectangle saved
           EFI_INVALID_PARAMETER  - Invalid parameter
           EFI_OUT_OF_RESOURCES   - Allocation failed
           Other                  - Blt operation failed
**/
EFI_STATUS
EFIAPI
CaptureOriginalBackground (
  IN TIMEOUT_CONTAINER  *Toc
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINTN       SquareSize;

  // Verify necessary structure exist
  if (Toc == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Toc->Spinner == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SquareSize = sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) *
               (Toc->Spinner->OuterRadius * 2 + 1) *
               (Toc->Spinner->OuterRadius * 2 + 1);

  // Create Pixel Buffer
  Toc->OriginalSquare = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)AllocateZeroPool (SquareSize);

  if (Toc->OriginalSquare == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Capture the initial square from the video out
  if (gGraphicsOutput != NULL) {
    Status = gGraphicsOutput->Blt (
                                gGraphicsOutput,
                                Toc->OriginalSquare,
                                EfiBltVideoToBltBuffer,
                                Toc->Spinner->Origin.X-Toc->Spinner->OuterRadius,
                                Toc->Spinner->Origin.Y-Toc->Spinner->OuterRadius,
                                0,
                                0,
                                Toc->Spinner->OuterRadius*2+1,
                                Toc->Spinner->OuterRadius*2+1,
                                0
                                );
  }

  return Status;
}

/**
  RestoreBackground

  Restore background to previous state using stored square

  @param   Toc     - Timeout Container

  @retval  EFI_SUCCESS           - Background restored
           EFI_INVALID_PARAMETER - Invalid parameter
           Other                 - Error during Blt operation
**/
EFI_STATUS
EFIAPI
RestoreBackground (
  IN TIMEOUT_CONTAINER  *Toc
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  // Verify original data exists
  if (Toc == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Toc->OriginalSquare == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Restore initial video out square from stored pixel buffer
  if (gGraphicsOutput != NULL) {
    Status = gGraphicsOutput->Blt (
                                gGraphicsOutput,
                                Toc->OriginalSquare,
                                EfiBltBufferToVideo,
                                0,
                                0,
                                Toc->Spinner->Origin.X-Toc->Spinner->OuterRadius,
                                Toc->Spinner->Origin.Y-Toc->Spinner->OuterRadius,
                                Toc->Spinner->OuterRadius*2+1,
                                Toc->Spinner->OuterRadius*2+1,
                                0
                                );
  }

  return Status;
}

/**
  FreeSpinnerMemory

  Free memory allocated for pixel buffers and structures

  @param  Toc  - Timeout Container

  @retval NONE
**/
VOID
EFIAPI
FreeSpinnerMemory (
  IN TIMEOUT_CONTAINER  *Toc
  )
{
  // Free All Allocated Pools
  if (Toc != NULL) {
    if (Toc->Icon != NULL) {
      if (Toc->Icon->BitmapData != NULL) {
        FreePool (Toc->Icon->BitmapData);
        Toc->Icon->BitmapData = NULL;
      }

      FreePool (Toc->Icon);
      Toc->Icon = NULL;
    }

    if (Toc->Spinner != NULL) {
      delete_ProgressCircle (Toc->Spinner->pc);
      FreePool (Toc->Spinner);
      Toc->Spinner = NULL;
    }

    FreePool (Toc);
  }
}

/**
  SetupTimeoutContainer

  Initialize everything needed for timeout spinner

  @param   Spc      - Spinner Container

  @retval  EFI_SUCCESS            Timer container setup completed normally
           EFI_OUT_OF_RESOURCES   Out of memory error
           EFI_INVALID_PARAMETER  Error in input parameters

**/
EFI_STATUS
EFIAPI
SetupTimeoutContainer (
  IN SPINNER_CONTAINER  *Spc
  )
{
  EFI_STATUS         Status;
  TIMEOUT_CONTAINER  *Toc;

  if (gGraphicsOutput == NULL) {
    Status = gBS->LocateProtocol (
                    &gEfiGraphicsOutputProtocolGuid,
                    NULL,
                    (VOID **)&gGraphicsOutput
                    );

    if (EFI_ERROR (Status)) {
      gGraphicsOutput = NULL;
      DEBUG ((DEBUG_ERROR, "%a: Error %r locating GOP\n", __FUNCTION__, Status));
      return Status;
    }
  }

  // Allocate Container
  Toc = (TIMEOUT_CONTAINER *)AllocateZeroPool (sizeof (TIMEOUT_CONTAINER));
  if (NULL == Toc) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory pool for timeout container\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto Error;
  }

  Spc->Toc = Toc;

  // Allocate structure for Bitmap Icon
  Toc->Icon = (TIMEOUT_ICON *)AllocateZeroPool (sizeof (TIMEOUT_ICON));
  if (NULL == Toc->Icon) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory pool for timeout icon\n", __FUNCTION__));
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  // Allocate Structure for Progress Spinner
  Toc->Spinner = (TIMEOUT_SPINNER *)AllocateZeroPool (sizeof (TIMEOUT_SPINNER));
  if (Toc->Spinner == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory pool for timeout container\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto Error;
  }

  // Load Bitmap from File
  Status = GetBitmapFromFile (Spc);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get IconFile. Code=%r\n", __FUNCTION__, Status));
    goto Error;
  }

  // Calculate Outer Radius, Inner Radius based on the size of the Icon
  Toc->Spinner->OuterRadius = (UINT16)MIN (Toc->Icon->Width, Toc->Icon->Height);
  Toc->Spinner->InnerRadius = Toc->Spinner->OuterRadius * 85 / 100;

  // Calculate the Icon position on the screen based on location requested
  switch (Spc->Location) {
    case Location_LR_Corner:
      Toc->Icon->UpperLeft.X = gGraphicsOutput->Mode->Info->HorizontalResolution - (Toc->Icon->Width / 2) - Toc->Spinner->OuterRadius - 1;
      Toc->Icon->UpperLeft.Y = gGraphicsOutput->Mode->Info->VerticalResolution - (Toc->Icon->Height / 2) - Toc->Spinner->OuterRadius - 1;
      break;

    case Location_LL_Corner:
      Toc->Icon->UpperLeft.X = Toc->Spinner->OuterRadius - (Toc->Icon->Width / 2);
      Toc->Icon->UpperLeft.Y = gGraphicsOutput->Mode->Info->VerticalResolution - (Toc->Icon->Height / 2) - Toc->Spinner->OuterRadius - 1;
      break;

    case Location_UR_Corner:
      Toc->Icon->UpperLeft.X = gGraphicsOutput->Mode->Info->HorizontalResolution - (Toc->Icon->Width / 2) -  Toc->Spinner->OuterRadius - 1;
      Toc->Icon->UpperLeft.Y = Toc->Spinner->OuterRadius - (Toc->Icon->Height / 2);
      break;

    case Location_UL_Corner:
      Toc->Icon->UpperLeft.X = Toc->Spinner->OuterRadius - (Toc->Icon->Width / 2);
      Toc->Icon->UpperLeft.Y = Toc->Spinner->OuterRadius - (Toc->Icon->Height / 2);
      break;

    case Location_Center:
      Toc->Icon->UpperLeft.X = (gGraphicsOutput->Mode->Info->HorizontalResolution / 2) - (Toc->Icon->Width / 2)  - 1;
      Toc->Icon->UpperLeft.Y = (gGraphicsOutput->Mode->Info->VerticalResolution / 2) - (Toc->Icon->Height / 2) - 1;
      break;
  }

  // Calculate the Spinner origin (center of circle) within the Icon
  Toc->Spinner->Origin.X = Toc->Icon->UpperLeft.X + Toc->Icon->Width / 2;
  Toc->Spinner->Origin.Y = Toc->Icon->UpperLeft.Y + Toc->Icon->Height / 2;

  DEBUG ((DEBUG_INFO, "%a: Icon Location  X=%d, Y=%d\n", __FUNCTION__, Toc->Icon->UpperLeft.X, Toc->Icon->UpperLeft.Y));

  Status = CaptureOriginalBackground (Toc);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: Cannot capture original background. Code=%r\n", __FUNCTION__, Status));
    goto Error;
  }

  Status = UpdateSpinnerGraphic (Toc);    // Don't wait for a timer tick to display the first image

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Initial Graphics Update failed. Code = %r\n", __FUNCTION__, Status));
    goto Error;
  }

  Spc->Toc = Toc;

  return Status;

Error:
  FreeSpinnerMemory (Toc);
  return Status;
}

/**
  UpdateSpinnerGraphic

  Initial spinner drawing and progress update

  @param   Toc  - Timeout Container

  @retval EFI_SUCCESS           - Spinner updated
          EFI_INVALID_PARAMETER - Invalid parameter

**/
EFI_STATUS
EFIAPI
UpdateSpinnerGraphic (
  IN TIMEOUT_CONTAINER  *Toc
  )
{
  UINTN       j;
  EFI_STATUS  Status;

  // Verify containers exists
  if (Toc == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Toc->Icon == NULL) || (Toc->Spinner == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // If First iteration, i.e. ProgressCircle hasn't been initialized yet
  if (Toc->Spinner->pc == NULL) {
    // Draw Icon
    Status = DisplayBitmap (Toc);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Get Frame buffer and pixels per scan line
    UINT8  *fb  = (UINT8 *)((UINTN)(gGraphicsOutput->Mode->FrameBufferBase));
    UINTN  ppsl = (UINTN)(gGraphicsOutput->Mode->Info->PixelsPerScanLine);

    // Create Spinner
    Toc->Spinner->pc = new_ProgressCircle (
                         &Toc->Spinner->Origin,
                         fb,
                         ppsl,
                         Toc->Spinner->InnerRadius,
                         Toc->Spinner->OuterRadius
                         );

    if (Toc->Spinner->pc == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    DrawAll (Toc->Spinner->pc, BACKGROUND_COLOR);

    // Initialize Step
    Toc->Spinner->CurrentStep = gStep;
  }

  // Update Progress Circle
  for (j = Toc->Spinner->CurrentStep; j < Toc->Spinner->CurrentStep + (gStep*BAR_LENGTH_COEFFICIENT); j++) {
    DrawSegment (Toc->Spinner->pc, (UINT8)(j % 100)+1, SPINNER_COLOR);
  }

  // clear old step
  for (j = Toc->Spinner->CurrentStep-gStep; j < Toc->Spinner->CurrentStep; j++) {
    DrawSegment (Toc->Spinner->pc, (UINT8)(j % 100)+1, BACKGROUND_COLOR);
  }

  Toc->Spinner->CurrentStep += gStep;

  return EFI_SUCCESS;
}
