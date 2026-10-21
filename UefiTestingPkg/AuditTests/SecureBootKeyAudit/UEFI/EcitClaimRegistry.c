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
  EcitClaimIdentifierOidPair,
  EcitClaimIdentifierGuid
} ECIT_CLAIM_IDENTIFIER_KIND;

typedef struct {
  ECIT_CLAIM_ID                 ClaimId;
  CONST CHAR8                   *Name;
  CONST EFI_GUID                *Feature;
  ECIT_CLAIM_IDENTIFIER_KIND    IdentifierKind;
  CONST CHAR8                   *Oid;
  CONST EFI_GUID                *SignatureListType;
  CONST CHAR8                   *PairedOid;
} ECIT_CLAIM_DESCRIPTOR;

#define ECIT_GUID_CLAIM(ClaimId, ClaimName, FeatureGuid, SignatureGuid) \
  { ClaimId, ClaimName, FeatureGuid, EcitClaimIdentifierGuid, NULL, SignatureGuid, NULL }

STATIC CONST ECIT_CLAIM_DESCRIPTOR  mClaimDescriptors[] = {
  {
    EcitDbUpdateAuthorizationX509,
    "db-update-authorization:x509",
    &gEfiEcitFeatureSbDatabaseUpdateAuthorizationGuid,
    EcitClaimIdentifierGuid,
    NULL,
    &gEfiCertX509Guid,
    NULL
  },
  {
    EcitDbUpdateVerificationRsaSha256,
    "db-update-verification:rsa-sha256",
    &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid,
    EcitClaimIdentifierOid,
    "1.2.840.113549.1.1.11",
    NULL,
    NULL
  },
  {
    EcitDbUpdateVerificationEcdsaSha256,
    "db-update-verification:ecdsa-sha256",
    &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid,
    EcitClaimIdentifierOid,
    "1.2.840.10045.4.3.2",
    NULL,
    NULL
  },
  {
    EcitDbUpdateVerificationSha256,
    "db-update-verification:sha256",
    &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid,
    EcitClaimIdentifierOid,
    "2.16.840.1.101.3.4.2.1",
    NULL,
    NULL
  },
  {
    EcitDbUpdateVerificationMldsa87,
    "db-update-verification:mldsa87",
    &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid,
    EcitClaimIdentifierOid,
    "2.16.840.1.101.3.4.3.19",
    NULL,
    NULL
  },
  {
    EcitDbUpdateVerificationSha512,
    "db-update-verification:sha512",
    &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid,
    EcitClaimIdentifierOid,
    "2.16.840.1.101.3.4.2.3",
    NULL,
    NULL
  },
  {
    EcitDbUpdateVerificationRsaSha256Profile,
    "db-update-verification-profile:sha256-with-rsa",
    &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid,
    EcitClaimIdentifierOidPair,
    "1.2.840.113549.1.1.11",
    NULL,
    "2.16.840.1.101.3.4.2.1"
  },
  {
    EcitDbUpdateVerificationEcdsaSha256Profile,
    "db-update-verification-profile:sha256-with-ecdsa",
    &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid,
    EcitClaimIdentifierOidPair,
    "1.2.840.10045.4.3.2",
    NULL,
    "2.16.840.1.101.3.4.2.1"
  },
  {
    EcitDbUpdateVerificationMldsa87Sha512Profile,
    "db-update-verification-profile:sha512-with-mldsa87",
    &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid,
    EcitClaimIdentifierOidPair,
    "2.16.840.1.101.3.4.3.19",
    NULL,
    "2.16.840.1.101.3.4.2.3"
  },
  ECIT_GUID_CLAIM (EcitImageAuthorizationX509V1, "image-authorization:x509-v1", &gEfiEcitFeatureSbImageAuthorizationGuid, &gEfiCertX509Guid),
  ECIT_GUID_CLAIM (EcitImageAuthorizationX509V2, "image-authorization:x509-v2", &gEfiEcitFeatureSbImageAuthorizationGuid, &gEfiCertV2X509Guid),
  ECIT_GUID_CLAIM (EcitImageAuthorizationHashV1Sha1, "image-authorization:hash-v1-sha1", &gEfiEcitFeatureSbImageAuthorizationGuid, &gEfiCertSha1Guid),
  ECIT_GUID_CLAIM (EcitImageAuthorizationHashV1Sha256, "image-authorization:hash-v1-sha256", &gEfiEcitFeatureSbImageAuthorizationGuid, &gEfiCertSha256Guid),
  ECIT_GUID_CLAIM (EcitImageAuthorizationHashV2Sha256, "image-authorization:hash-v2-sha256", &gEfiEcitFeatureSbImageAuthorizationGuid, &gEfiCertV2Sha256Guid),
  ECIT_GUID_CLAIM (EcitImageAuthorizationHashV1Sha384, "image-authorization:hash-v1-sha384", &gEfiEcitFeatureSbImageAuthorizationGuid, &gEfiCertSha384Guid),
  ECIT_GUID_CLAIM (EcitImageAuthorizationHashV2Sha384, "image-authorization:hash-v2-sha384", &gEfiEcitFeatureSbImageAuthorizationGuid, &gEfiCertV2Sha384Guid),
  ECIT_GUID_CLAIM (EcitImageAuthorizationHashV1Sha512, "image-authorization:hash-v1-sha512", &gEfiEcitFeatureSbImageAuthorizationGuid, &gEfiCertSha512Guid),
  ECIT_GUID_CLAIM (EcitImageAuthorizationHashV2Sha512, "image-authorization:hash-v2-sha512", &gEfiEcitFeatureSbImageAuthorizationGuid, &gEfiCertV2Sha512Guid),
  ECIT_GUID_CLAIM (EcitImageAuthorizationTbsV1Sha256, "image-authorization:tbs-v1-sha256", &gEfiEcitFeatureSbImageAuthorizationGuid, &gEfiCertX509Sha256Guid),
  ECIT_GUID_CLAIM (EcitImageAuthorizationTbsV2Sha256, "image-authorization:tbs-v2-sha256", &gEfiEcitFeatureSbImageAuthorizationGuid, &gEfiCertV2X509Sha256Guid),
  ECIT_GUID_CLAIM (EcitImageAuthorizationTbsV1Sha384, "image-authorization:tbs-v1-sha384", &gEfiEcitFeatureSbImageAuthorizationGuid, &gEfiCertX509Sha384Guid),
  ECIT_GUID_CLAIM (EcitImageAuthorizationTbsV2Sha384, "image-authorization:tbs-v2-sha384", &gEfiEcitFeatureSbImageAuthorizationGuid, &gEfiCertV2X509Sha384Guid),
  ECIT_GUID_CLAIM (EcitImageAuthorizationTbsV1Sha512, "image-authorization:tbs-v1-sha512", &gEfiEcitFeatureSbImageAuthorizationGuid, &gEfiCertX509Sha512Guid),
  ECIT_GUID_CLAIM (EcitImageAuthorizationTbsV2Sha512, "image-authorization:tbs-v2-sha512", &gEfiEcitFeatureSbImageAuthorizationGuid, &gEfiCertV2X509Sha512Guid),
  ECIT_GUID_CLAIM (EcitImageRevocationX509V1, "image-revocation:x509-v1", &gEfiEcitFeatureSbImageRevocationGuid, &gEfiCertX509Guid),
  ECIT_GUID_CLAIM (EcitImageRevocationX509V2, "image-revocation:x509-v2", &gEfiEcitFeatureSbImageRevocationGuid, &gEfiCertV2X509Guid),
  ECIT_GUID_CLAIM (EcitImageRevocationHashV1Sha1, "image-revocation:hash-v1-sha1", &gEfiEcitFeatureSbImageRevocationGuid, &gEfiCertSha1Guid),
  ECIT_GUID_CLAIM (EcitImageRevocationHashV1Sha256, "image-revocation:hash-v1-sha256", &gEfiEcitFeatureSbImageRevocationGuid, &gEfiCertSha256Guid),
  ECIT_GUID_CLAIM (EcitImageRevocationHashV2Sha256, "image-revocation:hash-v2-sha256", &gEfiEcitFeatureSbImageRevocationGuid, &gEfiCertV2Sha256Guid),
  ECIT_GUID_CLAIM (EcitImageRevocationHashV1Sha384, "image-revocation:hash-v1-sha384", &gEfiEcitFeatureSbImageRevocationGuid, &gEfiCertSha384Guid),
  ECIT_GUID_CLAIM (EcitImageRevocationHashV2Sha384, "image-revocation:hash-v2-sha384", &gEfiEcitFeatureSbImageRevocationGuid, &gEfiCertV2Sha384Guid),
  ECIT_GUID_CLAIM (EcitImageRevocationHashV1Sha512, "image-revocation:hash-v1-sha512", &gEfiEcitFeatureSbImageRevocationGuid, &gEfiCertSha512Guid),
  ECIT_GUID_CLAIM (EcitImageRevocationHashV2Sha512, "image-revocation:hash-v2-sha512", &gEfiEcitFeatureSbImageRevocationGuid, &gEfiCertV2Sha512Guid),
  ECIT_GUID_CLAIM (EcitImageRevocationTbsV1Sha256, "image-revocation:tbs-v1-sha256", &gEfiEcitFeatureSbImageRevocationGuid, &gEfiCertX509Sha256Guid),
  ECIT_GUID_CLAIM (EcitImageRevocationTbsV2Sha256, "image-revocation:tbs-v2-sha256", &gEfiEcitFeatureSbImageRevocationGuid, &gEfiCertV2X509Sha256Guid),
  ECIT_GUID_CLAIM (EcitImageRevocationTbsV1Sha384, "image-revocation:tbs-v1-sha384", &gEfiEcitFeatureSbImageRevocationGuid, &gEfiCertX509Sha384Guid),
  ECIT_GUID_CLAIM (EcitImageRevocationTbsV2Sha384, "image-revocation:tbs-v2-sha384", &gEfiEcitFeatureSbImageRevocationGuid, &gEfiCertV2X509Sha384Guid),
  ECIT_GUID_CLAIM (EcitImageRevocationTbsV1Sha512, "image-revocation:tbs-v1-sha512", &gEfiEcitFeatureSbImageRevocationGuid, &gEfiCertX509Sha512Guid),
  ECIT_GUID_CLAIM (EcitImageRevocationTbsV2Sha512, "image-revocation:tbs-v2-sha512", &gEfiEcitFeatureSbImageRevocationGuid, &gEfiCertV2X509Sha512Guid),
  {
    EcitImageVerificationRsaSha256,
    "image-verification:rsa-sha256",
    &gEfiEcitFeatureSbImageVerificationGuid,
    EcitClaimIdentifierOid,
    "1.2.840.113549.1.1.11",
    NULL,
    NULL
  },
  {
    EcitImageVerificationEcdsaSha256,
    "image-verification:ecdsa-sha256",
    &gEfiEcitFeatureSbImageVerificationGuid,
    EcitClaimIdentifierOid,
    "1.2.840.10045.4.3.2",
    NULL,
    NULL
  },
  {
    EcitImageVerificationSha256,
    "image-verification:sha256",
    &gEfiEcitFeatureSbImageVerificationGuid,
    EcitClaimIdentifierOid,
    "2.16.840.1.101.3.4.2.1",
    NULL,
    NULL
  },
  {
    EcitImageVerificationMldsa87,
    "image-verification:mldsa87",
    &gEfiEcitFeatureSbImageVerificationGuid,
    EcitClaimIdentifierOid,
    "2.16.840.1.101.3.4.3.19",
    NULL,
    NULL
  },
  {
    EcitImageVerificationSha512,
    "image-verification:sha512",
    &gEfiEcitFeatureSbImageVerificationGuid,
    EcitClaimIdentifierOid,
    "2.16.840.1.101.3.4.2.3",
    NULL,
    NULL
  },
  {
    EcitImageVerificationRsaSha256Profile,
    "image-verification-profile:sha256-with-rsa",
    &gEfiEcitFeatureSbImageVerificationGuid,
    EcitClaimIdentifierOidPair,
    "1.2.840.113549.1.1.11",
    NULL,
    "2.16.840.1.101.3.4.2.1"
  },
  {
    EcitImageVerificationEcdsaSha256Profile,
    "image-verification-profile:sha256-with-ecdsa",
    &gEfiEcitFeatureSbImageVerificationGuid,
    EcitClaimIdentifierOidPair,
    "1.2.840.10045.4.3.2",
    NULL,
    "2.16.840.1.101.3.4.2.1"
  },
  {
    EcitImageVerificationMldsa87Sha512Profile,
    "image-verification-profile:sha512-with-mldsa87",
    &gEfiEcitFeatureSbImageVerificationGuid,
    EcitClaimIdentifierOidPair,
    "2.16.840.1.101.3.4.3.19",
    NULL,
    "2.16.840.1.101.3.4.2.3"
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
    Found      = FALSE;
    Offset     = sizeof (*Table);
    for (EntryIndex = 0; EntryIndex < Table->NumberOfEntries; EntryIndex++) {
      Entry = (CONST EFI_CRYPTO_INDICATOR_ENTRY *)((CONST UINT8 *)Table + Offset);
      if (CompareGuid (&Entry->FeatureIdentifier, Descriptor->Feature)) {
        Payload     = (CONST UINT8 *)(Entry + 1);
        PayloadSize = Entry->EntryLength - sizeof (*Entry);
        if (Descriptor->IdentifierKind == EcitClaimIdentifierOid) {
          Found = CsvContainsToken (Payload, PayloadSize, Descriptor->Oid);
        } else if (Descriptor->IdentifierKind == EcitClaimIdentifierOidPair) {
          Found = CsvContainsToken (Payload, PayloadSize, Descriptor->Oid) &&
                  CsvContainsToken (Payload, PayloadSize, Descriptor->PairedOid);
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
FindEcitOidPairClaim (
  IN CONST EFI_GUID  *Feature,
  IN CONST CHAR8     *SignatureOid,
  IN CONST CHAR8     *DigestOid
  )
{
  CONST ECIT_CLAIM_DESCRIPTOR  *Descriptor;
  UINTN                        Index;

  if ((Feature == NULL) || (SignatureOid == NULL) || (DigestOid == NULL)) {
    return EcitClaimCount;
  }

  for (Index = 0; Index < ARRAY_SIZE (mClaimDescriptors); Index++) {
    Descriptor = &mClaimDescriptors[Index];
    if ((Descriptor->IdentifierKind == EcitClaimIdentifierOidPair) &&
        CompareGuid (Feature, Descriptor->Feature) &&
        (AsciiStrCmp (SignatureOid, Descriptor->Oid) == 0) &&
        (AsciiStrCmp (DigestOid, Descriptor->PairedOid) == 0))
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

CONST CHAR8 *
GetEcitClaimName (
  IN ECIT_CLAIM_ID  ClaimId
  )
{
  CONST ECIT_CLAIM_DESCRIPTOR  *Descriptor;

  Descriptor = GetClaimDescriptor (ClaimId);
  return (Descriptor == NULL) ? NULL : Descriptor->Name;
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
