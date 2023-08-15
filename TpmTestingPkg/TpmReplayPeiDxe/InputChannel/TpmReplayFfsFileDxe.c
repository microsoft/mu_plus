/** @file
  TPM Replay DXE FFS File Logic

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>

#include "../TpmReplayEventLog.h"
#include "TpmReplayInputChannelInternal.h"

/**
  Retrieves a TPM Replay Event Log from a FFS file.

  @param[out] Data            A pointer to a pointer to the buffer to hold the event log data.
  @param[out] DataSize        The size of the data placed in the data buffer.

  @retval    EFI_SUCCESS            The TPM Replay event log was returned successfully.
  @retval    EFI_INVALID_PARAMETER  A pointer argument given is NULL.
  @retval    EFI_UNSUPPORTED        The function is not implemented yet. The arguments are not used.
  @retval    EFI_COMPROMISED_DATA   The event log data found is not valid.
  @retval    EFI_NOT_FOUND          The event log data was not found in a FFS file.

**/
EFI_STATUS
GetTpmReplayEventLogFfsFile (
  OUT VOID   **Data,
  OUT UINTN  *DataSize
  )
{
  // Todo: Add DXE implementation
  return EFI_UNSUPPORTED;
}
