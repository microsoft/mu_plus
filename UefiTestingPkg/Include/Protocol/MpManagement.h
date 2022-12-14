/** @file
  When installed, the MP Management Protocol produces a collection of power
  management service to power on/off the APs and suspend/resume all cores.

  Copyright (c) Microsoft Corporation.
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

///
/// Supported processor suspension states.
///
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

/**
  A BSP invoked function to perform self suspend. A timeout period needs
  to be provided by the called to invoke self-wakeup service.

  @param This                   MP Management Protocol.
  @param BspPowerState          The target power state the BSP should be
                                suspended to.
  @param TargetPowerLevel       The target power level of BSP after suspending,
                                certain architecture could require this value
                                to be paired with BspPowerState.
  @param TimeoutInMicroseconds  Time out in microseconds specified when the
                                timer should fire to wake up itself.

  @return EFI_SUCCESS             The routine completed successfully.
  @return EFI_INVALID_PARAMETER   The input power level or state is not within range.
  @return Others                  Other failures from interrupt setup/restorations.
**/
typedef
EFI_STATUS
(EFIAPI *MP_MANAGEMENT_BSP_SUSPEND)(
  IN  MP_MANAGEMENT_PROTOCOL  *This,
  IN  AP_POWER_STATE          BspPowerState,
  IN  UINTN                   TargetPowerLevel,  OPTIONAL
  IN  UINTN                   TimeoutInMicroseconds
  );

/**
  Function to perform AP power on.

  @param This             MP Management Protocol.
  @param ProcessorNumber  The CPU index to be powered on.

  @return EFI_SUCCESS             The routine completed successfully.
  @return EFI_INVALID_PARAMETER   The CPU index is out of range.
  @return EFI_NOT_READY           The MP service is not initialized.
  @return EFI_ALREADY_STARTED     The target AP is already in ON state.
  @return EFI_ABORTED             The target AP is in unexpected states.
  @return Others                  Other errors from MP services.
**/
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

/**
  Function to perform AP power off.

  @param This             MP Management Protocol.
  @param ProcessorNumber  The CPU index to be powered off.

  @return EFI_SUCCESS             The routine completed successfully.
  @return EFI_INVALID_PARAMETER   The CPU index is out of range.
  @return EFI_NOT_READY           The MP service is not initialized.
  @return EFI_ALREADY_STARTED     The target AP is already in OFF state.
  @return EFI_ABORTED             The target AP is in unexpected states.
  @return Others                  Other errors from MP services.
**/
typedef
EFI_STATUS
(EFIAPI *MP_MANAGEMENT_AP_OFF)(
  IN  MP_MANAGEMENT_PROTOCOL  *This,
  IN  UINTN                   ProcessorNumber
  );

/**
  Function to perform AP execution suspend.

  @param This             MP Management Protocol.
  @param ProcessorNumber  The CPU index to be powered off.
  @param ApPowerState     The intended power state the CPU should be suspended to.
                          The support input values are defined in AP_POWER_STATE.
  @param TargetPowerLevel The target power level of AP when suspended, certain
                          architecture could require this value to be paired with
                          ApPowerState.

  @return EFI_SUCCESS             The routine completed successfully.
  @return EFI_INVALID_PARAMETER   The CPU index is out of range or power state is
                                  not setup properly.
  @return EFI_NOT_READY           The MP service is not initialized.
  @return EFI_ALREADY_STARTED     The target AP is already in intended power state.
  @return EFI_ABORTED             The target AP is in unexpected states.
**/
typedef
EFI_STATUS
(EFIAPI *MP_MANAGEMENT_AP_SUSPEND)(
  IN  MP_MANAGEMENT_PROTOCOL  *This,
  IN  UINTN                   ProcessorNumber,
  IN  AP_POWER_STATE          ApPowerState,
  IN  UINTN                   TargetPowerLevel  OPTIONAL
  );

/**
  Function to perform AP execution resumption.

  @param This             MP Management Protocol.
  @param ProcessorNumber  The CPU index to be powered off.

  @return EFI_SUCCESS             The routine completed successfully.
  @return EFI_INVALID_PARAMETER   The CPU index is out of range or power state is
                                  not setup properly.
  @return EFI_NOT_READY           The MP service is not initialized.
  @return EFI_ALREADY_STARTED     The target AP is already in ON state.
  @return EFI_ABORTED             The target AP is in unexpected states.
**/
typedef
EFI_STATUS
(EFIAPI *MP_MANAGEMENT_AP_RESUME)(
  IN  MP_MANAGEMENT_PROTOCOL  *This,
  IN  UINTN                   ProcessorNumber
  );

struct _MP_MANAGEMENT_PROTOCOL {
  MP_MANAGEMENT_BSP_SUSPEND   BspSuspend;
  MP_MANAGEMENT_AP_ON         ApOn;
  // MP_MANAGEMENT_AP_PROCEDURE  ApProcedure;
  MP_MANAGEMENT_AP_OFF        ApOff;
  MP_MANAGEMENT_AP_SUSPEND    ApSuspend;
  MP_MANAGEMENT_AP_RESUME     ApResume;
};

extern EFI_GUID  gMpManagementProtocolGuid;

#endif
