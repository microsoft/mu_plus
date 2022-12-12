/** @file
  TODO: Populate this.

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
  UINTN                     ApStatus;       // AP_STATE
  UINTN                     TargetStatus;       // AP_STATE
  UINTN                     ApTask;       // AP_TASK
  UINTN                     TargetPowerState;
  BASE_LIBRARY_JUMP_BUFFER  JumpBuffer;
  UINTN                     ApBufferSize;
  VOID                      *ApBuffer;
  VOID                      *CpuArchBuffer;
} MP_MANAGEMENT_METADATA;

typedef struct {
  EFI_MP_SERVICES_PROTOCOL    *Mp;
  CHAR16                      **Buffer;
} APFUNC_ARG;
#pragma pack (pop)

extern UINTN                              mNumCpus;
extern volatile MP_MANAGEMENT_METADATA    *mCommonBuffer;
extern EFI_MP_SERVICES_PROTOCOL           *mMpServices;

/** The procedure to run with the MP Services interface.

  @param Arg The procedure argument.

**/
VOID
EFIAPI
ApFunction (
  IN OUT VOID  *Arg
  );

EFI_STATUS
CpuMpArchInit (
  IN UINTN        NumOfCpus
  );

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

VOID
EFIAPI
CpuArchWakeFromSleep (
  UINTN   CpuIndex
  );

EFI_STATUS
CpuArchDisableAllInterruptsButSetupTimer (
  IN  EFI_HANDLE  *Handle,
  IN  UINTN       TimeoutInMicroseconds
  );

EFI_STATUS
CpuArchRestoreAllInterrupts (
  IN  EFI_HANDLE  Handle
  );

#endif  //  MP_MANAGEMENT_INTERNAL_H_
