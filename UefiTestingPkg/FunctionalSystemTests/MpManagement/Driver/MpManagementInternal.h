/** @file
  Internal metadata, enum and function definitions for MP management
  protocol driver.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MP_MANAGEMENT_INTERNAL_H_
#define MP_MANAGEMENT_INTERNAL_H_

typedef enum {
  AP_TASK_IDLE,
  AP_TASK_BUSY,
  AP_TASK_ACTIVE,
  AP_TASK_NUM
} AP_TASK;

typedef enum {
  AP_STATE_ON,
  AP_STATE_OFF,
  AP_STATE_SUSPEND_HALT,
  AP_STATE_SUSPEND_CLOCK_GATE,
  AP_STATE_SUSPEND_SLEEP,
  AP_STATE_RESUME,
  AP_STATE_NUM
} AP_STATE;

#pragma pack (push, 1)
///
/// Structure that describes information about a logical CPU.
///
typedef struct {
  UINTN                       ApStatus;     // AP_STATE
  UINTN                       TargetStatus; // AP_STATE
  UINTN                       ApTask;       // AP_TASK
  UINTN                       TargetPowerState;
  BASE_LIBRARY_JUMP_BUFFER    JumpBuffer;
  UINTN                       ApBufferSize;
  VOID                        *ApBuffer;
  VOID                        *CpuArchBuffer;
} MP_MANAGEMENT_METADATA;
#pragma pack (pop)

extern UINTN                            mNumCpus;
extern UINTN                            mBspIndex;
extern volatile MP_MANAGEMENT_METADATA  *mCommonBuffer;
extern EFI_MP_SERVICES_PROTOCOL         *mMpServices;

/** The procedure to run with the MP Services interface.

  @param Arg The procedure argument.

**/
VOID
EFIAPI
ApFunction (
  IN OUT VOID  *Arg
  );

/**
  Architectural initialization routine, allowing different CPU architectures
  to prepare their own register data buffer, data cache, etc.

  @param  NumOfCpus     The number of CPUs supported on this platform.

  @return EFI_SUCCESS   The routine completed successfully.
  @return Others        The routine failed during operation.
**/
EFI_STATUS
CpuMpArchInit (
  IN UINTN  NumOfCpus
  );

/**
  This routine will setup/recover the AP specific interrupt states.

  The main goal is to enable the AP to accept software generated
  interrupts sent from BSP.

  @param  CpuIndex      The number of intended CPU to be setup.

  @return EFI_SUCCESS   The routine completed successfully.
  @return Others        The routine failed during operation.
**/
EFI_STATUS
SetupInterruptStatus (
  IN  UINTN  CpuIndex
  );

/**
  This routine will restore the AP specific interrupt states after
  the entire AP routine is about to be completed.

  @param  CpuIndex      The number of intended CPU to be setup.

  @return EFI_SUCCESS   The routine completed successfully.
  @return Others        The routine failed during operation.
**/
EFI_STATUS
RestoreInterruptStatus (
  IN  UINTN  CpuIndex
  );

/**
  This routine will perform common architectural restores after all
  types of suspend resumption.

  @param  CpuIndex      The number of intended CPU to be setup.

  @return EFI_SUCCESS   The routine completed successfully.
  @return Others        The routine failed during operation.
**/
EFI_STATUS
CpuArchResumeCommon (
  IN  UINTN  CpuIndex
  );

/**
  This routine will be used for suspending the currently running
  processor to halt state. It could be run by both BSP and APs.

  Given the state definition, this function will halt execution
  until woken up.

  @return EFI_SUCCESS   The routine wake up successfully.
  @return Others        The routine failed during operation.
**/
EFI_STATUS
EFIAPI
CpuArchHalt (
  VOID
  );

/**
  This routine will be used for suspending the currently running
  processor to clock gate state. It could be run by both BSP and APs.

  This architectural specific routine should validate whether the
  power state is supported for clock gate suspension.

  Given the state definition, this function will halt execution
  until woken up.

  @param  PowerState    The intended power state.

  @return EFI_SUCCESS   The routine wake up successfully.
  @return Others        The routine failed during operation.
**/
EFI_STATUS
EFIAPI
CpuArchClockGate (
  IN UINTN  PowerState   OPTIONAL
  );

/**
  This routine will be used for suspending the currently running
  processor to sleep state. It could be run by both BSP and APs.

  This architectural specific routine should validate whether the
  power state is supported for clock gate suspension.

  Given the state definition, this function will make the CPU to
  resume without any context. The caller should handle the data
  saving and restoration accordingly.

  @param  PowerState    The intended power state.

  @return EFI_SUCCESS   The routine wake up successfully.
  @return Others        The routine failed during operation.
**/
EFI_STATUS
EFIAPI
CpuArchSleep (
  IN UINTN  PowerState   OPTIONAL
  );

/**
  This helper routine will be used for preparing the active BSP
  to enter sleep state. It could be run by BSP.

  This architectural specific routine should setup necessary wakeup
  resources, if not already provided, for the CPU to wake up from
  sleep state.

  Given the state definition, this function will make the CPU to
  resume without any context. The caller should handle the data
  saving and restoration accordingly.

  @param  PowerState              The intended power state.
  @param  TimeoutInMicrosecond    The intended timer to wake this core.

  @return EFI_SUCCESS   The routine wake up successfully.
  @return Others        The routine failed during operation.
**/
EFI_STATUS
EFIAPI
CpuArchBspSleepPrep (
  IN UINTN PowerState, OPTIONAL
  IN UINTN  TimeoutInMicrosecond
  );

/**
  This routine is invoked by BSP to wake up suspended APs.

  @param  CpuIndex      The number of intended CPU to be setup.

  @return None.
**/
VOID
EFIAPI
CpuArchWakeFromSleep (
  UINTN  CpuIndex
  );

/**
  This routine will be used for disabling all the current interrupts,
  but set up timer interrupt to prepare for BSP suspension. It is only
  run by BSP.

  @param  Handle        An EFI_HANDLE that is used for the BSP to
                        manage and cache current interrupts' status.

  @return EFI_SUCCESS   The routine wake up successfully.
  @return Others        The routine failed during operation.
**/
EFI_STATUS
CpuArchDisableAllInterruptsButSetupTimer (
  IN  EFI_HANDLE  *Handle,
  IN  UINTN       TimeoutInMicroseconds
  );

/**
  This routine will be used for restoring all the interrupts, from
  previously prepared EFI_HANDLE before BSP finishes timed suspension
  routine. It is only run by BSP.

  @param  Handle        An EFI_HANDLE that is used for the BSP to
                        manage and cache current interrupts' status.

  @return EFI_SUCCESS   The routine wake up successfully.
  @return Others        The routine failed during operation.
**/
EFI_STATUS
CpuArchRestoreAllInterrupts (
  IN  EFI_HANDLE  Handle
  );

#endif //  MP_MANAGEMENT_INTERNAL_H_
