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

CONST CHAR8 *
EcitClaimStateName (
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

CONST CHAR8 *
EcitObservationStateName (
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

CONST CHAR8 *
EcitCoverageStateName (
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
    "[ECIT_V2] t=ads f=%u k=%a v=%a\n",
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

EFI_STATUS
GetEcitProfileCounts (
  IN CONST EFI_CRYPTO_INDICATOR_TABLE  *Table,
  IN CONST EFI_GUID                    *Feature,
  IN CONST ECIT_COVERAGE_STATE         *CoverageRows,
  OUT ECIT_PROFILE_COUNTS              *Counts
  )
{
  ECIT_CLAIM_ID                     ClaimId;
  UINTN                             DigestIndex;
  UINTN                             End;
  CONST EFI_CRYPTO_INDICATOR_ENTRY  *Entry;
  UINTN                             EntryIndex;
  ECIT_PROFILE_OID                  Oids[MAX_PROFILE_OIDS];
  UINTN                             OidCount;
  UINTN                             Offset;
  CONST UINT8                       *Payload;
  UINTN                             PayloadSize;
  UINTN                             SignatureIndex;
  UINTN                             Start;
  CHAR8                             Token[MAX_OID_TEXT_SIZE];
  UINTN                             TokenSize;

  if ((Table == NULL) || (Feature == NULL) ||
      (CoverageRows == NULL) || (Counts == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Counts, sizeof (*Counts));
  Offset = sizeof (*Table);
  for (EntryIndex = 0; EntryIndex < Table->NumberOfEntries; EntryIndex++) {
    if ((Offset > Table->Length) ||
        ((Table->Length - Offset) < sizeof (*Entry)))
    {
      return EFI_COMPROMISED_DATA;
    }

    Entry = (CONST EFI_CRYPTO_INDICATOR_ENTRY *)((CONST UINT8 *)Table + Offset);
    if ((Entry->EntryLength < sizeof (*Entry)) ||
        (Entry->EntryLength > (Table->Length - Offset)))
    {
      return EFI_COMPROMISED_DATA;
    }

    if (!CompareGuid (&Entry->FeatureIdentifier, Feature)) {
      Offset += Entry->EntryLength;
      continue;
    }

    OidCount    = 0;
    Payload     = (CONST UINT8 *)(Entry + 1);
    PayloadSize = Entry->EntryLength - sizeof (*Entry);
    Start       = 0;
    while ((Start < PayloadSize) && (Payload[Start] != '\0')) {
      End = Start;
      while ((End < PayloadSize) && (Payload[End] != '\0') &&
             (Payload[End] != ','))
      {
        End++;
      }

      TokenSize = End - Start;
      if ((TokenSize > 0) && (TokenSize < sizeof (Token)) &&
          (OidCount < ARRAY_SIZE (Oids)))
      {
        CopyMem (Token, &Payload[Start], TokenSize);
        Token[TokenSize] = '\0';
        if (FindOidDigestKind (Token, &Oids[OidCount].Digest)) {
          AsciiStrCpyS (
            Oids[OidCount].Oid,
            sizeof (Oids[OidCount].Oid),
            Token
            );
          OidCount++;
        }
      }

      if ((End == PayloadSize) || (Payload[End] == '\0')) {
        break;
      }

      Start = End + 1;
    }

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
        Counts->Advertised++;
        if (ClaimId >= EcitClaimCount) {
          Counts->NotTested++;
        } else if (CoverageRows[ClaimId] == EcitCoverageVerified) {
          Counts->Tested++;
        } else if ((CoverageRows[ClaimId] == EcitCoverageNotTested) ||
                   (CoverageRows[ClaimId] ==
                    EcitCoverageNotIndependentlyVerifiable))
        {
          Counts->NotTested++;
        } else {
          Counts->Mismatches++;
        }
      }
    }

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

STATIC
VOID
EmitOidEvidence (
  IN UINTN           FeatureIndex,
  IN CONST EFI_GUID  *Feature,
  IN CONST UINT8     *Payload,
  IN UINTN           PayloadSize,
  IN OUT UINTN       *RecordCount
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
  UINTN             UnmodeledCount;
  CHAR8             Values[ECIT_EVIDENCE_VALUE_SIZE];

  OidCount       = 0;
  ProfileCount   = 0;
  Start          = 0;
  UnmodeledCount = 0;
  Values[0]      = '\0';

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
      if (ClaimId >= EcitClaimCount) {
        UnmodeledCount++;
      }

      ProfileCount++;
    }
  }

  UT_LOG_INFO (
    "[ECIT_V2] t=derived f=%u advertised=%u modeled=%u unmodeled=%u\n",
    FeatureIndex,
    ProfileCount,
    ProfileCount - UnmodeledCount,
    UnmodeledCount
    );
  (*RecordCount)++;
}

STATIC
VOID
EmitGuidEvidence (
  IN UINTN        FeatureIndex,
  IN CONST UINT8  *Payload,
  IN UINTN        PayloadSize,
  IN OUT UINTN    *RecordCount
  )
{
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
  IN OUT UINTN                         *RecordCount
  )
{
  ECIT_PAYLOAD_KIND  PayloadKind;

  PayloadKind = FindPayloadKind (&Entry->FeatureIdentifier);
  UT_LOG_INFO (
    "[ECIT_V2] t=feature i=%u id=%g kind=%a\n",
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
        RecordCount
        );
      break;
    case EcitPayloadSignatureListTypes:
      EmitGuidEvidence (
        FeatureIndex,
        Payload,
        PayloadSize,
        RecordCount
        );
      break;
    default:
      break;
  }
}

VOID
LogEcitEvidence (
  IN CONST EFI_CRYPTO_INDICATOR_TABLE  *Table OPTIONAL,
  IN EFI_STATUS                        ParseStatus
  )
{
  CONST UINT8                       *Payload;
  UINTN                             PayloadSize;
  CONST EFI_CRYPTO_INDICATOR_ENTRY  *Entry;
  UINTN                             EntryIndex;
  UINTN                             FeatureCount;
  UINTN                             Offset;
  UINTN                             RecordCount;

  FeatureCount = (Table == NULL) ? 0 : Table->NumberOfEntries;
  RecordCount  = 1;
  UT_LOG_INFO (
    "[ECIT_V2] t=begin features=%u parse=0x%lx\n",
    FeatureCount,
    (UINT64)ParseStatus
    );

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
      &RecordCount
      );
    Offset += Entry->EntryLength;
  }

  UT_LOG_INFO (
    "[ECIT_V2] t=end features=%u records=%u\n",
    FeatureCount,
    RecordCount + 1
    );
}
