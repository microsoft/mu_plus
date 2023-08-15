/** @file
  TPM Replay PEI Pre-Memory - Generic pre-memory functionality.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/TpmInitialized.h>

#include "TpmReplayPei.h"

STATIC EFI_PEI_NOTIFY_DESCRIPTOR  mTpmInitDoneNotifyList = {
  EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gPeiTpmInitializationDonePpiGuid,
  TpmReplayTpmInitializedNotify
};

/**
  Performs actions needed in pre-memory to support TPM Replay.

  @retval    EFI_SUCCESS            The actions were performed successfully.
  @retval    Others                 An error occurred preventing the TPM Replay from being
                                    initialized properly.

**/
EFI_STATUS
TpmReplayPreMemActions (
  VOID
  )
{
  // Wait for TPM to Be Initialized to Proceed
  return PeiServicesNotifyPpi (&mTpmInitDoneNotifyList);
}
