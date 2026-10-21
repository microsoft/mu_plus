/** @file
  Data-driven authenticated-variable scenarios for the Secure Boot key audit.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AUDIT_SCENARIO_H_
#define AUDIT_SCENARIO_H_

#include <Uefi.h>

#include "AuditTypes.h"

typedef struct {
  CONST CHAR8            *ScenarioId;
  CONST CHAR16           *VariableName;
  CONST EFI_GUID         *VendorGuid;
  UINT32                 Attributes;
  CONST UINT8            *AuthenticatedUpdate;
  UINTN                  AuthenticatedUpdateSize;
  EFI_STATUS             ExpectedStatus;
  CONST UINT8            *ExpectedBefore;
  UINTN                  ExpectedBeforeSize;
  UINT32                 ExpectedBeforeAttributes;
  CONST UINT8            *ExpectedAfter;
  UINTN                  ExpectedAfterSize;
  UINT32                 ExpectedAfterAttributes;
  BOOLEAN                ExpectMutation;
  CONST ECIT_CLAIM_ID    *CoveredClaims;
  UINTN                  CoveredClaimCount;
} AUDIT_AUTH_VARIABLE_SCENARIO;

EFI_STATUS
ExecuteAuthVariableScenario (
  IN CONST AUDIT_AUTH_VARIABLE_SCENARIO  *Scenario,
  IN OUT ECIT_CLAIM_RESULT               *ClaimResults,
  IN UINTN                               ClaimResultCount
  );

#endif
