/** @file
  Secure Boot variable state access for the key audit.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef SECURE_BOOT_STATE_H_
#define SECURE_BOOT_STATE_H_

#include <Uefi.h>

#include "AuditTypes.h"

#define SECURE_BOOT_KEY_ATTRIBUTES  \
  (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | \
   EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)

EFI_STATUS
ReadVariable (
  IN  CONST CHAR16    *Name,
  IN  CONST EFI_GUID  *Guid,
  OUT UINT32          *Attributes,
  OUT UINT8           **Data,
  OUT UINTN           *DataSize
  );

EFI_STATUS
ReadModeVariable (
  IN  CONST CHAR16  *Name,
  OUT UINT8         *Value
  );

BOOLEAN
PkMatches (
  IN CONST UINT8  *Pk,
  IN UINTN        PkSize,
  IN CONST UINT8  *Expected,
  IN UINTN        ExpectedSize
  );

EFI_STATUS
VerifyPk (
  IN CONST UINT8  *Expected,
  IN UINTN        ExpectedSize
  );

EFI_STATUS
VerifyDb (
  IN CONST UINT8  *Expected,
  IN UINTN        ExpectedSize
  );

EFI_STATUS
VerifyDbx (
  IN CONST UINT8  *Expected,
  IN UINTN        ExpectedSize
  );

EFI_STATUS
ApplyPkUpdate (
  IN CONST UINT8  *AuthenticatedUpdate,
  IN UINTN        AuthenticatedUpdateSize,
  IN CONST UINT8  *ExpectedPk,
  IN UINTN        ExpectedPkSize
  );

EFI_STATUS
ApplyDbUpdate (
  IN CONST UINT8  *AuthenticatedUpdate,
  IN UINTN        AuthenticatedUpdateSize,
  IN CONST UINT8  *ExpectedDb,
  IN UINTN        ExpectedDbSize
  );

EFI_STATUS
ApplyDbxUpdate (
  IN CONST UINT8  *AuthenticatedUpdate,
  IN UINTN        AuthenticatedUpdateSize,
  IN CONST UINT8  *ExpectedDbx,
  IN UINTN        ExpectedDbxSize
  );

CONST CHAR8 *
AuditVariableStateName (
  IN AUDIT_VARIABLE_STATE  State
  );

EFI_STATUS
CaptureBaselineSnapshot (
  OUT AUDIT_BASELINE_SNAPSHOT  *Snapshot
  );

EFI_STATUS
InspectSetupModeBaseline (
  OUT AUDIT_BASELINE_SNAPSHOT  *Snapshot
  );

EFI_STATUS
RequireSetupModeBaseline (
  VOID
  );

#endif
