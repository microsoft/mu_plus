/** @file
  Structured ECIT audit evidence emission.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Guid/CryptoIndicatorTable.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/UnitTestLib.h>

#include "AuditTypes.h"
#include "EcitClaimRegistry.h"
#include "EcitEvidence.h"

#define ECIT_EVIDENCE_VALUE_SIZE  384
#define MAX_OID_TEXT_SIZE         64
#define MAX_PROFILE_OIDS          32

typedef enum {
  EcitPayloadOpaque,
  EcitPayloadCsvOids,
  EcitPayloadSignatureListTypes
} ECIT_PAYLOAD_KIND;

typedef struct {
  CONST EFI_GUID       *Feature;
  ECIT_PAYLOAD_KIND    PayloadKind;
} ECIT_FEATURE_KIND;

typedef struct {
  CONST CHAR8    *Oid;
  BOOLEAN        Digest;
} ECIT_OID_KIND;

typedef struct {
  CHAR8      Oid[MAX_OID_TEXT_SIZE];
  BOOLEAN    Digest;
} ECIT_PROFILE_OID;

typedef struct {
  UINTN    Advertised;
  UINTN    Coverage[EcitCoverageInconclusive + 1];
} ECIT_COVERAGE_TOTALS;

STATIC CONST ECIT_FEATURE_KIND  mFeatureKinds[] = {
  { &gEfiEcitFeatureSbImageVerificationGuid,           EcitPayloadCsvOids            },
  { &gEfiEcitFeatureSbImageAuthorizationGuid,          EcitPayloadSignatureListTypes },
  { &gEfiEcitFeatureSbImageRevocationGuid,             EcitPayloadSignatureListTypes },
  { &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid,  EcitPayloadCsvOids            },
  { &gEfiEcitFeatureSbDatabaseUpdateAuthorizationGuid, EcitPayloadSignatureListTypes },
  { &gEfiEcitFeatureSystemFirmwareUpdateGuid,          EcitPayloadCsvOids            },
  { &gEfiEcitFeatureEsrtFirmwareUpdateGuid,            EcitPayloadOpaque             }
};

STATIC CONST ECIT_OID_KIND  mOidKinds[] = {
  { "1.2.840.113549.1.1.4",    FALSE },
  { "1.2.840.113549.1.1.5",    FALSE },
  { "1.2.840.113549.1.1.11",   FALSE },
  { "1.2.840.113549.1.1.12",   FALSE },
  { "1.2.840.113549.1.1.13",   FALSE },
  { "1.2.840.113549.1.1.14",   FALSE },
  { "1.2.840.10045.4.1",       FALSE },
  { "1.2.840.10045.4.3.1",     FALSE },
  { "1.2.840.10045.4.3.2",     FALSE },
  { "1.2.840.10045.4.3.3",     FALSE },
  { "1.2.840.10045.4.3.4",     FALSE },
  { "2.16.840.1.101.3.4.3.17", FALSE },
  { "2.16.840.1.101.3.4.3.18", FALSE },
  { "2.16.840.1.101.3.4.3.19", FALSE },
  { "1.3.14.3.2.26",           TRUE  },
  { "2.16.840.1.101.3.4.2.1",  TRUE  },
  { "2.16.840.1.101.3.4.2.2",  TRUE  },
  { "2.16.840.1.101.3.4.2.3",  TRUE  },
  { "2.16.840.1.101.3.4.2.4",  TRUE  },
  { "1.2.840.113549.2.5",      TRUE  },
  { "1.2.156.10197.1.401",     TRUE  }
};

STATIC
ECIT_PAYLOAD_KIND
FindPayloadKind (
  IN CONST EFI_GUID  *Feature
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mFeatureKinds); Index++) {
    if (CompareGuid (Feature, mFeatureKinds[Index].Feature)) {
      return mFeatureKinds[Index].PayloadKind;
    }
  }

  return EcitPayloadOpaque;
}

STATIC
CONST CHAR8 *
PayloadKindName (
  IN ECIT_PAYLOAD_KIND  PayloadKind
  )
{
  switch (PayloadKind) {
    case EcitPayloadCsvOids:
      return "OIDS";
    case EcitPayloadSignatureListTypes:
      return "GUIDS";
    default:
      return "OPAQUE";
  }
}

STATIC
BOOLEAN
FindOidDigestKind (
  IN CONST CHAR8  *Oid,
  OUT BOOLEAN     *Digest
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mOidKinds); Index++) {
    if (AsciiStrCmp (Oid, mOidKinds[Index].Oid) == 0) {
      *Digest = mOidKinds[Index].Digest;
      return TRUE;
    }
  }

  return FALSE;
}

STATIC
CONST CHAR8 *
ClaimStateName (
  IN ECIT_CLAIM_STATE  Claim
  )
{
  switch (Claim) {
    case EcitClaimAdvertised:
      return "ADVERTISED";
    case EcitClaimAbsent:
      return "ABSENT";
    default:
      return "UNKNOWN";
  }
}

STATIC
CONST CHAR8 *
ObservationStateName (
  IN ECIT_OBSERVATION_STATE  Observation
  )
{
  switch (Observation) {
    case EcitObservationSuccess:
      return "SUCCESS";
    case EcitObservationFailure:
      return "FAILURE";
    case EcitObservationInconclusive:
      return "INCONCLUSIVE";
    default:
      return "NOT_TESTED";
  }
}

STATIC
CONST CHAR8 *
CoverageStateName (
  IN ECIT_COVERAGE_STATE  Coverage
  )
{
  switch (Coverage) {
    case EcitCoverageVerified:
      return "VERIFIED";
    case EcitCoverageOverReported:
      return "OVER_REPORTED";
    case EcitCoverageUnadvertisedSuccess:
      return "UNADVERTISED_SUCCESS";
    case EcitCoverageVerifiedAbsent:
      return "VERIFIED_ABSENT";
    case EcitCoverageNotAdvertised:
      return "NOT_ADVERTISED";
    case EcitCoverageNotIndependentlyVerifiable:
      return "NOT_INDEPENDENTLY_VERIFIABLE";
    case EcitCoverageNotTested:
      return "NOT_TESTED";
    default:
      return "INCONCLUSIVE";
  }
}

STATIC
VOID
AccumulateCoverage (
  IN OUT ECIT_COVERAGE_TOTALS  *Totals,
  IN ECIT_COVERAGE_STATE       Coverage,
  IN BOOLEAN                   Advertised
  )
{
  if (Advertised) {
    Totals->Advertised++;
  }

  Totals->Coverage[Coverage]++;
}

STATIC
CONST CHAR8 *
AuditVerdict (
  IN CONST ECIT_COVERAGE_TOTALS  *Totals
  )
{
  if ((Totals->Coverage[EcitCoverageOverReported] != 0) ||
      (Totals->Coverage[EcitCoverageInconclusive] != 0))
  {
    return "FAIL";
  }

  if ((Totals->Coverage[EcitCoverageNotIndependentlyVerifiable] != 0) ||
      (Totals->Coverage[EcitCoverageNotTested] != 0))
  {
    return "PASS_WITH_LIMITATIONS";
  }

  return "PASS";
}

STATIC
VOID
FlushAdvertisedValues (
  IN UINTN        FeatureIndex,
  IN CONST CHAR8  *IdentifierKind,
  IN OUT CHAR8    *Values,
  IN OUT UINTN    *RecordCount
  )
{
  if (Values[0] == '\0') {
    return;
  }

  UT_LOG_INFO (
    "[ECIT_V1] t=ads f=%u k=%a v=%a\n",
    FeatureIndex,
    IdentifierKind,
    Values
    );
  (*RecordCount)++;
  Values[0] = '\0';
}

STATIC
VOID
AppendAdvertisedValue (
  IN UINTN        FeatureIndex,
  IN CONST CHAR8  *IdentifierKind,
  IN CONST CHAR8  *Value,
  IN OUT CHAR8    *Values,
  IN OUT UINTN    *RecordCount
  )
{
  UINTN  CurrentSize;
  UINTN  ValueSize;

  CurrentSize = AsciiStrLen (Values);
  ValueSize   = AsciiStrLen (Value);
  if ((CurrentSize != 0) &&
      ((CurrentSize + 1 + ValueSize) >= ECIT_EVIDENCE_VALUE_SIZE))
  {
    FlushAdvertisedValues (
      FeatureIndex,
      IdentifierKind,
      Values,
      RecordCount
      );
    CurrentSize = 0;
  }

  if (CurrentSize != 0) {
    Values[CurrentSize++] = ',';
  }

  CopyMem (&Values[CurrentSize], Value, ValueSize + 1);
}

STATIC
VOID
AccumulateClaim (
  IN ECIT_CLAIM_ID              ClaimId,
  IN CONST ECIT_COVERAGE_STATE  *CoverageRows,
  IN OUT ECIT_COVERAGE_TOTALS   *FeatureTotals,
  IN OUT ECIT_COVERAGE_TOTALS   *GrandTotals
  )
{
  ECIT_COVERAGE_STATE  Coverage;

  Coverage = (ClaimId < EcitClaimCount) ?
             CoverageRows[ClaimId] :
             EcitCoverageNotTested;
  AccumulateCoverage (FeatureTotals, Coverage, TRUE);
  AccumulateCoverage (GrandTotals, Coverage, TRUE);
}

STATIC
VOID
EmitOidEvidence (
  IN UINTN                      FeatureIndex,
  IN CONST EFI_GUID             *Feature,
  IN CONST UINT8                *Payload,
  IN UINTN                      PayloadSize,
  IN CONST ECIT_COVERAGE_STATE  *CoverageRows,
  IN OUT ECIT_COVERAGE_TOTALS   *FeatureTotals,
  IN OUT ECIT_COVERAGE_TOTALS   *GrandTotals,
  IN OUT UINTN                  *RecordCount
  )
{
  ECIT_CLAIM_ID     ClaimId;
  UINTN             DigestIndex;
  UINTN             End;
  ECIT_PROFILE_OID  Oids[MAX_PROFILE_OIDS];
  UINTN             OidCount;
  UINTN             ProfileCount;
  UINTN             SignatureIndex;
  UINTN             Start;
  CHAR8             Token[MAX_OID_TEXT_SIZE];
  UINTN             TokenSize;
  UINTN             UntestedCount;
  CHAR8             Values[ECIT_EVIDENCE_VALUE_SIZE];

  OidCount      = 0;
  ProfileCount  = 0;
  Start         = 0;
  UntestedCount = 0;
  Values[0]     = '\0';

  while ((Start < PayloadSize) && (Payload[Start] != '\0')) {
    End = Start;
    while ((End < PayloadSize) && (Payload[End] != '\0') &&
           (Payload[End] != ','))
    {
      End++;
    }

    TokenSize = End - Start;
    if ((TokenSize > 0) && (TokenSize < sizeof (Token))) {
      CopyMem (Token, &Payload[Start], TokenSize);
      Token[TokenSize] = '\0';
      AppendAdvertisedValue (
        FeatureIndex,
        "OID",
        Token,
        Values,
        RecordCount
        );
      ClaimId = FindEcitOidClaim (Feature, Token);
      AccumulateClaim (
        ClaimId,
        CoverageRows,
        FeatureTotals,
        GrandTotals
        );

      if ((OidCount < ARRAY_SIZE (Oids)) &&
          FindOidDigestKind (Token, &Oids[OidCount].Digest))
      {
        AsciiStrCpyS (Oids[OidCount].Oid, sizeof (Oids[OidCount].Oid), Token);
        OidCount++;
      }
    }

    if ((End == PayloadSize) || (Payload[End] == '\0')) {
      break;
    }

    Start = End + 1;
  }

  FlushAdvertisedValues (
    FeatureIndex,
    "OID",
    Values,
    RecordCount
    );

  for (SignatureIndex = 0; SignatureIndex < OidCount; SignatureIndex++) {
    if (Oids[SignatureIndex].Digest) {
      continue;
    }

    for (DigestIndex = 0; DigestIndex < OidCount; DigestIndex++) {
      if (!Oids[DigestIndex].Digest) {
        continue;
      }

      ClaimId = FindEcitOidPairClaim (
                  Feature,
                  Oids[SignatureIndex].Oid,
                  Oids[DigestIndex].Oid
                  );
      AccumulateClaim (
        ClaimId,
        CoverageRows,
        FeatureTotals,
        GrandTotals
        );
      if (ClaimId >= EcitClaimCount) {
        UntestedCount++;
      }

      ProfileCount++;
    }
  }

  UT_LOG_INFO (
    "[ECIT_V1] t=derived f=%u advertised=%u tested=%u not_tested=%u\n",
    FeatureIndex,
    ProfileCount,
    ProfileCount - UntestedCount,
    UntestedCount
    );
  (*RecordCount)++;
}

STATIC
VOID
EmitGuidEvidence (
  IN UINTN                      FeatureIndex,
  IN CONST EFI_GUID             *Feature,
  IN CONST UINT8                *Payload,
  IN UINTN                      PayloadSize,
  IN CONST ECIT_COVERAGE_STATE  *CoverageRows,
  IN OUT ECIT_COVERAGE_TOTALS   *FeatureTotals,
  IN OUT ECIT_COVERAGE_TOTALS   *GrandTotals,
  IN OUT UINTN                  *RecordCount
  )
{
  ECIT_CLAIM_ID   ClaimId;
  CONST EFI_GUID  *Guid;
  CHAR8           GuidText[37];
  UINTN           Index;
  CHAR8           Values[ECIT_EVIDENCE_VALUE_SIZE];

  Values[0] = '\0';
  for (Index = 0; Index < PayloadSize; Index += sizeof (EFI_GUID)) {
    Guid = (CONST EFI_GUID *)&Payload[Index];
    AsciiSPrint (GuidText, sizeof (GuidText), "%g", Guid);
    AppendAdvertisedValue (
      FeatureIndex,
      "GUID",
      GuidText,
      Values,
      RecordCount
      );
    ClaimId = FindEcitSignatureListClaim (Feature, Guid);
    AccumulateClaim (
      ClaimId,
      CoverageRows,
      FeatureTotals,
      GrandTotals
      );
  }

  FlushAdvertisedValues (
    FeatureIndex,
    "GUID",
    Values,
    RecordCount
    );
}

STATIC
VOID
EmitFeatureEvidence (
  IN UINTN                             FeatureIndex,
  IN CONST EFI_CRYPTO_INDICATOR_ENTRY  *Entry,
  IN CONST UINT8                       *Payload,
  IN UINTN                             PayloadSize,
  IN CONST ECIT_COVERAGE_STATE         *CoverageRows,
  IN OUT ECIT_COVERAGE_TOTALS          *GrandTotals,
  IN OUT UINTN                         *RecordCount
  )
{
  ECIT_COVERAGE_TOTALS  FeatureTotals;
  ECIT_PAYLOAD_KIND     PayloadKind;

  ZeroMem (&FeatureTotals, sizeof (FeatureTotals));
  PayloadKind = FindPayloadKind (&Entry->FeatureIdentifier);
  UT_LOG_INFO (
    "[ECIT_V1] t=feature i=%u id=%g kind=%a\n",
    FeatureIndex,
    &Entry->FeatureIdentifier,
    PayloadKindName (PayloadKind)
    );
  (*RecordCount)++;

  switch (PayloadKind) {
    case EcitPayloadCsvOids:
      EmitOidEvidence (
        FeatureIndex,
        &Entry->FeatureIdentifier,
        Payload,
        PayloadSize,
        CoverageRows,
        &FeatureTotals,
        GrandTotals,
        RecordCount
        );
      break;
    case EcitPayloadSignatureListTypes:
      EmitGuidEvidence (
        FeatureIndex,
        &Entry->FeatureIdentifier,
        Payload,
        PayloadSize,
        CoverageRows,
        &FeatureTotals,
        GrandTotals,
        RecordCount
        );
      break;
    default:
      AccumulateCoverage (
        &FeatureTotals,
        EcitCoverageNotTested,
        TRUE
        );
      AccumulateCoverage (
        GrandTotals,
        EcitCoverageNotTested,
        TRUE
        );
      break;
  }

  UT_LOG_INFO (
    "[ECIT_V1] t=feature_summary f=%u advertised=%u verified=%u "
    "not_tested=%u limitations=%u mismatches=%u\n",
    FeatureIndex,
    FeatureTotals.Advertised,
    FeatureTotals.Coverage[EcitCoverageVerified],
    FeatureTotals.Coverage[EcitCoverageNotTested],
    FeatureTotals.Coverage[EcitCoverageNotIndependentlyVerifiable],
    FeatureTotals.Coverage[EcitCoverageOverReported]
    );
  (*RecordCount)++;
}

VOID
LogEcitEvidence (
  IN CONST EFI_CRYPTO_INDICATOR_TABLE  *Table OPTIONAL,
  IN CONST ECIT_CLAIM_RESULT           *ClaimResults,
  IN CONST ECIT_COVERAGE_STATE         *CoverageRows,
  IN UINTN                             ClaimResultCount,
  IN EFI_STATUS                        ParseStatus
  )
{
  ECIT_COVERAGE_TOTALS              GrandTotals;
  CONST UINT8                       *Payload;
  UINTN                             PayloadSize;
  CONST EFI_CRYPTO_INDICATOR_ENTRY  *Entry;
  UINTN                             EntryIndex;
  UINTN                             FeatureCount;
  UINTN                             Index;
  UINTN                             Offset;
  UINTN                             RecordCount;

  FeatureCount = (Table == NULL) ? 0 : Table->NumberOfEntries;
  RecordCount  = 1;
  UT_LOG_INFO (
    "[ECIT_V1] t=begin features=%u results=%u parse=0x%lx\n",
    FeatureCount,
    ClaimResultCount,
    (UINT64)ParseStatus
    );

  ZeroMem (&GrandTotals, sizeof (GrandTotals));
  Offset = (Table == NULL) ? 0 : sizeof (*Table);
  for (EntryIndex = 0; EntryIndex < FeatureCount; EntryIndex++) {
    Entry       = (CONST EFI_CRYPTO_INDICATOR_ENTRY *)((CONST UINT8 *)Table + Offset);
    Payload     = (CONST UINT8 *)(Entry + 1);
    PayloadSize = Entry->EntryLength - sizeof (*Entry);
    EmitFeatureEvidence (
      EntryIndex + 1,
      Entry,
      Payload,
      PayloadSize,
      CoverageRows,
      &GrandTotals,
      &RecordCount
      );
    Offset += Entry->EntryLength;
  }

  UT_LOG_INFO (
    "[ECIT_V1] t=summary verdict=%a advertised=%u verified=%u "
    "not_tested=%u unverifiable=%u over_reported=%u "
    "unadvertised_success=%u inconclusive=%u verified_absent=%u\n",
    AuditVerdict (&GrandTotals),
    GrandTotals.Advertised,
    GrandTotals.Coverage[EcitCoverageVerified],
    GrandTotals.Coverage[EcitCoverageNotTested],
    GrandTotals.Coverage[EcitCoverageNotIndependentlyVerifiable],
    GrandTotals.Coverage[EcitCoverageOverReported],
    GrandTotals.Coverage[EcitCoverageUnadvertisedSuccess],
    GrandTotals.Coverage[EcitCoverageInconclusive],
    GrandTotals.Coverage[EcitCoverageVerifiedAbsent]
    );
  RecordCount++;

  for (Index = 0; Index < ClaimResultCount; Index++) {
    UT_LOG_INFO (
      "[ECIT_V1] t=result n=%a c=%a o=%a v=%a s=0x%lx\n",
      ClaimResults[Index].Name,
      ClaimStateName (ClaimResults[Index].Claim),
      ObservationStateName (ClaimResults[Index].Observation),
      CoverageStateName (CoverageRows[Index]),
      (UINT64)ClaimResults[Index].ObservationStatus
      );
    RecordCount++;
  }

  UT_LOG_INFO (
    "[ECIT_V1] t=end features=%u results=%u records=%u\n",
    FeatureCount,
    ClaimResultCount,
    RecordCount + 1
    );
}
