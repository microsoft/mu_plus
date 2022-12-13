/** @file
  TODO: Populate this.

  Copyright (c) 2013-2020, ARM Limited and Contributors. All rights reserved.
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
#include <Library/HobLib.h>
#include <Library/ArmGicLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/ArmGenericTimerCounterLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <IndustryStandard/ArmStdSmc.h>
#include <Pi/PiMultiPhase.h>
#include <Protocol/MpService.h>
#include <Protocol/Timer.h>
#include <Guid/ArmMpCoreInfo.h>

#include "MpManagementInternal.h"

#define FF_PSTATE_SHIFT           1
#define FF_PSTATE_ORIG            0
#define FF_PSTATE_EXTENDED        1

/* Features flags for CPU SUSPEND OS Initiated mode support. Bits [0:0] */
#define FF_MODE_SUPPORT_SHIFT     0
#define FF_SUPPORTS_OS_INIT_MODE  1

#define FF_SUSPEND_MASK            ((1 << FF_PSTATE_SHIFT) | (1 << FF_MODE_SUPPORT_SHIFT))

#define PSTATE_TYPE_SHIFT_EX      30
#define PSTATE_TYPE_SHIFT_ORIG    16
#define PSTATE_TYPE_MASK          1
#define PSTATE_TYPE_STANDBY       0x0
#define PSTATE_TYPE_POWERDOWN     0x1

typedef struct {
  VOID                         *Ttbr0;
  UINTN                        Tcr;
  UINTN                        Mair;
} AARCH64_AP_BUFFER;

VOID
RegisterEl0Stack (
  IN  VOID  *Stack
  );

UINTN
ReadEl0Stack (
  VOID
  );

BOOLEAN         mExtendedPowerState = FALSE;
ARM_CORE_INFO   *mCpuInfo           = NULL;
UINTN           mBspVbar               = 0;
UINTN           mBspHcrReg             = 0;
UINTN           mBspEl0Sp              = 0;

/**
  EFI_CPU_INTERRUPT_HANDLER that is called when a processor interrupt occurs.

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor. This parameter is
                           processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.

  @return None

**/
VOID
EFIAPI
ApIrqInterruptHandler (
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  UINTN IntValue;

  IntValue = ArmGicV3AcknowledgeInterrupt ();
  if ((IntValue) != PcdGet32 (PcdGicSgiIntId)) {
    // Some other spurious interrupts should do not happen
    return;
  }
  ArmGicV3EndOfInterrupt (IntValue);
}

EFI_STATUS
CpuMpArchInit (
  IN UINTN        NumOfCpus
  )
{
  ARM_SMC_ARGS                Args;
  EFI_STATUS                  Status;
  UINTN                       Index;
  UINTN                       MaxCpus;
  EFI_HOB_GENERIC_HEADER      *Hob;
  VOID                        *HobData;
  UINTN                       HobDataSize;

  Status = EFI_SUCCESS;

  /* Query the suspend feature flags during init steps */
  Args.Arg0 = ARM_SMC_ID_PSCI_FEATURES;

  if (sizeof (Args.Arg1) == sizeof (UINT32)) {
    Args.Arg1 = ARM_SMC_ID_PSCI_CPU_SUSPEND_AARCH32;
  } else {
    Args.Arg1 = ARM_SMC_ID_PSCI_CPU_SUSPEND_AARCH64;
  }

  ArmCallSmc (&Args);

  if (Args.Arg0 & (~FF_SUSPEND_MASK)) {
    Status = EFI_DEVICE_ERROR;
    DEBUG ((DEBUG_ERROR, "%a Query suspend feature flags failed - %x\n", __FUNCTION__, Args.Arg0));
    goto Done;
  }

  mExtendedPowerState = (((Args.Arg0 >> FF_PSTATE_SHIFT) & 1) == FF_PSTATE_EXTENDED);

  /* Prepare the architectural specific buffer */
  for (Index = 0; Index < NumOfCpus; Index ++) {
    mCommonBuffer[Index].CpuArchBuffer = AllocatePool (sizeof (AARCH64_AP_BUFFER));
    if (mCommonBuffer[Index].CpuArchBuffer == NULL) {
      DEBUG ((DEBUG_ERROR, "%a Running out of memory when allocating for core %d\n", __FUNCTION__, Index));
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }
  }

  /* Prepare the architectural specific CPU info from the hob */
  Hob = GetFirstGuidHob (&gArmMpCoreInfoGuid);
  if (Hob != NULL) {
    HobData     = GET_GUID_HOB_DATA (Hob);
    HobDataSize = GET_GUID_HOB_DATA_SIZE (Hob);
    mCpuInfo    = (ARM_CORE_INFO *)HobData;
    MaxCpus     = HobDataSize / sizeof (ARM_CORE_INFO);
  }

  if (MaxCpus != NumOfCpus) {
    DEBUG ((DEBUG_WARN, "Trying to use EFI_MP_SERVICES_PROTOCOL on a UP system"));
    // We are not MP so nothing to do
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  Status = EFI_SUCCESS;

Done:
  if (EFI_ERROR (Status)) {
    for (Index = 0; Index < NumOfCpus; Index ++) {
      if (mCommonBuffer[Index].CpuArchBuffer != NULL) {
        FreePool (mCommonBuffer[Index].CpuArchBuffer);
        mCommonBuffer[Index].CpuArchBuffer = NULL;
      }
    }
  }

  return Status;
}

EFI_STATUS
RestoreBspInterrupts (
  VOID
  )
{
  RegisterEl0Stack ((VOID*)mBspEl0Sp);
  ArmWriteHcr (mBspHcrReg);
  ArmWriteVBar (mBspVbar);

  // Set binary point reg to 0x7 (no preemption)
  ArmGicV3SetBinaryPointer (0x7);

  // Set priority mask reg to 0xff to allow all priorities through
  ArmGicV3SetPriorityMask (0xff);

  // Enable gic cpu interface
  ArmGicV3EnableInterruptInterface ();

  ArmEnableInterrupts ();

  return EFI_SUCCESS;
}

EFI_STATUS
SetupInterruptStatus (
  IN  UINTN       CpuIndex
  )
{
  EFI_STATUS Status;

  if (mCommonBuffer[CpuIndex].CpuArchBuffer == NULL) {
    return EFI_NOT_READY;
  }

  // Cache the TCR, MAIR and Ttbr0 values, like MP services do
  ((AARCH64_AP_BUFFER*)mCommonBuffer[CpuIndex].CpuArchBuffer)->Tcr    = ArmGetTCR ();
  ((AARCH64_AP_BUFFER*)mCommonBuffer[CpuIndex].CpuArchBuffer)->Mair   = ArmGetMAIR ();
  ((AARCH64_AP_BUFFER*)mCommonBuffer[CpuIndex].CpuArchBuffer)->Ttbr0  = ArmGetTTBR0BaseAddress ();

  Status = InitializeCpuExceptionHandlers (NULL);
  ASSERT_EFI_ERROR (Status);

  Status = RegisterCpuInterruptHandler (ARM_ARCH_EXCEPTION_IRQ, ApIrqInterruptHandler);
  if ((Status != EFI_SUCCESS) && (Status != EFI_ALREADY_STARTED)) {
    // We can take that the handler is already registered, but not other errors
    ASSERT (FALSE);
  }

  // Enable gic cpu interface
  ArmGicV3EnableInterruptInterface ();

  // Enable the intended interrupt source
  ArmGicEnableInterrupt ((UINT32)PcdGet64 (PcdGicDistributorBase), (UINT32)PcdGet64 (PcdGicRedistributorsBase), PcdGet32 (PcdGicSgiIntId));

  ArmEnableInterrupts ();

  return EFI_SUCCESS;
}

EFI_STATUS
RestoreInterruptStatus (
  IN  UINTN       CpuIndex
  )
{
  // Disable gic cpu interface
  ArmGicV3DisableInterruptInterface ();

  // Disable the intended interrupt source
  ArmGicDisableInterrupt ((UINT32)PcdGet64 (PcdGicDistributorBase), (UINT32)PcdGet64 (PcdGicRedistributorsBase), PcdGet32 (PcdGicSgiIntId));

  return EFI_SUCCESS;
}

EFI_STATUS
CpuArchResumeCommon (
  IN  UINTN       CpuIndex
  )
{
  return EFI_SUCCESS;
}

VOID
EFIAPI
CpuArchWakeFromSleep (
  UINTN   CpuIndex
  )
{
  // Sending SGI to the specified secondary CPU interfaces
  // TODO: This is essentially reverse engineering to correlate the CPU index with the MPIDRs...
  ArmGicSendSgiTo (PcdGet64 (PcdGicDistributorBase), ARM_GIC_ICDSGIR_FILTER_TARGETLIST, mCpuInfo[CpuIndex].Mpidr, PcdGet32 (PcdGicSgiIntId));
}

VOID
ApEntryPoint (
  VOID
  )
{
  volatile MP_MANAGEMENT_METADATA   *MyBuffer;
  UINTN                             ProcessorId;
  EFI_STATUS                        Status;

  // Upon return, first figure who am i.
  Status = mMpServices->WhoAmI (mMpServices, &ProcessorId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot even figure who am I... Bail here - %r\n", Status));
    goto Done;
  }

  MyBuffer = &mCommonBuffer[ProcessorId];

  // Configure the MMU and caches
  ArmSetTCR (((AARCH64_AP_BUFFER*)MyBuffer->CpuArchBuffer)->Tcr);
  ArmSetTTBR0 (((AARCH64_AP_BUFFER*)MyBuffer->CpuArchBuffer)->Ttbr0);
  ArmSetMAIR (((AARCH64_AP_BUFFER*)MyBuffer->CpuArchBuffer)->Mair);
  ArmDisableAlignmentCheck ();
  ArmEnableStackAlignmentCheck ();
  ArmEnableInstructionCache ();
  ArmEnableDataCache ();
  ArmEnableMmu ();

  if (ProcessorId != mBspIndex) {
    Status = SetupInterruptStatus (ProcessorId);
  } else {
    Status = RestoreBspInterrupts ();
  }

  LongJump ((BASE_LIBRARY_JUMP_BUFFER*)(&(MyBuffer->JumpBuffer)), 1);

Done:
  // If LongJump succeeded, we should not even get here.
  ASSERT (FALSE);
}

EFI_STATUS
EFIAPI
CpuArchHalt (
  VOID
  )
{
  ArmEnableInterrupts ();

  ArmCallWFI ();

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
GetPowerType (
  IN UINTN          PowerLevel
  )
{
  UINTN PowerType;

  if (mExtendedPowerState) {
    PowerType = ((PowerLevel >> PSTATE_TYPE_SHIFT_EX) & PSTATE_TYPE_MASK);
  } else {
    PowerType = ((PowerLevel >> PSTATE_TYPE_SHIFT_ORIG) & PSTATE_TYPE_MASK);
  }

  return PowerType;
}

STATIC
EFI_STATUS
EFIAPI
ArmPsciSuspendHelper (
  IN UINTN          PowerLevel,
  IN UINTN          EntryPoint, OPTIONAL
  IN UINTN          ContextId   OPTIONAL
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
  Args.Arg2 = EntryPoint;
  // Parameter for context_id, only need for powerdown state
  Args.Arg3 = ContextId;

  ArmCallSmc (&Args);

  if (Args.Arg0 != ARM_SMC_PSCI_RET_SUCCESS) {
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

EFI_STATUS
EFIAPI
CpuArchClockGate (
  IN UINTN         PowerLevel
  )
{
  EFI_STATUS  Status;
  UINTN       PowerType;

  PowerType = GetPowerType (PowerLevel);
  if (PowerType == PSTATE_TYPE_POWERDOWN) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  Status = ArmPsciSuspendHelper (PowerLevel, 0, 0);

Done:
  return Status;
}

EFI_STATUS
EFIAPI
CpuArchSleep (
  IN UINTN         PowerLevel
  )
{
  EFI_STATUS  Status;
  UINTN       PowerType;

  PowerType = GetPowerType (PowerLevel);
  if (PowerType == PSTATE_TYPE_STANDBY) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  Status = ArmPsciSuspendHelper (PowerLevel, (UINTN)ApEntryPoint, 0);

Done:
  return Status;
}

EFI_STATUS
CpuArchDisableAllInterruptsButSetupTimer (
  IN  EFI_HANDLE  *Handle,
  IN  UINTN       TimeoutInMicroseconds
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINTN       GicNumInterrupts = 0;
  UINT64      CounterValue;
  UINT64      TimerTicks;
  BOOLEAN     *InterruptStates = NULL;

  if (Handle == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  // Cache the TCR, MAIR and Ttbr0 values, like MP services do
  ((AARCH64_AP_BUFFER*)mCommonBuffer[mBspIndex].CpuArchBuffer)->Tcr    = ArmGetTCR ();
  ((AARCH64_AP_BUFFER*)mCommonBuffer[mBspIndex].CpuArchBuffer)->Mair   = ArmGetMAIR ();
  ((AARCH64_AP_BUFFER*)mCommonBuffer[mBspIndex].CpuArchBuffer)->Ttbr0  = ArmGetTTBR0BaseAddress ();

  mBspVbar   = ArmReadVBar ();
  mBspHcrReg = ArmReadHcr ();
  mBspEl0Sp  = ReadEl0Stack ();

  GicNumInterrupts = ArmGicGetMaxNumInterrupts ((UINT32)PcdGet64 (PcdGicDistributorBase));
  InterruptStates  = AllocatePool (GicNumInterrupts);

  // This capturing needs to be done at the time of use instead of module init
  // Because other modules might have programmed interrupts in between.
  for (Index = 0; Index < GicNumInterrupts; Index++) {
    InterruptStates[Index] = ArmGicIsInterruptEnabled (PcdGet64 (PcdGicDistributorBase), PcdGet64 (PcdGicRedistributorsBase), Index);
    if (InterruptStates[Index]) {
      // Only touch the obviously enabled ones
      // we don't see it enabled it only means UEFI does not get this signal
      ArmGicDisableInterrupt (PcdGet64 (PcdGicDistributorBase), PcdGet64 (PcdGicRedistributorsBase), Index);
    }
  }

  // Clear any pending interrupts after they are all disabled
  UINTN IntValue = ArmGicV3AcknowledgeInterrupt ();
  ArmGicV3EndOfInterrupt (IntValue);

  // Serenity, it is...
  if (ArmIsArchTimerImplemented () == 0) {
    DEBUG ((DEBUG_ERROR, "ARM Architectural Timer is not available in the CPU, hence can't use this Driver \n"));
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  // Always disable the timer
  ArmGenericTimerDisableTimer ();

  // TimerTicks  = TimerPeriod in us unit x Frequency
  //             = (TimerPeriod in s unit x Frequency) x 10^-6
  TimerTicks = MultU64x32 (TimeoutInMicroseconds , (UINT32)ArmGenericTimerGetTimerFreq ());
  TimerTicks = DivU64x32 (TimerTicks, 1000000U);

  // Get value of the current timer
  CounterValue = ArmGenericTimerGetSystemCount ();
  // Set the interrupt in Current Time + mTimerTick
  ArmGenericTimerSetCompareVal (CounterValue + TimerTicks);

  // Enable the timer
  ArmGenericTimerEnableTimer ();

  ArmGicEnableInterrupt (PcdGet64 (PcdGicDistributorBase), PcdGet64 (PcdGicRedistributorsBase), PcdGet32 (PcdArmArchTimerSecIntrNum));
  ArmGicEnableInterrupt (PcdGet64 (PcdGicDistributorBase), PcdGet64 (PcdGicRedistributorsBase), PcdGet32 (PcdArmArchTimerIntrNum));
  ArmGicEnableInterrupt (PcdGet64 (PcdGicDistributorBase), PcdGet64 (PcdGicRedistributorsBase), PcdGet32 (PcdArmArchTimerVirtIntrNum));
  if (PcdGet32 (PcdArmArchTimerHypIntrNum)) {
    ArmGicEnableInterrupt (PcdGet64 (PcdGicDistributorBase), PcdGet64 (PcdGicRedistributorsBase), PcdGet32 (PcdArmArchTimerHypIntrNum));
  }

  Status = EFI_SUCCESS;

  // The timer will get caught by the original timer interrupt from the tiemr arch protocol
Done:
  if (EFI_ERROR (Status)) {
    if (InterruptStates != NULL) {
      FreePool (InterruptStates);
    }
  } else {
    *Handle = InterruptStates;
  }
  return Status;
}

EFI_STATUS
CpuArchRestoreAllInterrupts (
  IN  EFI_HANDLE  Handle
  )
{
  BOOLEAN                 *InterruptStates = NULL;
  UINTN                   Index;
  UINTN                   GicNumInterrupts = 0;
  UINT64                  TimerPeriod;
  EFI_STATUS              Status;
  EFI_TIMER_ARCH_PROTOCOL *TimerProtocol;

  if (Handle != NULL) {
    InterruptStates   = Handle;
    GicNumInterrupts  = ArmGicGetMaxNumInterrupts ((UINT32)PcdGet64 (PcdGicDistributorBase));
    // Grandma says when you leave the room, remember to turn off the light...
    for (Index = 0; Index < GicNumInterrupts; Index++) {
      if (InterruptStates[Index]) {
        // First touch the obviously enabled ones
        ArmGicEnableInterrupt (PcdGet64 (PcdGicDistributorBase), PcdGet64 (PcdGicRedistributorsBase), Index);
      }
    }
    FreePool (InterruptStates);
  }

  Status = gBS->LocateProtocol (
                &gEfiTimerArchProtocolGuid,
                NULL,
                (VOID **)&TimerProtocol
                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Timer protocol is not located - %r\n", Status));
    goto Done;
  }

  // Grab the timer protocol cached value
  Status = TimerProtocol->GetTimerPeriod (TimerProtocol, &TimerPeriod);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Timer period is not fetched - %r\n", Status));
    goto Done;
  }

  // And set it back, trying to make it looks like nothing ever happened...
  Status = TimerProtocol->SetTimerPeriod (TimerProtocol, TimerPeriod);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Timer period is not recovered - %r\n", Status));
    goto Done;
  }

Done:
  return Status;
}
