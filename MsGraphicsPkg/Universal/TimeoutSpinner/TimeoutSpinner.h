/** @file TimeoutSpinner.h

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __TIMEOUT_SPINNER_H__
#define __TIMEOUT_SPINNER_H__

#include <Uefi.h>

#include <Guid/NVMeEventGroup.h>
#include <Guid/SpinnerEventGroup.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "ScreenGraphics.h"

#include "TimeoutGeneral.h"

#define SPINNER_TICK_RATE      2000000 // 200 milliseconds
#define TIME_TO_SPINNER       50000000 // 5 Seconds

/**
 * Start Spinner Common routine
 *
 * @param Spinner
 * @param IconFile
 *
 * @return VOID
 */
VOID
StartSpinnerCommon (
  IN SPINNER_CONTAINER *Spc
  );

/**
 * Stop Spinner Common routine
 *
 * @param Spc
 *
 * @return VOID
 */
VOID
StopSpinnerCommon (
  IN SPINNER_CONTAINER *Spc
  );


#endif
