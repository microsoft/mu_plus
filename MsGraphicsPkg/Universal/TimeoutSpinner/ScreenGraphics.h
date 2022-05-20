/** @file ScreenGraphics.h

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef   TIMEOUT_ON_SCREEN_GRAPHICS_H
#define   TIMEOUT_ON_SCREEN_GRAPHICS_H

#include <UiPrimitiveSupport.h>

#include <Protocol/GraphicsOutput.h>

#include <Library/BaseMemoryLib.h>
#include <Library/BmpSupportLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UiProgressCircleLib.h>

#define SPINNER_COLOR           (0xF4BF42)
#define BACKGROUND_COLOR        (0xFF373a36)
#define STEPS_PER_ROTATION      (25)
#define BAR_LENGTH_COEFFICIENT  (4)

typedef enum {
  Location_LR_Corner = 1,
  Location_LL_Corner = 2,
  Location_UR_Corner = 3,
  Location_UL_Corner = 4,
  Location_Center    = 5
} SPINNER_LOCATION;

typedef struct {
  UINTN             CurrentStep;
  UINT16            InnerRadius;
  UINT16            OuterRadius;
  POINT             Origin;
  ProgressCircle    *pc;
} TIMEOUT_SPINNER;

typedef struct {
  POINT                            UpperLeft;
  UINTN                            Width;
  UINTN                            Height;
  UINTN                            BitmapBufferSize;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *BitmapData;
} TIMEOUT_ICON;

typedef struct {
  TIMEOUT_ICON                     *Icon;
  TIMEOUT_SPINNER                  *Spinner;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *OriginalSquare;
} TIMEOUT_CONTAINER;

/**
 * Initialize Nvme Spinner
 *
 * @param
 *
 * @return EFI_STATUS
 */
EFI_STATUS
InitializeNvmeSpinner (
  VOID
  );

typedef
EFI_STATUS
(*INITIALIZE_SPINNER)(
  IN TIMEOUT_CONTAINER  *Toc
  );

typedef enum {
  Standard = 1,         // Immediate Spinner
  Delay    = 2          // Delayed Spinner
} SPINNER_TYPE;

typedef struct {
  TIMEOUT_CONTAINER    *Toc;
  EFI_GUID             *StartEventGuid;
  EFI_GUID             *StopEventGuid;
  EFI_GUID             *Icon;
  UINTN                Id;

  SPINNER_LOCATION     Location;
  SPINNER_TYPE         Type;

  EFI_EVENT            StartEvent;
  EFI_EVENT            StopEvent;
  EFI_EVENT            DelayEvent;

  UINT32               IconFileToken;
  UINT32               SpinnerTypeToken;
  UINT32               SpinnerLocationToken;
} SPINNER_CONTAINER;

EFI_STATUS
EFIAPI
CaptureOriginalBackground (
  IN TIMEOUT_CONTAINER *
  );

EFI_STATUS
EFIAPI
SetupTimeoutContainer (
  IN SPINNER_CONTAINER  *Spc
  );

EFI_STATUS
EFIAPI
RestoreBackground (
  IN TIMEOUT_CONTAINER  *Toc
  );

EFI_STATUS
EFIAPI
DisplayBitmap (
  IN TIMEOUT_CONTAINER  *Toc
  );

VOID
EFIAPI
FreeSpinnerMemory (
  IN TIMEOUT_CONTAINER  *Toc
  );

EFI_STATUS
EFIAPI
UpdateSpinnerGraphic (
  IN TIMEOUT_CONTAINER  *Toc
  );

#endif
