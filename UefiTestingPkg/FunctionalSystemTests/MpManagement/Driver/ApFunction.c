/** @file
  TODO: Populate this.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Pi/PiMultiPhase.h>
#include <Protocol/MpService.h>

#include "MpManagementInternal.h"

/** The procedure to run with the MP Services interface.

  @param Arg The procedure argument.

**/
VOID
EFIAPI
ApFunction (
  IN OUT VOID  *Arg
  )
{
  EFI_STATUS                        Status;
  UINTN                             ProcessorId;
  BOOLEAN                           BreakLoop;
  volatile MP_MANAGEMENT_METADATA   *MyBuffer;

  // First figure who am i.
  Status = mMpServices->WhoAmI (mMpServices, &ProcessorId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot even figure who am I... Bail here - %r\n", Status));
    goto Done;
  }

  // Initially start, populate greeting message
  MyBuffer            = &mCommonBuffer[ProcessorId];
  MyBuffer->ApStatus  = AP_STATE_ON;
  AsciiSPrint (MyBuffer->ApBuffer, MyBuffer->ApBufferSize, "Hello from CPU %ld\n", ProcessorId);

  // Clear the active state
  MyBuffer->ApTask    = AP_TASK_IDLE;

  BreakLoop = FALSE;
  while (TRUE) {
    if (MyBuffer->ApTask != AP_TASK_ACTIVE) {
      continue;
    }

    switch (MyBuffer->TargetStatus) {
      case AP_STATE_OFF:
        // Easy operation, just exit here
        BreakLoop = TRUE;
        break;
      case AP_STATE_SUSPEND:
        break;
      default:
        ASSERT (FALSE);
        break;
    }

    if (BreakLoop) {
      break;
    }
  }

Done:
  MyBuffer->ApStatus  = AP_STATE_OFF;
  MyBuffer->ApTask    = AP_TASK_IDLE;
  return;
}
