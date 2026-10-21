/** @file
  ECIT claim descriptors and observation registry.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Guid/CryptoIndicatorTable.h>
#include <Guid/ImageAuthentication.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include "AuditTypes.h"
#include "EcitClaimRegistry.h"

typedef enum {
  EcitClaimIdentifierNone,
  EcitClaimIdentifierOid,
  EcitClaimIdentifierGuid
} ECIT_CLAIM_IDENTIFIER_KIND;

typedef struct {
  ECIT_CLAIM_ID                 ClaimId;
  CONST CHAR8                   *Name;
  CONST EFI_GUID                *Feature;
  ECIT_CLAIM_IDENTIFIER_KIND    IdentifierKind;
  CONST CHAR8                   *Oid;
  CONST EFI_GUID                *SignatureListType;
  BOOLEAN                       CandidateProfile;
} ECIT_CLAIM_DESCRIPTOR;

STATIC CONST ECIT_CLAIM_DESCRIPTOR  mClaimDescriptors[] = {
  {
    EcitDbUpdateAuthorizationX509,
    "db-update-authorization:x509",
    &gEfiEcitFeatureSbDatabaseUpdateAuthorizationGuid,
    EcitClaimIdentifierGuid,
    NULL,
    &gEfiCertX509Guid,
    FALSE
  },
  {
    EcitDbUpdateVerificationRsaSha256,
    "db-update-verification:rsa-sha256",
    &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid,
    EcitClaimIdentifierOid,
    "1.2.840.113549.1.1.11",
    NULL,
    FALSE
  },
  {
    EcitDbUpdateVerificationSha256,
    "db-update-verification:sha256",
    &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid,
    EcitClaimIdentifierOid,
    "2.16.840.1.101.3.4.2.1",
    NULL,
    FALSE
  },
  {
    EcitDbUpdateVerificationMldsa87,
    "db-update-verification:mldsa87",
    &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid,
    EcitClaimIdentifierOid,
    "2.16.840.1.101.3.4.3.19",
    NULL,
    FALSE
  },
  {
    EcitDbUpdateVerificationSha512,
    "db-update-verification:sha512",
    &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid,
    EcitClaimIdentifierOid,
    "2.16.840.1.101.3.4.2.3",
    NULL,
    FALSE
  },
  {
    EcitImageAuthorizationTbsV1Sha512,
    "image-authorization:tbs-v1-sha512",
    &gEfiEcitFeatureSbImageAuthorizationGuid,
    EcitClaimIdentifierGuid,
    NULL,
    &gEfiCertX509Sha512Guid,
    FALSE
  },
  {
    EcitImageAuthorizationTbsV2Sha512,
    "image-authorization:tbs-v2-sha512",
    &gEfiEcitFeatureSbImageAuthorizationGuid,
    EcitClaimIdentifierGuid,
    NULL,
    &gEfiCertV2X509Sha512Guid,
    FALSE
  },
  {
    EcitImageVerificationMldsa87,
    "image-verification:mldsa87",
    &gEfiEcitFeatureSbImageVerificationGuid,
    EcitClaimIdentifierOid,
    "2.16.840.1.101.3.4.3.19",
    NULL,
    FALSE
  },
  {
    EcitImageVerificationSha512,
    "image-verification:sha512",
    &gEfiEcitFeatureSbImageVerificationGuid,
    EcitClaimIdentifierOid,
    "2.16.840.1.101.3.4.2.3",
    NULL,
    FALSE
  },
  {
    EcitImageVerificationMldsa87Sha512Profile,
    "image-verification-profile:sha512-with-mldsa87",
    &gEfiEcitFeatureSbImageVerificationGuid,
    EcitClaimIdentifierNone,
    NULL,
    NULL,
    TRUE
  }
};

STATIC_ASSERT (
  ARRAY_SIZE (mClaimDescriptors) == EcitClaimCount,
  "Every ECIT claim ID must have exactly one descriptor"
  );

STATIC
BOOLEAN
CsvContainsToken (
  IN CONST UINT8  *Csv,
  IN UINTN        CsvSize,
  IN CONST CHAR8  *Token
  )
{
  UINTN  End;
  UINTN  Start;
  UINTN  TokenSize;

  TokenSize = AsciiStrLen (Token);
  Start     = 0;
  while ((Start < CsvSize) && (Csv[Start] != '\0')) {
    End = Start;
    while ((End < CsvSize) && (Csv[End] != '\0') && (Csv[End] != ',')) {
      End++;
    }

    if (((End - Start) == TokenSize) && (CompareMem (&Csv[Start], Token, TokenSize) == 0)) {
      return TRUE;
    }

    if ((End == CsvSize) || (Csv[End] == '\0')) {
      break;
    }

    Start = End + 1;
  }

  return FALSE;
}

STATIC
BOOLEAN
GuidPayloadContains (
  IN CONST UINT8     *Payload,
  IN UINTN           PayloadSize,
  IN CONST EFI_GUID  *Guid
  )
{
  UINTN  Offset;

  if ((PayloadSize == 0) || ((PayloadSize % sizeof (EFI_GUID)) != 0)) {
    return FALSE;
  }

  for (Offset = 0; Offset < PayloadSize; Offset += sizeof (EFI_GUID)) {
    if (CompareGuid ((CONST EFI_GUID *)&Payload[Offset], Guid)) {
      return TRUE;
    }
  }

  return FALSE;
}

STATIC
CONST ECIT_CLAIM_DESCRIPTOR *
GetClaimDescriptor (
  IN ECIT_CLAIM_ID  ClaimId
  )
{
  if (ClaimId >= EcitClaimCount) {
    return NULL;
  }

  ASSERT (mClaimDescriptors[ClaimId].ClaimId == ClaimId);
  return &mClaimDescriptors[ClaimId];
}

VOID
InitializeEcitClaimResults (
  OUT ECIT_CLAIM_RESULT  *ClaimResults,
  IN UINTN               ClaimResultCount
  )
{
  CONST ECIT_CLAIM_DESCRIPTOR  *Descriptor;
  UINTN                        Index;

  ASSERT (ClaimResults != NULL);
  ASSERT (ClaimResultCount == EcitClaimCount);

  for (Index = 0; Index < ClaimResultCount; Index++) {
    Descriptor = GetClaimDescriptor ((ECIT_CLAIM_ID)Index);
    ASSERT (Descriptor != NULL);
    ClaimResults[Index].Name              = Descriptor->Name;
    ClaimResults[Index].Claim             = EcitClaimUnknown;
    ClaimResults[Index].Observation       = EcitObservationNotTested;
    ClaimResults[Index].ObservationStatus = EFI_NOT_READY;
    ClaimResults[Index].CandidateProfile  = Descriptor->CandidateProfile;
  }
}

EFI_STATUS
PopulateEcitClaimStates (
  IN CONST EFI_CRYPTO_INDICATOR_TABLE  *Table,
  IN OUT ECIT_CLAIM_RESULT             *ClaimResults,
  IN UINTN                             ClaimResultCount
  )
{
  CONST ECIT_CLAIM_DESCRIPTOR       *Descriptor;
  CONST EFI_CRYPTO_INDICATOR_ENTRY  *Entry;
  UINTN                             EntryIndex;
  BOOLEAN                           Found;
  UINTN                             Index;
  UINTN                             Offset;
  CONST UINT8                       *Payload;
  UINTN                             PayloadSize;

  if ((Table == NULL) || (ClaimResults == NULL) || (ClaimResultCount != EcitClaimCount)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < ClaimResultCount; Index++) {
    Descriptor = GetClaimDescriptor ((ECIT_CLAIM_ID)Index);
    if (Descriptor->CandidateProfile) {
      ClaimResults[Index].Claim = EcitClaimUnknown;
      continue;
    }

    Found  = FALSE;
    Offset = sizeof (*Table);
    for (EntryIndex = 0; EntryIndex < Table->NumberOfEntries; EntryIndex++) {
      Entry = (CONST EFI_CRYPTO_INDICATOR_ENTRY *)((CONST UINT8 *)Table + Offset);
      if (CompareGuid (&Entry->FeatureIdentifier, Descriptor->Feature)) {
        Payload     = (CONST UINT8 *)(Entry + 1);
        PayloadSize = Entry->EntryLength - sizeof (*Entry);
        if (Descriptor->IdentifierKind == EcitClaimIdentifierOid) {
          Found = CsvContainsToken (Payload, PayloadSize, Descriptor->Oid);
        } else {
          Found = GuidPayloadContains (
                    Payload,
                    PayloadSize,
                    Descriptor->SignatureListType
                    );
        }

        if (Found) {
          break;
        }
      }

      Offset += Entry->EntryLength;
    }

    ClaimResults[Index].Claim = Found ? EcitClaimAdvertised : EcitClaimAbsent;
  }

  return EFI_SUCCESS;
}

ECIT_CLAIM_ID
FindEcitOidClaim (
  IN CONST EFI_GUID  *Feature,
  IN CONST CHAR8     *Oid
  )
{
  CONST ECIT_CLAIM_DESCRIPTOR  *Descriptor;
  UINTN                        Index;

  if ((Feature == NULL) || (Oid == NULL)) {
    return EcitClaimCount;
  }

  for (Index = 0; Index < ARRAY_SIZE (mClaimDescriptors); Index++) {
    Descriptor = &mClaimDescriptors[Index];
    if ((Descriptor->IdentifierKind == EcitClaimIdentifierOid) &&
        CompareGuid (Feature, Descriptor->Feature) &&
        (AsciiStrCmp (Oid, Descriptor->Oid) == 0))
    {
      return Descriptor->ClaimId;
    }
  }

  return EcitClaimCount;
}

ECIT_CLAIM_ID
FindEcitSignatureListClaim (
  IN CONST EFI_GUID  *Feature,
  IN CONST EFI_GUID  *SignatureListType
  )
{
  CONST ECIT_CLAIM_DESCRIPTOR  *Descriptor;
  UINTN                        Index;

  if ((Feature == NULL) || (SignatureListType == NULL)) {
    return EcitClaimCount;
  }

  for (Index = 0; Index < ARRAY_SIZE (mClaimDescriptors); Index++) {
    Descriptor = &mClaimDescriptors[Index];
    if ((Descriptor->IdentifierKind == EcitClaimIdentifierGuid) &&
        CompareGuid (Feature, Descriptor->Feature) &&
        CompareGuid (SignatureListType, Descriptor->SignatureListType))
    {
      return Descriptor->ClaimId;
    }
  }

  return EcitClaimCount;
}

VOID
RecordEcitClaimObservation (
  IN OUT ECIT_CLAIM_RESULT   *ClaimResults,
  IN UINTN                   ClaimResultCount,
  IN ECIT_CLAIM_ID           ClaimId,
  IN ECIT_OBSERVATION_STATE  Observation,
  IN EFI_STATUS              Status
  )
{
  if ((ClaimResults == NULL) ||
      (ClaimId >= ClaimResultCount) ||
      (ClaimResults[ClaimId].Observation == EcitObservationSuccess))
  {
    return;
  }

  ClaimResults[ClaimId].Observation       = Observation;
  ClaimResults[ClaimId].ObservationStatus = Status;
}

VOID
RecordEcitClaimObservations (
  IN OUT ECIT_CLAIM_RESULT   *ClaimResults,
  IN UINTN                   ClaimResultCount,
  IN CONST ECIT_CLAIM_ID     *ClaimIds,
  IN UINTN                   ClaimCount,
  IN ECIT_OBSERVATION_STATE  Observation,
  IN EFI_STATUS              Status
  )
{
  UINTN  Index;

  if ((ClaimIds == NULL) && (ClaimCount != 0)) {
    return;
  }

  for (Index = 0; Index < ClaimCount; Index++) {
    RecordEcitClaimObservation (
      ClaimResults,
      ClaimResultCount,
      ClaimIds[Index],
      Observation,
      Status
      );
  }
}
