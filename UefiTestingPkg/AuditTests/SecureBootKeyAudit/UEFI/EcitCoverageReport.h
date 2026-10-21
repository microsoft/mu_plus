/** @file
  Human-readable ECIT audit coverage reporting.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ECIT_COVERAGE_REPORT_H_
#define ECIT_COVERAGE_REPORT_H_

#include <Uefi.h>

#include <Guid/CryptoIndicatorTable.h>

#include "AuditTypes.h"

VOID
LogEcitCoverageReport (
  IN CONST EFI_CRYPTO_INDICATOR_TABLE  *Table,
  IN CONST ECIT_CLAIM_RESULT           *ClaimResults,
  IN CONST ECIT_COVERAGE_STATE         *CoverageRows
  );

#endif
