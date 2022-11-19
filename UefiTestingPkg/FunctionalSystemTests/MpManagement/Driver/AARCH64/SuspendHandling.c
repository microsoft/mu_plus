/** @file
  TODO: Populate this.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/ArmLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Library/ArmGicLib.h>
#include <Library/ArmSmcLib.h>

#include <IndustryStandard/ArmStdSmc.h>
#include <Pi/PiMultiPhase.h>
#include <Protocol/MpService.h>

#include "MpManagementInternal.h"

#define INTERRUPT_SOURCE_NUMBER     0x08

EFI_STATUS
SetupResumeContext (
  IN  UINTN       CpuIndex
  )
{
  UINT64      CpuTarget;
  UINT64      MpId;

  // Route the interrupt to this CPU
  MpId      = ArmReadMpidr ();
  CpuTarget = MpId &
              (ARM_CORE_AFF0 | ARM_CORE_AFF1 | ARM_CORE_AFF2 | ARM_CORE_AFF3);

  // Route this SGI to the primary CPU. SPIs start at the INTID 32
  MmioWrite64 (
    PcdGet64 (PcdGicDistributorBase) + ARM_GICD_IROUTER + (INTERRUPT_SOURCE_NUMBER * 8),
    (UINT32)CpuTarget
    );

  // Enable the intended interrupt source
  ArmGicEnableInterrupt ((UINT32)PcdGet64 (PcdGicDistributorBase), (UINT32)PcdGet64 (PcdGicRedistributorsBase), INTERRUPT_SOURCE_NUMBER);

  return EFI_SUCCESS;
}

VOID
EFIAPI
CpuArchWakeFromSleep (
  UINTN   CpuIndex
  )
{
  // Sending SGI to the specified secondary CPU interfaces
  ArmGicSendSgiTo (PcdGet64 (PcdGicDistributorBase), ARM_GIC_ICDSGIR_FILTER_TARGETLIST, 0xFF, INTERRUPT_SOURCE_NUMBER);
}

VOID
EFIAPI
CpuArchHalt (
  VOID
  )
{
  ArmCallWFI ();
}

EFI_STATUS
EFIAPI
CpuArchClockGate (
  IN UINTN         PowerLevel
  )
{
  ARM_SMC_ARGS  Args;
  EFI_STATUS    Status;

  Status = EFI_SUCCESS;

  /* Turn the AP on */
  if (sizeof (Args.Arg0) == sizeof (UINT32)) {
    Args.Arg0 = ARM_SMC_ID_PSCI_CPU_SUSPEND_AARCH32;
  } else {
    Args.Arg0 = ARM_SMC_ID_PSCI_CPU_SUSPEND_AARCH64;
  }

  // Parameter for power_state
  Args.Arg1 = PowerLevel;
  // Parameter for entrypoint, only need for powerdown state
  Args.Arg2 = 0;
  // Parameter for context_id, only need for powerdown state
  Args.Arg3 = 0;

  ArmCallSmc (&Args);

  if (Args.Arg0 != ARM_SMC_PSCI_RET_SUCCESS) {
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

EFI_STATUS
EFIAPI
CpuArchSleep (
  IN UINTN         PowerLevel
  )
{
  return CpuArchClockGate (PowerLevel);
}
