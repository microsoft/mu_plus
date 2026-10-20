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

VOID
LogEcitEvidence (
  IN CONST EFI_CRYPTO_INDICATOR_TABLE  *Table OPTIONAL,
  IN CONST ECIT_CLAIM_RESULT           *ClaimResults,
  IN CONST ECIT_COVERAGE_STATE         *CoverageRows,
  IN UINTN                             ClaimResultCount,
  IN EFI_STATUS                        ParseStatus
  );

#endif
