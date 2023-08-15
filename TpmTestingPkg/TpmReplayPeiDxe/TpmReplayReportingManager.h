/** @file
  TPM Replay Reporting Manager - Manages reporting of TPM Replay status.

  The purpose of this manage is to provide a common funnel of important user-visible
  errors. Changing the reporting of these errors is consolidated in a common reporting
  function with dependable error code values.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TPM_REPLAY_REPORTING_MANAGER_H_
#define TPM_REPLAY_REPORTING_MANAGER_H_

#include <Base.h>

#define REPORT_ERROR(_Error)                                              \
          ReportTpmReplayError (_Error, __func__);

#define REPORT_IF_STATUS_ERROR(_Status, _Error)                           \
          if (EFI_ERROR (_Status)) {                                      \
            ReportTpmReplayError (_Error, __func__);                      \
          }

#define REPORT_AND_RETURN_IF_STATUS_ERROR(_Status, _Error, _ReturnValue)  \
          if (EFI_ERROR (_Status)) {                                      \
            ReportTpmReplayError (_Error, __func__);                      \
            return _ReturnValue;                                          \
          }

#define REPORT_ON_CONDITION(_Condition, _Error)                           \
          if (_Condition) {                                               \
            ReportTpmReplayError (_Error, __func__);                      \
          }

#define REPORT_AND_RETURN_ON_CONDITION(_Condition, _Error, _ReturnValue)  \
          if (_Condition) {                                               \
            ReportTpmReplayError (_Error, __func__);                      \
            return _ReturnValue;                                          \
          }

//
// Each bit should represent a unique error
//
typedef enum {
  TpmReplayErrorUnknown                        = BIT0,
  TpmReplayErrorTpmNotReady                    = BIT1,
  TpmReplayErrorTpmExtendError                 = BIT2,
  TpmReplayErrorEventLogEntryCreationFailure   = BIT3,
  TpmReplayErrorReplayEventLogRetrievalFailure = BIT4,
  TpmReplayErrorReplayEventLogInvalid          = BIT5,
  TpmReplayErrorDigestUnpackFailed             = BIT6,
  TpmReplayErrorEventUnpackFailed              = BIT7,
  TpmReplaySimErrorMax                         = MAX_UINT64
} TPM_REPLAY_ERROR;

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
  );

#endif
