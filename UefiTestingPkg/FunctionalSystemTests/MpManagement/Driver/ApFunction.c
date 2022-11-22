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
#include <Library/IoLib.h>
#include <Library/ArmGicLib.h>
#include <Library/ArmLib.h>
#include <Library/CpuExceptionHandlerLib.h>

#include "MpManagementInternal.h"

EFI_STATUS
EFIAPI
CpuArchHalt (
  VOID
  );

EFI_STATUS
EFIAPI
CpuArchClockGate (
  UINTN         PowerState
  );

EFI_STATUS
EFIAPI
CpuArchSleep (
  UINTN         PowerState
  );

EFI_STATUS
SetupResumeContext (
  IN  UINTN       CpuIndex
  );
#define GICR_WAKER 0x14
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
  BASE_LIBRARY_JUMP_BUFFER          JumpBuffer;

  // First figure who am i.
  Status = mMpServices->WhoAmI (mMpServices, &ProcessorId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot even figure who am I... Bail here - %r\n", Status));
    goto Done;
  }

  // Initially start, populate greeting message
  MyBuffer            = &mCommonBuffer[ProcessorId];
  //
  // The initial call to SetJump() must always return 0.
  // Subsequent calls to LongJump() cause a non-zero value to be returned by SetJump().
  //
  if (SetJump (&JumpBuffer)) {
    // Got back from the C-states, do some clean up.
    // RegisterMemoryProfileImage (Image, (Image->ImageContext.ImageType == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION ? EFI_FV_FILETYPE_APPLICATION : EFI_FV_FILETYPE_DRIVER));
    // //
    // // Call the image's entry point
    // //
    // Image->Started = TRUE;
    // Image->Status  = Image->EntryPoint (ImageHandle, Image->Info.SystemTable);

    // //
    // // If the image returns, exit it through Exit()
    // //
    // CoreExit (ImageHandle, Image->Status, 0, NULL);
  }

  MyBuffer->ApStatus  = AP_STATE_ON;

  UINT32 waker = MmioRead32(PcdGet64 (PcdGicRedistributorsBase) + GICR_WAKER);
  UINT32 IccSre = ArmGicV3GetControlSystemRegisterEnable ();
  Status = InitializeCpuExceptionHandlers (NULL);
  UINTN vbar = ArmReadVBar ();
  
  UINT32 grp0 = MmioRead32 (PcdGet64 (PcdGicRedistributorsBase) + BASE_64KB + 0x80);

  UINT32 grpm0 = MmioRead32 (PcdGet64 (PcdGicRedistributorsBase) + BASE_64KB + 0xd00);

  AsciiSPrint (MyBuffer->ApBuffer, MyBuffer->ApBufferSize, "Hello from CPU %ld waker: %x iccsre: %x vbar: %llx %r %x %x\n", ProcessorId, waker, IccSre, vbar, Status, grp0, grpm0);

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
        SetupResumeContext (ProcessorId);
        AsciiSPrint (MyBuffer->ApBuffer, MyBuffer->ApBufferSize, "See you later - CPU %ld.\n", ProcessorId);
        MyBuffer->ApStatus  = AP_STATE_SUSPEND_HALT;
        MyBuffer->ApTask    = AP_TASK_IDLE;
        // This call will not return from here
        Status = CpuArchHalt ();
        if (EFI_ERROR (Status)) {
          // if we ever return from this power level, something is off.
          AsciiSPrint (MyBuffer->ApBuffer, MyBuffer->ApBufferSize, "CPU %ld failed to clock gate, and it is off now.\n", ProcessorId);
          BreakLoop = TRUE;
        } else {
          // Recover from the previously saved jump buffer.
          LongJump (&JumpBuffer, 1);
        }
        break;
      case AP_STATE_SUSPEND_CLOCK_GATE:
        // SetupResumeContext (ProcessorId);
        AsciiSPrint (MyBuffer->ApBuffer, MyBuffer->ApBufferSize, "Siesta time - CPU %ld.\n", ProcessorId);
        MyBuffer->ApStatus  = AP_STATE_SUSPEND_CLOCK_GATE;
        MyBuffer->ApTask    = AP_TASK_IDLE;
        // TODO: Register interrupt here?
        Status = CpuArchClockGate (MyBuffer->TargetPowerState);
        if (EFI_ERROR (Status)) {
          // if we ever return from this power level, something is off.
          AsciiSPrint (MyBuffer->ApBuffer, MyBuffer->ApBufferSize, "CPU %ld failed to clock gate, and it is off now.\n", ProcessorId);
          BreakLoop = TRUE;
        }
        break;
      case AP_STATE_SUSPEND_SLEEP:
        // SetupResumeContext (ProcessorId);
        AsciiSPrint (MyBuffer->ApBuffer, MyBuffer->ApBufferSize, "Good night - CPU %ld.\n", ProcessorId);
        MyBuffer->ApStatus  = AP_STATE_SUSPEND_SLEEP;
        MyBuffer->ApTask    = AP_TASK_IDLE;
        // TODO: Register interrupt here?
        Status = CpuArchSleep (MyBuffer->TargetPowerState);
        if (EFI_ERROR (Status)) {
          // if we ever return from this power level, something is off.
          AsciiSPrint (MyBuffer->ApBuffer, MyBuffer->ApBufferSize, "CPU %ld failed to sleep, and it is off now.\n", ProcessorId);
          BreakLoop = TRUE;
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
  MyBuffer->ApStatus  = AP_STATE_OFF;
  MyBuffer->ApTask    = AP_TASK_IDLE;
  return;
}
