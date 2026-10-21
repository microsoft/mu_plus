/** @file
  Human-readable ECIT audit coverage reporting.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Guid/CryptoIndicatorTable.h>
#include <Guid/ImageAuthentication.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UnitTestLib.h>

#include "AuditTypes.h"
#include "EcitClaimRegistry.h"
#include "EcitCoverageReport.h"

#define ECIT_AUDIT_TABLE_RULE  "------------------------------------------------------------------------------------------------------------"
#define MAX_OID_TEXT_SIZE      64

typedef enum {
  EcitPayloadOpaque,
  EcitPayloadCsvOids,
  EcitPayloadSignatureListTypes
} ECIT_PAYLOAD_KIND;

typedef struct {
  CONST EFI_GUID       *Feature;
  CONST CHAR8          *Name;
  ECIT_PAYLOAD_KIND    PayloadKind;
} ECIT_FEATURE_REPORT;

typedef struct {
  EFI_GUID       Guid;
  CONST CHAR8    *Name;
} ECIT_SIGNATURE_LIST_NAME;

typedef struct {
  CONST CHAR8    *Oid;
  CONST CHAR8    *Name;
} ECIT_OID_NAME;

typedef struct {
  UINTN    Advertised;
  UINTN    Coverage[EcitCoverageInconclusive + 1];
} ECIT_COVERAGE_TOTALS;

STATIC CONST ECIT_FEATURE_REPORT  mFeatureReports[] = {
  {
    &gEfiEcitFeatureSbImageVerificationGuid,
    "Secure Boot Image Verification",
    EcitPayloadCsvOids
  },
  {
    &gEfiEcitFeatureSbImageAuthorizationGuid,
    "Secure Boot Image Authorization",
    EcitPayloadSignatureListTypes
  },
  {
    &gEfiEcitFeatureSbImageRevocationGuid,
    "Secure Boot Image Revocation",
    EcitPayloadSignatureListTypes
  },
  {
    &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid,
    "Secure Boot Database Update Verification",
    EcitPayloadCsvOids
  },
  {
    &gEfiEcitFeatureSbDatabaseUpdateAuthorizationGuid,
    "Secure Boot Database Update Authorization",
    EcitPayloadSignatureListTypes
  },
  {
    &gEfiEcitFeatureSystemFirmwareUpdateGuid,
    "System Firmware Update",
    EcitPayloadCsvOids
  },
  {
    &gEfiEcitFeatureEsrtFirmwareUpdateGuid,
    "ESRT Device Firmware Update",
    EcitPayloadOpaque
  }
};

STATIC CONST ECIT_SIGNATURE_LIST_NAME  mSignatureListNames[] = {
  { EFI_CERT_X509_GUID,           "X.509 certificate"            },
  { EFI_CERT_V2_X509_GUID,        "X.509 certificate (V2)"       },
  { EFI_CERT_SHA1_GUID,           "SHA-1 image hash"             },
  { EFI_CERT_SHA256_GUID,         "SHA-256 image hash"           },
  { EFI_CERT_V2_SHA256_GUID,      "SHA-256 image hash (V2)"      },
  { EFI_CERT_SHA384_GUID,         "SHA-384 image hash"           },
  { EFI_CERT_V2_SHA384_GUID,      "SHA-384 image hash (V2)"      },
  { EFI_CERT_SHA512_GUID,         "SHA-512 image hash"           },
  { EFI_CERT_V2_SHA512_GUID,      "SHA-512 image hash (V2)"      },
  { EFI_CERT_X509_SHA256_GUID,    "X.509 TBS hash (SHA-256)"     },
  { EFI_CERT_V2_X509_SHA256_GUID, "X.509 TBS hash (SHA-256, V2)" },
  { EFI_CERT_X509_SHA384_GUID,    "X.509 TBS hash (SHA-384)"     },
  { EFI_CERT_V2_X509_SHA384_GUID, "X.509 TBS hash (SHA-384, V2)" },
  { EFI_CERT_X509_SHA512_GUID,    "X.509 TBS hash (SHA-512)"     },
  { EFI_CERT_V2_X509_SHA512_GUID, "X.509 TBS hash (SHA-512, V2)" }
};

STATIC CONST ECIT_OID_NAME  mOidNames[] = {
  { "1.2.840.113549.1.1.4",    "md5WithRSAEncryption"    },
  { "1.2.840.113549.1.1.5",    "sha1WithRSAEncryption"   },
  { "1.2.840.113549.1.1.11",   "sha256WithRSAEncryption" },
  { "1.2.840.113549.1.1.12",   "sha384WithRSAEncryption" },
  { "1.2.840.113549.1.1.13",   "sha512WithRSAEncryption" },
  { "1.2.840.113549.1.1.14",   "sha224WithRSAEncryption" },
  { "1.2.840.10045.4.1",       "ecdsa-with-SHA1"         },
  { "1.2.840.10045.4.3.1",     "ecdsa-with-SHA224"       },
  { "1.2.840.10045.4.3.2",     "ecdsa-with-SHA256"       },
  { "1.2.840.10045.4.3.3",     "ecdsa-with-SHA384"       },
  { "1.2.840.10045.4.3.4",     "ecdsa-with-SHA512"       },
  { "2.16.840.1.101.3.4.3.17", "id-ml-dsa-44"            },
  { "2.16.840.1.101.3.4.3.18", "id-ml-dsa-65"            },
  { "2.16.840.1.101.3.4.3.19", "id-ml-dsa-87"            },
  { "1.3.14.3.2.26",           "sha1"                    },
  { "2.16.840.1.101.3.4.2.1",  "sha256"                  },
  { "2.16.840.1.101.3.4.2.2",  "sha384"                  },
  { "2.16.840.1.101.3.4.2.3",  "sha512"                  },
  { "2.16.840.1.101.3.4.2.4",  "sha224"                  },
  { "1.2.840.113549.2.5",      "md5"                     },
  { "1.2.156.10197.1.401",     "sm3"                     }
};

STATIC
CONST ECIT_FEATURE_REPORT *
FindFeatureReport (
  IN CONST EFI_GUID  *Feature
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mFeatureReports); Index++) {
    if (CompareGuid (Feature, mFeatureReports[Index].Feature)) {
      return &mFeatureReports[Index];
    }
  }

  return NULL;
}

STATIC
CONST CHAR8 *
FindSignatureListName (
  IN CONST EFI_GUID  *Guid
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mSignatureListNames); Index++) {
    if (CompareGuid (Guid, &mSignatureListNames[Index].Guid)) {
      return mSignatureListNames[Index].Name;
    }
  }

  return "Unknown signature-list type";
}

STATIC
CONST CHAR8 *
FindOidName (
  IN CONST CHAR8  *Oid
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mOidNames); Index++) {
    if (AsciiStrCmp (Oid, mOidNames[Index].Oid) == 0) {
      return mOidNames[Index].Name;
    }
  }

  return Oid;
}

STATIC
CONST CHAR8 *
ObservationName (
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
CoverageName (
  IN ECIT_COVERAGE_STATE  Coverage
  )
{
  switch (Coverage) {
    case EcitCoverageVerified:
      return "VERIFIED";
    case EcitCoverageOverReported:
      return "OVER_REPORTED";
    case EcitCoverageUnderReported:
      return "UNDER_REPORTED";
    case EcitCoverageVerifiedAbsent:
      return "VERIFIED_ABSENT";
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
VOID
LogClaim (
  IN UINTN                     Row,
  IN CONST CHAR8               *ClaimName,
  IN CONST CHAR8               *Identifier,
  IN ECIT_OBSERVATION_STATE    Observation,
  IN ECIT_COVERAGE_STATE       Coverage,
  IN OUT ECIT_COVERAGE_TOTALS  *FeatureTotals,
  IN OUT ECIT_COVERAGE_TOTALS  *GrandTotals
  )
{
  AccumulateCoverage (FeatureTotals, Coverage, TRUE);
  AccumulateCoverage (GrandTotals, Coverage, TRUE);

  UT_LOG_INFO (
    "[ECIT_TABLE] %2u  %-26a %-27a %-12a %-30a\n",
    (UINT32)Row,
    ClaimName,
    Identifier,
    ObservationName (Observation),
    CoverageName (Coverage)
    );
}

STATIC
VOID
LogOidClaims (
  IN CONST EFI_GUID             *Feature,
  IN CONST UINT8                *Payload,
  IN UINTN                      PayloadSize,
  IN CONST ECIT_CLAIM_RESULT    *ClaimResults,
  IN CONST ECIT_COVERAGE_STATE  *CoverageRows,
  IN OUT UINTN                  *Row,
  IN OUT ECIT_COVERAGE_TOTALS   *FeatureTotals,
  IN OUT ECIT_COVERAGE_TOTALS   *GrandTotals
  )
{
  ECIT_CLAIM_ID           ClaimId;
  ECIT_COVERAGE_STATE     Coverage;
  ECIT_OBSERVATION_STATE  Observation;
  UINTN                   End;
  CHAR8                   Oid[MAX_OID_TEXT_SIZE];
  UINTN                   Start;
  UINTN                   TokenSize;

  Start = 0;
  while ((Start < PayloadSize) && (Payload[Start] != '\0')) {
    End = Start;
    while ((End < PayloadSize) && (Payload[End] != '\0') && (Payload[End] != ',')) {
      End++;
    }

    TokenSize = End - Start;
    if ((TokenSize == 0) || (TokenSize >= sizeof (Oid))) {
      LogClaim (
        (*Row)++,
        "<malformed OID>",
        "<invalid>",
        EcitObservationInconclusive,
        EcitCoverageInconclusive,
        FeatureTotals,
        GrandTotals
        );
    } else {
      CopyMem (Oid, &Payload[Start], TokenSize);
      Oid[TokenSize] = '\0';
      ClaimId        = FindEcitOidClaim (Feature, Oid);
      if (ClaimId < EcitClaimCount) {
        Observation = ClaimResults[ClaimId].Observation;
        Coverage    = CoverageRows[ClaimId];
      } else {
        Observation = EcitObservationNotTested;
        Coverage    = EcitCoverageNotTested;
      }

      LogClaim (
        (*Row)++,
        FindOidName (Oid),
        Oid,
        Observation,
        Coverage,
        FeatureTotals,
        GrandTotals
        );
    }

    if ((End == PayloadSize) || (Payload[End] == '\0')) {
      break;
    }

    Start = End + 1;
  }
}

STATIC
VOID
LogSignatureListClaims (
  IN CONST EFI_GUID             *Feature,
  IN CONST UINT8                *Payload,
  IN UINTN                      PayloadSize,
  IN CONST ECIT_CLAIM_RESULT    *ClaimResults,
  IN CONST ECIT_COVERAGE_STATE  *CoverageRows,
  IN OUT UINTN                  *Row,
  IN OUT ECIT_COVERAGE_TOTALS   *FeatureTotals,
  IN OUT ECIT_COVERAGE_TOTALS   *GrandTotals
  )
{
  ECIT_CLAIM_ID           ClaimId;
  ECIT_COVERAGE_STATE     Coverage;
  CONST EFI_GUID          *Guid;
  UINTN                   Index;
  ECIT_OBSERVATION_STATE  Observation;

  if ((PayloadSize == 0) || ((PayloadSize % sizeof (EFI_GUID)) != 0)) {
    LogClaim (
      (*Row)++,
      "<malformed signature-list payload>",
      "<invalid>",
      EcitObservationInconclusive,
      EcitCoverageInconclusive,
      FeatureTotals,
      GrandTotals
      );
    return;
  }

  for (Index = 0; Index < PayloadSize; Index += sizeof (EFI_GUID)) {
    Guid    = (CONST EFI_GUID *)&Payload[Index];
    ClaimId = FindEcitSignatureListClaim (Feature, Guid);
    if (ClaimId < EcitClaimCount) {
      Observation = ClaimResults[ClaimId].Observation;
      Coverage    = CoverageRows[ClaimId];
    } else {
      Observation = EcitObservationNotTested;
      Coverage    = EcitCoverageNotTested;
    }

    AccumulateCoverage (FeatureTotals, Coverage, TRUE);
    AccumulateCoverage (GrandTotals, Coverage, TRUE);
    UT_LOG_INFO (
      "[ECIT_TABLE] %2u  %-26a %g %-12a %-30a\n",
      (UINT32)(*Row)++,
      FindSignatureListName (Guid),
      Guid,
      ObservationName (Observation),
      CoverageName (Coverage)
      );
  }
}

STATIC
VOID
LogOpaqueClaim (
  IN CONST EFI_GUID            *Feature,
  IN OUT UINTN                 *Row,
  IN OUT ECIT_COVERAGE_TOTALS  *FeatureTotals,
  IN OUT ECIT_COVERAGE_TOTALS  *GrandTotals
  )
{
  AccumulateCoverage (FeatureTotals, EcitCoverageNotTested, TRUE);
  AccumulateCoverage (GrandTotals, EcitCoverageNotTested, TRUE);
  UT_LOG_INFO (
    "[ECIT_TABLE] %2u  %-26a %g %-12a %-30a\n",
    (UINT32)(*Row)++,
    "Opaque feature payload",
    Feature,
    "NOT_TESTED",
    "NOT_TESTED"
    );
}

STATIC
VOID
LogFeatureSummary (
  IN CONST ECIT_COVERAGE_TOTALS  *Totals
  )
{
  UT_LOG_INFO (
    "[ECIT_TABLE]     Feature summary: advertised=%u verified=%u not_tested=%u limitations=%u mismatches=%u\n",
    Totals->Advertised,
    Totals->Coverage[EcitCoverageVerified],
    Totals->Coverage[EcitCoverageNotTested],
    Totals->Coverage[EcitCoverageNotIndependentlyVerifiable],
    Totals->Coverage[EcitCoverageOverReported] + Totals->Coverage[EcitCoverageUnderReported]
    );
}

STATIC
CONST CHAR8 *
AuditVerdict (
  IN CONST ECIT_COVERAGE_TOTALS  *Totals
  )
{
  if ((Totals->Coverage[EcitCoverageOverReported] != 0) ||
      (Totals->Coverage[EcitCoverageUnderReported] != 0) ||
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

VOID
LogEcitCoverageReport (
  IN CONST EFI_CRYPTO_INDICATOR_TABLE  *Table,
  IN CONST ECIT_CLAIM_RESULT           *ClaimResults,
  IN CONST ECIT_COVERAGE_STATE         *CoverageRows
  )
{
  CONST ECIT_FEATURE_REPORT         *FeatureReport;
  ECIT_COVERAGE_TOTALS              FeatureTotals;
  ECIT_COVERAGE_TOTALS              GrandTotals;
  CONST UINT8                       *Payload;
  UINTN                             PayloadSize;
  CONST EFI_CRYPTO_INDICATOR_ENTRY  *Entry;
  UINTN                             EntryIndex;
  UINTN                             Offset;
  UINTN                             Row;

  ZeroMem (&GrandTotals, sizeof (GrandTotals));
  Row    = 1;
  Offset = sizeof (*Table);

  UT_LOG_INFO ("[ECIT_TABLE] %a\n", ECIT_AUDIT_TABLE_RULE);
  UT_LOG_INFO ("[ECIT_TABLE] ECIT Secure Boot Audit Validation\n");
  UT_LOG_INFO ("[ECIT_TABLE] %a\n", ECIT_AUDIT_TABLE_RULE);
  UT_LOG_INFO (
    "[ECIT_TABLE] %-2a  %-26a %-36a %-12a %-30a\n",
    "#",
    "Advertised claim",
    "Identifier",
    "Observation",
    "Validation"
    );
  UT_LOG_INFO (
    "[ECIT_TABLE] Legend: NOT_TESTED=NO_OPERATIONAL_OBSERVATION; "
    "NOT_INDEPENDENTLY_VERIFIABLE=FLAT_PROFILE_AMBIGUITY\n"
    );

  for (EntryIndex = 0; EntryIndex < Table->NumberOfEntries; EntryIndex++) {
    Entry         = (CONST EFI_CRYPTO_INDICATOR_ENTRY *)((CONST UINT8 *)Table + Offset);
    Payload       = (CONST UINT8 *)(Entry + 1);
    PayloadSize   = Entry->EntryLength - sizeof (*Entry);
    FeatureReport = FindFeatureReport (&Entry->FeatureIdentifier);
    ZeroMem (&FeatureTotals, sizeof (FeatureTotals));

    UT_LOG_INFO ("[ECIT_TABLE] %a\n", ECIT_AUDIT_TABLE_RULE);
    if (FeatureReport == NULL) {
      UT_LOG_INFO ("[ECIT_TABLE] Feature: Unknown (%g)\n", &Entry->FeatureIdentifier);
      LogOpaqueClaim (&Entry->FeatureIdentifier, &Row, &FeatureTotals, &GrandTotals);
    } else {
      UT_LOG_INFO ("[ECIT_TABLE] Feature: %a\n", FeatureReport->Name);
      switch (FeatureReport->PayloadKind) {
        case EcitPayloadCsvOids:
          LogOidClaims (
            &Entry->FeatureIdentifier,
            Payload,
            PayloadSize,
            ClaimResults,
            CoverageRows,
            &Row,
            &FeatureTotals,
            &GrandTotals
            );
          break;

        case EcitPayloadSignatureListTypes:
          LogSignatureListClaims (
            &Entry->FeatureIdentifier,
            Payload,
            PayloadSize,
            ClaimResults,
            CoverageRows,
            &Row,
            &FeatureTotals,
            &GrandTotals
            );
          break;

        default:
          LogOpaqueClaim (&Entry->FeatureIdentifier, &Row, &FeatureTotals, &GrandTotals);
          break;
      }
    }

    LogFeatureSummary (&FeatureTotals);
    Offset += Entry->EntryLength;
  }

  UT_LOG_INFO ("[ECIT_TABLE] %a\n", ECIT_AUDIT_TABLE_RULE);
  UT_LOG_INFO ("[ECIT_TABLE] Cross-field candidate profiles\n");
  AccumulateCoverage (&GrandTotals, EcitCoverageNotIndependentlyVerifiable, FALSE);
  UT_LOG_INFO (
    "[ECIT_TABLE] %2u  %-26a %-36a %-12a %-30a\n",
    (UINT32)Row,
    "sha512-with-mldsa87",
    "candidate profile",
    ObservationName (ClaimResults[EcitImageVerificationMldsa87Sha512Profile].Observation),
    "NOT_INDEPENDENTLY_VERIFIABLE"
    );
  UT_LOG_INFO ("[ECIT_TABLE] %a\n", ECIT_AUDIT_TABLE_RULE);
  UT_LOG_INFO (
    "[ECIT_TABLE] Verdict: %a | Advertised claims: %u | Verified: %u | Not tested: %u | Limitations: %u\n",
    AuditVerdict (&GrandTotals),
    GrandTotals.Advertised,
    GrandTotals.Coverage[EcitCoverageVerified],
    GrandTotals.Coverage[EcitCoverageNotTested],
    GrandTotals.Coverage[EcitCoverageNotIndependentlyVerifiable]
    );
  UT_LOG_INFO (
    "[ECIT_TABLE] Mismatches: %u | Inconclusive: %u | Verified absent: %u\n",
    GrandTotals.Coverage[EcitCoverageOverReported] + GrandTotals.Coverage[EcitCoverageUnderReported],
    GrandTotals.Coverage[EcitCoverageInconclusive],
    GrandTotals.Coverage[EcitCoverageVerifiedAbsent]
    );
  UT_LOG_INFO ("[ECIT_TABLE] %a\n", ECIT_AUDIT_TABLE_RULE);
  UT_LOG_INFO (
    "[ECIT_SUMMARY] advertised_claims=%u verified=%u not_tested=%u "
    "not_independently_verifiable=%u over_reported=%u under_reported=%u inconclusive=%u verdict=%a\n",
    GrandTotals.Advertised,
    GrandTotals.Coverage[EcitCoverageVerified],
    GrandTotals.Coverage[EcitCoverageNotTested],
    GrandTotals.Coverage[EcitCoverageNotIndependentlyVerifiable],
    GrandTotals.Coverage[EcitCoverageOverReported],
    GrandTotals.Coverage[EcitCoverageUnderReported],
    GrandTotals.Coverage[EcitCoverageInconclusive],
    AuditVerdict (&GrandTotals)
    );
}
