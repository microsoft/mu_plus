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
  AP_STATE_SUSPEND,
  AP_STATE_RESUME,
  AP_STATE_NUM
} AP_STATE;

///
/// Structure that describes information about a logical CPU.
///
typedef struct {
  AP_STATE    ApStatus;
  AP_STATE    TargetStatus;
  AP_TASK     ApTask;
  UINTN       ApBufferSize;
  VOID        *ApBuffer;
} MP_MANAGEMENT_METADATA;

typedef struct {
  EFI_MP_SERVICES_PROTOCOL    *Mp;
  CHAR16                      **Buffer;
} APFUNC_ARG;

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

#endif  //  MP_MANAGEMENT_INTERNAL_H_
