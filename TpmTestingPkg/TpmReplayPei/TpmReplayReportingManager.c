/** @file
  TPM Replay Reporting Manager - Manages reporting of TPM Replay status.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TpmReplayReportingManager.h"

#include <Uefi.h>
#include <Library/DebugLib.h>

/**
  Reports a TPM replay error.

  A common reporting implementation for the feature that can be updated to report to sources
  such as serial, status code reporting, telemetry, etc.

  @param[in]  Error             The error code.
  @param[in]  FunctionName      The function name the error occurred in.

**/
VOID
ReportTpmReplayError (
  IN  TPM_REPLAY_ERROR  Error,
  IN  CONST CHAR8       *FunctionName   OPTIONAL
  )
{
  CONST CHAR8  *DebugFunctionName;

  DebugFunctionName = __func__;

  if (FunctionName != NULL) {
    DebugFunctionName = FunctionName;
  }

  DEBUG ((DEBUG_ERROR, "[%a] - TPM Replay error reported (%d).\n", DebugFunctionName, Error));
  ASSERT (FALSE);

  return;
}
