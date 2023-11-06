/** @file
  A null instance of the Input Channel Library.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/InputChannelLib.h>

/**
  Retrieves a TPM Replay Event Log through a custom interface.

  @param[out] ReplayEventLog            A pointer to a pointer to the buffer to hold the event log data.
  @param[out] ReplayEventLogSize        The size of the data placed in the buffer.

  @retval    EFI_SUCCESS            The TPM Replay event log was returned successfully.
  @retval    EFI_INVALID_PARAMETER  A pointer argument given is NULL.
  @retval    EFI_UNSUPPORTED        The function is not implemented yet. The arguments are not used.
  @retval    EFI_COMPROMISED_DATA   The event log data found is not valid.
  @retval    EFI_NOT_FOUND          The event log data was not found. The input channel is ignored in this case.

**/
EFI_STATUS
EFIAPI
GetReplayEventLogFromCustomInterface (
  OUT VOID   **ReplayEventLog,
  OUT UINTN  *ReplayEventLogSize
  )
{
  return EFI_UNSUPPORTED;
}
