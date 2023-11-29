/** @file
  TPM Replay Phase-Agnostic UEFI Variable Logic

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Guid/TpmReplayEventLog.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/ReadOnlyVariable2.h>

#include "TpmReplayInputChannelInternal.h"

/**
  Retrieves a TPM Replay Event Log from a UEFI variable.

  @param[out] Data            A pointer to a pointer to the buffer to hold the event log data.
  @param[out] DataSize        The size of the data placed in the data buffer.

  @retval    EFI_SUCCESS            The TPM Replay event log was returned successfully.
  @retval    EFI_INVALID_PARAMETER  A pointer argument given is NULL.
  @retval    EFI_OUT_OF_RESOURCES   Insufficient memory resources to allocate a necessary buffer.
  @retval    EFI_UNSUPPORTED        The function is not implemented yet. The arguments are not used.
  @retval    EFI_NOT_FOUND          The event log data was not found in a UEFI variable.

**/
EFI_STATUS
GetTpmReplayEventLogUefiVariable (
  OUT VOID   **Data,
  OUT UINTN  *DataSize
  )
{
  EFI_STATUS  Status;
  UINTN       VariableDataSize;
  VOID        *VariableData;

  if ((Data == NULL) || (DataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  VariableData     = NULL;
  VariableDataSize = 0;
  Status           =  InternalGetVariable (
                        TPM_REPLAY_UEFI_VARIABLE_NAME,
                        &gTpmReplayVendorGuid,
                        NULL,
                        &VariableDataSize,
                        VariableData
                        );
  ASSERT (Status == EFI_NOT_FOUND || Status == EFI_BUFFER_TOO_SMALL);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    VariableData = AllocateZeroPool (VariableDataSize);
    if (VariableData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status =  InternalGetVariable (
                TPM_REPLAY_UEFI_VARIABLE_NAME,
                &gTpmReplayVendorGuid,
                NULL,
                &VariableDataSize,
                VariableData
                );
    ASSERT_EFI_ERROR (Status);
    if (!EFI_ERROR (Status)) {
      *Data     = VariableData;
      *DataSize = VariableDataSize;

      return EFI_SUCCESS;
    }
  }

  if (VariableData != NULL) {
    FreePool (VariableData);
  }

  return EFI_NOT_FOUND;
}
