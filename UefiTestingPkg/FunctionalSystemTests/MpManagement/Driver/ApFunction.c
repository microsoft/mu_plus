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

  This routine manages the state machine of APs, checking
  and acknowledging BSP commands.

  The exit of this routine will leave to CPU power off.

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

  Status = SetupInterruptStatus (ProcessorId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot setup interrupt status properly... Bail here - %r\n", Status));
    goto Done;
  }

  // Initially start, populate greeting message
  MyBuffer            = &mCommonBuffer[ProcessorId];

  // Setup a long jump buffer so that the cores can come back to the same place after resuming.
  if (SetJump ((BASE_LIBRARY_JUMP_BUFFER*)(&(MyBuffer->JumpBuffer)))) {
    // Got back from the C-states, do some common clean up.
    CpuArchResumeCommon (ProcessorId);
  }

  MyBuffer->ApStatus  = AP_STATE_ON;
  AsciiSPrint (MyBuffer->ApBuffer, MyBuffer->ApBufferSize, "Hello from CPU %ld!\n", ProcessorId);

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
        AsciiSPrint (MyBuffer->ApBuffer, MyBuffer->ApBufferSize, "CPU %ld says bye.\n", ProcessorId);
        BreakLoop = TRUE;
        break;
      case AP_STATE_SUSPEND_HALT:
        AsciiSPrint (MyBuffer->ApBuffer, MyBuffer->ApBufferSize, "See you later - CPU %ld.\n", ProcessorId);
        MyBuffer->ApStatus  = AP_STATE_SUSPEND_HALT;
        MyBuffer->ApTask    = AP_TASK_IDLE;
        Status = CpuArchHalt ();
        if (EFI_ERROR (Status)) {
          // if we ever return from this power level, something is off.
          AsciiSPrint (MyBuffer->ApBuffer, MyBuffer->ApBufferSize, "CPU %ld failed to clock gate, and it is off now.\n", ProcessorId);
          BreakLoop = TRUE;
        } else {
          // Recover from the previously saved jump buffer.
          LongJump ((BASE_LIBRARY_JUMP_BUFFER*)(&(MyBuffer->JumpBuffer)), 1);
        }
        break;
      case AP_STATE_SUSPEND_CLOCK_GATE:
        AsciiSPrint (MyBuffer->ApBuffer, MyBuffer->ApBufferSize, "Siesta time - CPU %ld.\n", ProcessorId);
        MyBuffer->ApStatus  = AP_STATE_SUSPEND_CLOCK_GATE;
        MyBuffer->ApTask    = AP_TASK_IDLE;
        Status = CpuArchClockGate (MyBuffer->TargetPowerState);
        if (EFI_ERROR (Status)) {
          // if we ever return from this power level, something is off.
          AsciiSPrint (MyBuffer->ApBuffer, MyBuffer->ApBufferSize, "CPU %ld failed to enter stand by, and it is off now.\n", ProcessorId);
          BreakLoop = TRUE;
        } else {
          // Recover from the previously saved jump buffer.
          LongJump ((BASE_LIBRARY_JUMP_BUFFER*)(&(MyBuffer->JumpBuffer)), 1);
        }
        break;
      case AP_STATE_SUSPEND_SLEEP:
        AsciiSPrint (MyBuffer->ApBuffer, MyBuffer->ApBufferSize, "Good night - CPU %ld.\n", ProcessorId);
        MyBuffer->ApStatus  = AP_STATE_SUSPEND_SLEEP;
        MyBuffer->ApTask    = AP_TASK_IDLE;
        Status = CpuArchSleep (MyBuffer->TargetPowerState);
        if (EFI_ERROR (Status)) {
          // if we ever return from this power level, something is off.
          AsciiSPrint (MyBuffer->ApBuffer, MyBuffer->ApBufferSize, "CPU %ld failed to sleep, and it is off now.\n", ProcessorId);
          BreakLoop = TRUE;
        } else {
          // This does not make much sense, C3 sleep should not come back here.
          ASSERT (FALSE);
        }
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
  RestoreInterruptStatus (ProcessorId);

  MyBuffer->ApStatus  = AP_STATE_OFF;
  MyBuffer->ApTask    = AP_TASK_IDLE;

  return;
}
