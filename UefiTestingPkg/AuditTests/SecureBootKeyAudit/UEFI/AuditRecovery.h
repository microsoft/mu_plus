/** @file
  Recognized-state recovery for the Secure Boot key audit.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AUDIT_RECOVERY_H_
#define AUDIT_RECOVERY_H_

#include <Uefi.h>

#include "AuditTypes.h"

BOOLEAN
RecoveryStateIsRecognized (
  IN CONST AUDIT_BASELINE_SNAPSHOT  *Snapshot
  );

EFI_STATUS
RecoverSetupMode (
  VOID
  );

#endif
