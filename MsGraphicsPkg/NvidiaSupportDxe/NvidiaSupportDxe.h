/** @file NvidiaSupportDxe.h
    This is a driver for working around a specific defect in the Nvidia Graphics Output Protocol.

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _NVIDIA_SUPPORT_H_
#define _NVIDIA_SUPPORT_H_

#include <PiDxe.h>

#include <Protocol/GraphicsOutput.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FrameBufferBltLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/SafeIntLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define MOUSE_POINTER_WIDTH_MEDIUM  30
#define MOUSE_POINTER_WIDTH_SMALL   12

#define TELEMETRY_DELAY  50000000  // 5 Seconds

typedef struct {
  UINT32    Mode;
  UINT32    HorizontalResolution;
  UINT32    VerticalResolution;
  UINT32    PixelsPerScanLine;
  UINT8     ConfigureBuffer[];
} FB_CONFIGURE_INFO;

#endif // _NVIDIA_SUPPORT_H_
