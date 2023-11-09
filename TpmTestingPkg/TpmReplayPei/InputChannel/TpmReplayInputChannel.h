/** @file
  TPM Replay Generic Input Channel Header

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TPM_REPLAY_INPUT_CHANNEL_H_
#define TPM_REPLAY_INPUT_CHANNEL_H_

#include <Guid/TpmReplayEventLog.h>

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
  );

#endif
