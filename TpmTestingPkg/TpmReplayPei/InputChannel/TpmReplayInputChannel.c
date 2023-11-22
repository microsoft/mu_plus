/** @file
  TPM Replay Generic Input Channel Logic

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Guid/TpmReplayEventLog.h>
#include <Library/DebugLib.h>
#include <Library/InputChannelLib.h>

#include "TpmReplayInputChannel.h"
#include "TpmReplayInputChannelInternal.h"

/**
  Retrieves a TPM Replay Event Log from the highest priority input channel.

  @param[out] ReplayEventLog            A pointer to a pointer to the buffer to hold the event log data.
  @param[out] ReplayEventLogSize        The size of the data placed in the buffer.

  @retval    EFI_SUCCESS            The TPM Replay event log was returned successfully.
  @retval    EFI_INVALID_PARAMETER  A pointer argument given is NULL.
  @retval    EFI_UNSUPPORTED        The function is not implemented yet. The arguments are not used.
  @retval    EFI_COMPROMISED_DATA   The event log data found is not valid.
  @retval    EFI_NOT_FOUND          The event log data was not found in a FFS file.

**/
EFI_STATUS
GetReplayEventLog (
  OUT TPM_REPLAY_EVENT_LOG  **ReplayEventLog,
  OUT UINTN                 *ReplayEventLogSize
  )
{
  EFI_STATUS  Status;
  UINTN       ReplayEventLogDataSize;
  VOID        *ReplayEventLogData;

  ReplayEventLogData     = NULL;
  ReplayEventLogDataSize = 0;

  // First priority: UEFI variable set on the DUT
  Status = GetTpmReplayEventLogUefiVariable (&ReplayEventLogData, &ReplayEventLogDataSize);
  ASSERT (Status == EFI_SUCCESS || Status == EFI_NOT_FOUND);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "[%a] - Using TPM replay event log from UEFI variable.\n", __func__));
    goto Done;
  }

  // Second priority: Custom interface
  Status = GetReplayEventLogFromCustomInterface (&ReplayEventLogData, &ReplayEventLogDataSize);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "[%a] - Using TPM replay event log from a custom interface.\n", __func__));
    goto Done;
  } else if (EFI_ERROR (Status) && ((Status != EFI_UNSUPPORTED) && (Status != EFI_NOT_FOUND))) {
    DEBUG ((DEBUG_ERROR, "[%a] - TPM replay event log from custom interface failed - %r.\n", __func__, Status));
  }

  // Third priority: FFS in the FW image
  Status = GetTpmReplayEventLogFfsFile (&ReplayEventLogData, &ReplayEventLogDataSize);
  ASSERT (Status == EFI_SUCCESS || Status == EFI_NOT_FOUND);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "[%a] - Using TPM replay event log from the firmware flash image.\n", __func__));
    goto Done;
  }

  Status = EFI_NOT_FOUND;

Done:
  *ReplayEventLog     = ReplayEventLogData;
  *ReplayEventLogSize = ReplayEventLogDataSize;

  return Status;
}
