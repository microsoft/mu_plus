/** @file
  Structured ECIT audit evidence emission.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ECIT_EVIDENCE_H_
#define ECIT_EVIDENCE_H_

#include <Uefi.h>

#include <Guid/CryptoIndicatorTable.h>

#include "AuditTypes.h"

typedef struct {
  UINTN    Advertised;
  UINTN    Tested;
  UINTN    NotTested;
  UINTN    Mismatches;
} ECIT_PROFILE_COUNTS;

CONST CHAR8 *
EcitClaimStateName (
  IN ECIT_CLAIM_STATE  Claim
  );

CONST CHAR8 *
EcitObservationStateName (
  IN ECIT_OBSERVATION_STATE  Observation
  );

CONST CHAR8 *
EcitCoverageStateName (
  IN ECIT_COVERAGE_STATE  Coverage
  );

EFI_STATUS
GetEcitProfileCounts (
  IN CONST EFI_CRYPTO_INDICATOR_TABLE  *Table,
  IN CONST EFI_GUID                    *Feature,
  IN CONST ECIT_COVERAGE_STATE         *CoverageRows,
  OUT ECIT_PROFILE_COUNTS              *Counts
  );

VOID
LogEcitEvidence (
  IN CONST EFI_CRYPTO_INDICATOR_TABLE  *Table OPTIONAL,
  IN EFI_STATUS                        ParseStatus
  );

#endif
