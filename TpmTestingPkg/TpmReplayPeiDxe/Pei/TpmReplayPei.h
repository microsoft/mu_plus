/** @file
  TPM Replay PEI-Specific Header

  Generic module-wide definitions specific to PEI.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TPM_REPLAY_PEI_H_
#define TPM_REPLAY_PEI_H_

#include <PiPei.h>

/**
  Performs actions needed in pre-memory to support TPM Replay.

  @retval    EFI_SUCCESS            The actions were performed successfully.
  @retval    Others                 An error occurred preventing the TPM Replay from being
                                    initialized properly.

**/
EFI_STATUS
TpmReplayPreMemActions (
  VOID
  );

/**
  Performs TPM Replay actions that are dependent on TPM being initialized.

  @param[in]  PeiServices  Pointer to PEI Services Table.
  @param[in]  NotifyDesc   Pointer to the descriptor for the notification event that
                           caused callback to this function.
  @param[in]  NotifyPpi    Pointer to the PPI interface associated with this notify descriptor.

  @retval     EFI_SUCCESS  This function always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
TpmReplayTpmInitializedNotify (
  IN  EFI_PEI_SERVICES           **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDesc,
  IN  VOID                       *NotifyPpi
  );

#endif
