/** @file
  ECIT claim descriptors and observation registry.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ECIT_CLAIM_REGISTRY_H_
#define ECIT_CLAIM_REGISTRY_H_

#include <Uefi.h>

#include <Guid/CryptoIndicatorTable.h>

#include "AuditTypes.h"

VOID
InitializeEcitClaimResults (
  OUT ECIT_CLAIM_RESULT  *ClaimResults,
  IN UINTN               ClaimResultCount
  );

EFI_STATUS
PopulateEcitClaimStates (
  IN CONST EFI_CRYPTO_INDICATOR_TABLE  *Table,
  IN OUT ECIT_CLAIM_RESULT             *ClaimResults,
  IN UINTN                             ClaimResultCount
  );

ECIT_CLAIM_ID
FindEcitOidClaim (
  IN CONST EFI_GUID  *Feature,
  IN CONST CHAR8     *Oid
  );

ECIT_CLAIM_ID
FindEcitOidPairClaim (
  IN CONST EFI_GUID  *Feature,
  IN CONST CHAR8     *SignatureOid,
  IN CONST CHAR8     *DigestOid
  );

ECIT_CLAIM_ID
FindEcitSignatureListClaim (
  IN CONST EFI_GUID  *Feature,
  IN CONST EFI_GUID  *SignatureListType
  );

VOID
RecordEcitClaimObservation (
  IN OUT ECIT_CLAIM_RESULT   *ClaimResults,
  IN UINTN                   ClaimResultCount,
  IN ECIT_CLAIM_ID           ClaimId,
  IN ECIT_OBSERVATION_STATE  Observation,
  IN EFI_STATUS              Status
  );

VOID
RecordEcitClaimObservations (
  IN OUT ECIT_CLAIM_RESULT   *ClaimResults,
  IN UINTN                   ClaimResultCount,
  IN CONST ECIT_CLAIM_ID     *ClaimIds,
  IN UINTN                   ClaimCount,
  IN ECIT_OBSERVATION_STATE  Observation,
  IN EFI_STATUS              Status
  );

#endif
