/** @file
  When installed, the MP Services Protocol produces a collection of services
  that are needed for MP management.

  Copyright (c) 2006 - 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MP_MANAGEMENT_PROTOCOL_H_
#define MP_MANAGEMENT_PROTOCOL_H_

///
/// Global ID for the MP_MANAGEMENT_PROTOCOL.
///
#define MP_MANAGEMENT_PROTOCOL_GUID \
  { \
    0x3fdda605, 0xa76e, 0x4f46, {0xad, 0x29, 0x12, 0xf4, 0x53, 0x1b, 0x3d, 0x08} \
  }

///
/// Secretive combo to apply the operation to all APs.
///
#define OPERATION_FOR_ALL_APS     MAX_UINTN

typedef enum {
  AP_POWER_C1,
  AP_POWER_C2,
  AP_POWER_C3,
  AP_POWER_NUM
} AP_POWER_STATE;

///
/// Forward declaration for the MP_MANAGEMENT_PROTOCOL.
///
typedef struct _MP_MANAGEMENT_PROTOCOL MP_MANAGEMENT_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *MP_MANAGEMENT_INITIALIZE)(
  IN  MP_MANAGEMENT_PROTOCOL  *This
  );

typedef
EFI_STATUS
(EFIAPI *MP_MANAGEMENT_BSP_SUSPEND)(
  IN  MP_MANAGEMENT_PROTOCOL  *This,
  IN  AP_POWER_STATE          BspPowerState,
  IN  UINTN                   TargetPowerLevel,  OPTIONAL
  IN  UINTN                   TimeoutInMicroseconds
  );

typedef
EFI_STATUS
(EFIAPI *MP_MANAGEMENT_AP_ON)(
  IN  MP_MANAGEMENT_PROTOCOL  *This,
  IN  UINTN                   ProcessorNumber
  );

///
/// Potential support for supplying arbitrary function for AP to proceed.
///
// typedef
// EFI_STATUS
// (EFIAPI *MP_MANAGEMENT_AP_PROCEDURE)(
//   IN  MP_MANAGEMENT_PROTOCOL  *This,
//   IN  UINTN                   ProcessorNumber,
//   IN  EFI_AP_PROCEDURE        Procedure,
//   IN  VOID                    *ProcedureArgument      OPTIONAL,
//   );

typedef
EFI_STATUS
(EFIAPI *MP_MANAGEMENT_AP_OFF)(
  IN  MP_MANAGEMENT_PROTOCOL  *This,
  IN  UINTN                   ProcessorNumber
  );

typedef
EFI_STATUS
(EFIAPI *MP_MANAGEMENT_AP_SUSPEND)(
  IN  MP_MANAGEMENT_PROTOCOL  *This,
  IN  UINTN                   ProcessorNumber,
  IN  AP_POWER_STATE          ApPowerState,
  IN  UINTN                   TargetPowerLevel  OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *MP_MANAGEMENT_AP_RESUME)(
  IN  MP_MANAGEMENT_PROTOCOL  *This,
  IN  UINTN                   ProcessorNumber
  );

typedef
EFI_STATUS
(EFIAPI *MP_MANAGEMENT_CLEANUP)(
  IN MP_MANAGEMENT_PROTOCOL  *This
  );

struct _MP_MANAGEMENT_PROTOCOL {
  MP_MANAGEMENT_INITIALIZE    Initialize;
  MP_MANAGEMENT_BSP_SUSPEND   BspSuspend;
  MP_MANAGEMENT_AP_ON         ApOn;
  // MP_MANAGEMENT_AP_PROCEDURE  ApProcedure;
  MP_MANAGEMENT_AP_OFF        ApOff;
  MP_MANAGEMENT_AP_SUSPEND    ApSuspend;
  MP_MANAGEMENT_AP_RESUME     ApResume;
  MP_MANAGEMENT_CLEANUP       Cleanup;
};

extern EFI_GUID  gMpManagementProtocolGuid;

#endif
