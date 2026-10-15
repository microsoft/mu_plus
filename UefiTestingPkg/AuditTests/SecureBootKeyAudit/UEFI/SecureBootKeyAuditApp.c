/** @file
  Exercise ECIT-advertised Secure Boot PK rotation using real SetVariable calls.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Guid/CryptoIndicatorTable.h>
#include <Guid/GlobalVariable.h>
#include <Guid/ImageAuthentication.h>

#include <Protocol/DevicePath.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeCoffLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UnitTestLib.h>

#include "TestData.h"

#define UNIT_TEST_APP_NAME     "SecureBootKeyAuditApp"
#define UNIT_TEST_APP_VERSION  "0.4"

#define SHA256_WITH_RSA_OID  "1.2.840.113549.1.1.11"
#define SHA256_OID           "2.16.840.1.101.3.4.2.1"
#define SHA512_OID           "2.16.840.1.101.3.4.2.3"
#define ML_DSA_87_OID        "2.16.840.1.101.3.4.3.19"

#define SECURE_BOOT_KEY_ATTRIBUTES  \
  (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | \
   EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)

#define MAX_ECIT_LENGTH  SIZE_1MB

typedef struct {
  CONST UINT8    *Data;
  UINTN          Size;
} ECIT_PAYLOAD_VIEW;

typedef enum {
  EcitClaimUnknown,
  EcitClaimAbsent,
  EcitClaimAdvertised
} ECIT_CLAIM_STATE;

typedef enum {
  EcitObservationNotTested,
  EcitObservationSuccess,
  EcitObservationFailure,
  EcitObservationInconclusive
} ECIT_OBSERVATION_STATE;

typedef enum {
  EcitCoverageVerified,
  EcitCoverageOverReported,
  EcitCoverageUnderReported,
  EcitCoverageVerifiedAbsent,
  EcitCoverageNotIndependentlyVerifiable,
  EcitCoverageNotTested,
  EcitCoverageInconclusive
} ECIT_COVERAGE_STATE;

typedef enum {
  EcitDbUpdateAuthorizationX509,
  EcitDbUpdateVerificationRsaSha256,
  EcitDbUpdateVerificationSha256,
  EcitDbUpdateVerificationMldsa87,
  EcitDbUpdateVerificationSha512,
  EcitImageAuthorizationTbsV1Sha512,
  EcitImageAuthorizationTbsV2Sha512,
  EcitImageVerificationMldsa87,
  EcitImageVerificationSha512,
  EcitImageVerificationMldsa87Sha512Profile,
  EcitClaimCount
} ECIT_CLAIM_ID;

typedef struct {
  CONST CHAR8               *Name;
  ECIT_CLAIM_STATE          Claim;
  ECIT_OBSERVATION_STATE    Observation;
  EFI_STATUS                ObservationStatus;
  BOOLEAN                   CandidateProfile;
} ECIT_CLAIM_RESULT;

typedef enum {
  AuditVariableStateUnread,
  AuditVariableStateAbsent,
  AuditVariableStatePkA,
  AuditVariableStatePkB,
  AuditVariableStatePkMldsa,
  AuditVariableStateDbV1,
  AuditVariableStateDbV2,
  AuditVariableStateUnknown
} AUDIT_VARIABLE_STATE;

typedef struct {
  UINT8                   SetupMode;
  UINT8                   SecureBoot;
  AUDIT_VARIABLE_STATE    PkState;
  UINT32                  PkAttributes;
  UINTN                   PkSize;
  AUDIT_VARIABLE_STATE    DbState;
  UINT32                  DbAttributes;
  UINTN                   DbSize;
} AUDIT_BASELINE_SNAPSHOT;

#pragma pack (1)
typedef struct {
  MEMMAP_DEVICE_PATH          MemMap;
  EFI_DEVICE_PATH_PROTOCOL    End;
} TEST_IMAGE_DEVICE_PATH;
#pragma pack ()

STATIC TEST_IMAGE_DEVICE_PATH  mTestImageDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_MEMMAP_DP,
      { (UINT8)sizeof (MEMMAP_DEVICE_PATH),       (UINT8)(sizeof (MEMMAP_DEVICE_PATH) >> 8) }
    },
    EfiBootServicesData,
    0,
    0
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { (UINT8)sizeof (EFI_DEVICE_PATH_PROTOCOL), 0                                         }
  }
};

STATIC EFI_STATUS  mEcitParseStatus = EFI_NOT_READY;

STATIC ECIT_CLAIM_RESULT  mEcitClaimResults[EcitClaimCount] = {
  { "db-update-authorization:x509",                   EcitClaimUnknown, EcitObservationNotTested, EFI_NOT_READY, FALSE },
  { "db-update-verification:rsa-sha256",              EcitClaimUnknown, EcitObservationNotTested, EFI_NOT_READY, FALSE },
  { "db-update-verification:sha256",                  EcitClaimUnknown, EcitObservationNotTested, EFI_NOT_READY, FALSE },
  { "db-update-verification:mldsa87",                 EcitClaimUnknown, EcitObservationNotTested, EFI_NOT_READY, FALSE },
  { "db-update-verification:sha512",                  EcitClaimUnknown, EcitObservationNotTested, EFI_NOT_READY, FALSE },
  { "image-authorization:tbs-v1-sha512",              EcitClaimUnknown, EcitObservationNotTested, EFI_NOT_READY, FALSE },
  { "image-authorization:tbs-v2-sha512",              EcitClaimUnknown, EcitObservationNotTested, EFI_NOT_READY, FALSE },
  { "image-verification:mldsa87",                     EcitClaimUnknown, EcitObservationNotTested, EFI_NOT_READY, FALSE },
  { "image-verification:sha512",                      EcitClaimUnknown, EcitObservationNotTested, EFI_NOT_READY, FALSE },
  { "image-verification-profile:sha512-with-mldsa87", EcitClaimUnknown, EcitObservationNotTested, EFI_NOT_READY, TRUE  }
};

STATIC CONST ECIT_CLAIM_ID  mRsaUpdateClaims[] = {
  EcitDbUpdateAuthorizationX509,
  EcitDbUpdateVerificationRsaSha256,
  EcitDbUpdateVerificationSha256
};

STATIC CONST ECIT_CLAIM_ID  mMldsaUpdateClaims[] = {
  EcitDbUpdateAuthorizationX509,
  EcitDbUpdateVerificationMldsa87,
  EcitDbUpdateVerificationSha512
};

STATIC CONST ECIT_CLAIM_ID  mMldsaImageVerificationClaims[] = {
  EcitImageVerificationMldsa87,
  EcitImageVerificationSha512,
  EcitImageVerificationMldsa87Sha512Profile
};

STATIC
BOOLEAN
BufferIsZero (
  IN CONST UINT8  *Buffer,
  IN UINTN        BufferSize
  )
{
  UINTN  Index;

  for (Index = 0; Index < BufferSize; Index++) {
    if (Buffer[Index] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

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
  IN CONST ECIT_PAYLOAD_VIEW  *Payload,
  IN CONST EFI_GUID           *Guid
  )
{
  UINTN  Offset;

  if ((Payload->Size == 0) || ((Payload->Size % sizeof (EFI_GUID)) != 0)) {
    return FALSE;
  }

  for (Offset = 0; Offset < Payload->Size; Offset += sizeof (EFI_GUID)) {
    if (CompareGuid ((CONST EFI_GUID *)&Payload->Data[Offset], Guid)) {
      return TRUE;
    }
  }

  return FALSE;
}

STATIC
EFI_STATUS
ParseEcitClaims (
  VOID
  )
{
  UINT8                       Checksum;
  UINTN                       Count;
  ECIT_PAYLOAD_VIEW           Authorization;
  ECIT_PAYLOAD_VIEW           ImageAuthorization;
  ECIT_PAYLOAD_VIEW           ImageVerification;
  ECIT_PAYLOAD_VIEW           Verification;
  EFI_CRYPTO_INDICATOR_ENTRY  *Entry;
  EFI_CRYPTO_INDICATOR_TABLE  *Table;
  UINTN                       Index;
  UINTN                       Offset;
  UINTN                       Remaining;
  EFI_STATUS                  Status;

  ZeroMem (&Authorization, sizeof (Authorization));
  ZeroMem (&ImageAuthorization, sizeof (ImageAuthorization));
  ZeroMem (&ImageVerification, sizeof (ImageVerification));
  ZeroMem (&Verification, sizeof (Verification));

  Status = EfiGetSystemConfigurationTable (
             &gEfiCryptoIndicatorTableGuid,
             (VOID **)&Table
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("ECIT was not published: %r\n", Status);
    return Status;
  }

  if ((Table->Length < sizeof (*Table)) || (Table->Length > MAX_ECIT_LENGTH)) {
    UT_LOG_ERROR ("ECIT length is invalid: %u\n", Table->Length);
    return EFI_COMPROMISED_DATA;
  }

  if ((CompareMem (Table->Signature, "ECIT", sizeof (Table->Signature)) != 0) ||
      (Table->Version != EFI_CRYPTO_INDICATOR_TABLE_VERSION) ||
      !BufferIsZero (Table->Reserved, sizeof (Table->Reserved)))
  {
    UT_LOG_ERROR ("ECIT header is invalid\n");
    return EFI_COMPROMISED_DATA;
  }

  Checksum = 0;
  for (Index = 0; Index < Table->Length; Index++) {
    Checksum = (UINT8)(Checksum + ((UINT8 *)Table)[Index]);
  }

  if (Checksum != 0) {
    UT_LOG_ERROR ("ECIT checksum is invalid\n");
    return EFI_CRC_ERROR;
  }

  Count  = 0;
  Offset = sizeof (*Table);
  while (Count < Table->NumberOfEntries) {
    Remaining = Table->Length - Offset;
    if (Remaining < sizeof (*Entry)) {
      return EFI_COMPROMISED_DATA;
    }

    Entry = (EFI_CRYPTO_INDICATOR_ENTRY *)((UINT8 *)Table + Offset);
    if ((Entry->EntryLength < sizeof (*Entry)) ||
        ((Entry->EntryLength & 7) != 0) ||
        (Entry->EntryLength > Remaining) ||
        !BufferIsZero (Entry->Reserved, sizeof (Entry->Reserved)))
    {
      return EFI_COMPROMISED_DATA;
    }

    if (CompareGuid (
          &Entry->FeatureIdentifier,
          &gEfiEcitFeatureSbDatabaseUpdateAuthorizationGuid
          ))
    {
      if (Authorization.Data != NULL) {
        UT_LOG_ERROR ("ECIT contains duplicate database-update authorization entries\n");
        return EFI_COMPROMISED_DATA;
      }

      Authorization.Data = (CONST UINT8 *)(Entry + 1);
      Authorization.Size = Entry->EntryLength - sizeof (*Entry);
    } else if (CompareGuid (
                 &Entry->FeatureIdentifier,
                 &gEfiEcitFeatureSbImageAuthorizationGuid
                 ))
    {
      if (ImageAuthorization.Data != NULL) {
        UT_LOG_ERROR ("ECIT contains duplicate image-authorization entries\n");
        return EFI_COMPROMISED_DATA;
      }

      ImageAuthorization.Data = (CONST UINT8 *)(Entry + 1);
      ImageAuthorization.Size = Entry->EntryLength - sizeof (*Entry);
    } else if (CompareGuid (
                 &Entry->FeatureIdentifier,
                 &gEfiEcitFeatureSbImageVerificationGuid
                 ))
    {
      if (ImageVerification.Data != NULL) {
        UT_LOG_ERROR ("ECIT contains duplicate image-verification entries\n");
        return EFI_COMPROMISED_DATA;
      }

      ImageVerification.Data = (CONST UINT8 *)(Entry + 1);
      ImageVerification.Size = Entry->EntryLength - sizeof (*Entry);
    } else if (CompareGuid (
                 &Entry->FeatureIdentifier,
                 &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid
                 ))
    {
      if (Verification.Data != NULL) {
        UT_LOG_ERROR ("ECIT contains duplicate database-update verification entries\n");
        return EFI_COMPROMISED_DATA;
      }

      Verification.Data = (CONST UINT8 *)(Entry + 1);
      Verification.Size = Entry->EntryLength - sizeof (*Entry);
    }

    Offset += Entry->EntryLength;
    Count++;
  }

  if (Offset != Table->Length) {
    UT_LOG_ERROR ("ECIT entry count does not consume the table\n");
    return EFI_COMPROMISED_DATA;
  }

  mEcitClaimResults[EcitDbUpdateAuthorizationX509].Claim =
    GuidPayloadContains (&Authorization, &gEfiCertX509Guid) ? EcitClaimAdvertised : EcitClaimAbsent;
  mEcitClaimResults[EcitDbUpdateVerificationRsaSha256].Claim =
    CsvContainsToken (Verification.Data, Verification.Size, SHA256_WITH_RSA_OID) ? EcitClaimAdvertised : EcitClaimAbsent;
  mEcitClaimResults[EcitDbUpdateVerificationSha256].Claim =
    CsvContainsToken (Verification.Data, Verification.Size, SHA256_OID) ? EcitClaimAdvertised : EcitClaimAbsent;
  mEcitClaimResults[EcitDbUpdateVerificationMldsa87].Claim =
    CsvContainsToken (Verification.Data, Verification.Size, ML_DSA_87_OID) ? EcitClaimAdvertised : EcitClaimAbsent;
  mEcitClaimResults[EcitDbUpdateVerificationSha512].Claim =
    CsvContainsToken (Verification.Data, Verification.Size, SHA512_OID) ? EcitClaimAdvertised : EcitClaimAbsent;
  mEcitClaimResults[EcitImageAuthorizationTbsV1Sha512].Claim =
    GuidPayloadContains (&ImageAuthorization, &gEfiCertX509Sha512Guid) ? EcitClaimAdvertised : EcitClaimAbsent;
  mEcitClaimResults[EcitImageAuthorizationTbsV2Sha512].Claim =
    GuidPayloadContains (&ImageAuthorization, &gEfiCertV2X509Sha512Guid) ? EcitClaimAdvertised : EcitClaimAbsent;
  mEcitClaimResults[EcitImageVerificationMldsa87].Claim =
    CsvContainsToken (ImageVerification.Data, ImageVerification.Size, ML_DSA_87_OID) ? EcitClaimAdvertised : EcitClaimAbsent;
  mEcitClaimResults[EcitImageVerificationSha512].Claim =
    CsvContainsToken (ImageVerification.Data, ImageVerification.Size, SHA512_OID) ? EcitClaimAdvertised : EcitClaimAbsent;
  mEcitClaimResults[EcitImageVerificationMldsa87Sha512Profile].Claim = EcitClaimUnknown;

  UT_LOG_INFO ("ECIT claims parsed independently from operational tests\n");
  return EFI_SUCCESS;
}

STATIC
VOID
RecordEcitObservation (
  IN ECIT_CLAIM_ID           ClaimId,
  IN ECIT_OBSERVATION_STATE  Observation,
  IN EFI_STATUS              Status
  )
{
  if ((ClaimId >= EcitClaimCount) ||
      (mEcitClaimResults[ClaimId].Observation == EcitObservationSuccess))
  {
    return;
  }

  mEcitClaimResults[ClaimId].Observation       = Observation;
  mEcitClaimResults[ClaimId].ObservationStatus = Status;
}

STATIC
VOID
RecordEcitObservations (
  IN CONST ECIT_CLAIM_ID     *ClaimIds,
  IN UINTN                   ClaimCount,
  IN ECIT_OBSERVATION_STATE  Observation,
  IN EFI_STATUS              Status
  )
{
  UINTN  Index;

  for (Index = 0; Index < ClaimCount; Index++) {
    RecordEcitObservation (ClaimIds[Index], Observation, Status);
  }
}

STATIC
ECIT_COVERAGE_STATE
CorrelateEcitResult (
  IN ECIT_CLAIM_STATE        Claim,
  IN ECIT_OBSERVATION_STATE  Observation,
  IN BOOLEAN                 CandidateProfile,
  IN EFI_STATUS              ParseStatus
  )
{
  if (EFI_ERROR (ParseStatus) || (Observation == EcitObservationInconclusive)) {
    return EcitCoverageInconclusive;
  }

  if (CandidateProfile || (Claim == EcitClaimUnknown)) {
    return EcitCoverageNotIndependentlyVerifiable;
  }

  if (Observation == EcitObservationNotTested) {
    return EcitCoverageNotTested;
  }

  if (Claim == EcitClaimAdvertised) {
    return (Observation == EcitObservationSuccess) ?
           EcitCoverageVerified :
           EcitCoverageOverReported;
  }

  return (Observation == EcitObservationSuccess) ?
         EcitCoverageUnderReported :
         EcitCoverageVerifiedAbsent;
}

STATIC
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

STATIC
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

STATIC
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
CONST CHAR8 *
EcitCoverageReason (
  IN CONST ECIT_CLAIM_RESULT  *Result,
  IN ECIT_COVERAGE_STATE      Coverage
  )
{
  if (EFI_ERROR (mEcitParseStatus)) {
    return "ECIT_PARSE_FAILED";
  }

  if (Result->CandidateProfile) {
    return "FLAT_PROFILE_AMBIGUITY";
  }

  switch (Coverage) {
    case EcitCoverageNotTested:
      return "NO_OPERATIONAL_OBSERVATION";
    case EcitCoverageOverReported:
    case EcitCoverageVerifiedAbsent:
      return "OPERATION_FAILED";
    default:
      return "NONE";
  }
}

STATIC
VOID
EFIAPI
EcitSuiteSetup (
  VOID
  )
{
  UINTN  Index;

  for (Index = 0; Index < EcitClaimCount; Index++) {
    mEcitClaimResults[Index].Claim             = EcitClaimUnknown;
    mEcitClaimResults[Index].Observation       = EcitObservationNotTested;
    mEcitClaimResults[Index].ObservationStatus = EFI_NOT_READY;
  }

  mEcitParseStatus = ParseEcitClaims ();
}

STATIC
UNIT_TEST_STATUS
EFIAPI
EcitCorrelationReportTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  ECIT_COVERAGE_STATE  Coverage;
  UINTN                Counts[EcitCoverageInconclusive + 1];
  UINTN                Index;
  UINTN                AdvertisedClaims;

  ZeroMem (Counts, sizeof (Counts));
  AdvertisedClaims = 0;

  for (Index = 0; Index < EcitClaimCount; Index++) {
    Coverage = CorrelateEcitResult (
                 mEcitClaimResults[Index].Claim,
                 mEcitClaimResults[Index].Observation,
                 mEcitClaimResults[Index].CandidateProfile,
                 mEcitParseStatus
                 );
    Counts[Coverage]++;
    if (!mEcitClaimResults[Index].CandidateProfile &&
        (mEcitClaimResults[Index].Claim == EcitClaimAdvertised))
    {
      AdvertisedClaims++;
    }

    UT_LOG_INFO (
      "[ECIT_RESULT] name=%a claim=%a observation=%a coverage=%a reason=%a status=%r\n",
      mEcitClaimResults[Index].Name,
      EcitClaimStateName (mEcitClaimResults[Index].Claim),
      EcitObservationStateName (mEcitClaimResults[Index].Observation),
      EcitCoverageStateName (Coverage),
      EcitCoverageReason (&mEcitClaimResults[Index], Coverage),
      mEcitClaimResults[Index].ObservationStatus
      );
  }

  UT_LOG_INFO (
    "[ECIT_SUMMARY] rows=%u advertised_claims=%u verified=%u over_reported=%u under_reported=%u "
    "verified_absent=%u not_independently_verifiable=%u not_tested=%u inconclusive=%u\n",
    EcitClaimCount,
    AdvertisedClaims,
    Counts[EcitCoverageVerified],
    Counts[EcitCoverageOverReported],
    Counts[EcitCoverageUnderReported],
    Counts[EcitCoverageVerifiedAbsent],
    Counts[EcitCoverageNotIndependentlyVerifiable],
    Counts[EcitCoverageNotTested],
    Counts[EcitCoverageInconclusive]
    );

  if (EFI_ERROR (mEcitParseStatus) ||
      (Counts[EcitCoverageOverReported] != 0) ||
      (Counts[EcitCoverageUnderReported] != 0) ||
      (Counts[EcitCoverageNotTested] != 0) ||
      (Counts[EcitCoverageInconclusive] != 0))
  {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
EcitClaimsParseTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  if (EFI_ERROR (mEcitParseStatus)) {
    UT_LOG_ERROR ("ECIT parsing failed: %r\n", mEcitParseStatus);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
EcitCorrelationClassifierTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_EQUAL (
    CorrelateEcitResult (EcitClaimAdvertised, EcitObservationSuccess, FALSE, EFI_SUCCESS),
    EcitCoverageVerified
    );
  UT_ASSERT_EQUAL (
    CorrelateEcitResult (EcitClaimAdvertised, EcitObservationFailure, FALSE, EFI_SUCCESS),
    EcitCoverageOverReported
    );
  UT_ASSERT_EQUAL (
    CorrelateEcitResult (EcitClaimAbsent, EcitObservationSuccess, FALSE, EFI_SUCCESS),
    EcitCoverageUnderReported
    );
  UT_ASSERT_EQUAL (
    CorrelateEcitResult (EcitClaimAbsent, EcitObservationFailure, FALSE, EFI_SUCCESS),
    EcitCoverageVerifiedAbsent
    );
  UT_ASSERT_EQUAL (
    CorrelateEcitResult (EcitClaimUnknown, EcitObservationSuccess, TRUE, EFI_SUCCESS),
    EcitCoverageNotIndependentlyVerifiable
    );
  UT_ASSERT_EQUAL (
    CorrelateEcitResult (EcitClaimAdvertised, EcitObservationNotTested, FALSE, EFI_SUCCESS),
    EcitCoverageNotTested
    );
  UT_ASSERT_EQUAL (
    CorrelateEcitResult (EcitClaimAdvertised, EcitObservationInconclusive, FALSE, EFI_SUCCESS),
    EcitCoverageInconclusive
    );

  return UNIT_TEST_PASSED;
}

STATIC
EFI_STATUS
ReadVariable (
  IN  CONST CHAR16    *Name,
  IN  CONST EFI_GUID  *Guid,
  OUT UINT32          *Attributes,
  OUT UINT8           **Data,
  OUT UINTN           *DataSize
  )
{
  EFI_STATUS  Status;

  *Data       = NULL;
  *DataSize   = 0;
  *Attributes = 0;

  Status = gRT->GetVariable (
                  (CHAR16 *)Name,
                  (EFI_GUID *)Guid,
                  Attributes,
                  DataSize,
                  NULL
                  );
  if (Status == EFI_NOT_FOUND) {
    return Status;
  }

  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  *Data = AllocatePool (*DataSize);
  if (*Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gRT->GetVariable (
                  (CHAR16 *)Name,
                  (EFI_GUID *)Guid,
                  Attributes,
                  DataSize,
                  *Data
                  );
  if (EFI_ERROR (Status)) {
    FreePool (*Data);
    *Data = NULL;
  }

  return Status;
}

STATIC
EFI_STATUS
ReadModeVariable (
  IN  CONST CHAR16  *Name,
  OUT UINT8         *Value
  )
{
  UINT32  Attributes;
  UINTN   DataSize;

  DataSize = sizeof (*Value);
  return gRT->GetVariable (
                (CHAR16 *)Name,
                &gEfiGlobalVariableGuid,
                &Attributes,
                &DataSize,
                Value
                );
}

STATIC
BOOLEAN
PkMatches (
  IN CONST UINT8  *Pk,
  IN UINTN        PkSize,
  IN CONST UINT8  *Expected,
  IN UINTN        ExpectedSize
  )
{
  return (BOOLEAN)(
                   (PkSize == ExpectedSize) &&
                   (CompareMem (Pk, Expected, PkSize) == 0)
                   );
}

STATIC
EFI_STATUS
VerifyPk (
  IN CONST UINT8  *Expected,
  IN UINTN        ExpectedSize
  )
{
  UINT32      Attributes;
  UINT8       *Pk;
  UINTN       PkSize;
  EFI_STATUS  Status;

  Status = ReadVariable (
             EFI_PLATFORM_KEY_NAME,
             &gEfiGlobalVariableGuid,
             &Attributes,
             &Pk,
             &PkSize
             );
  if (ExpectedSize == 0) {
    if (Status == EFI_NOT_FOUND) {
      return EFI_SUCCESS;
    }

    if (!EFI_ERROR (Status)) {
      FreePool (Pk);
    }

    return EFI_COMPROMISED_DATA;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Attributes != SECURE_BOOT_KEY_ATTRIBUTES) ||
      !PkMatches (Pk, PkSize, Expected, ExpectedSize))
  {
    Status = EFI_COMPROMISED_DATA;
  }

  FreePool (Pk);
  return Status;
}

STATIC
EFI_STATUS
VerifyDb (
  IN CONST UINT8  *Expected,
  IN UINTN        ExpectedSize
  )
{
  UINT32      Attributes;
  UINT8       *Db;
  UINTN       DbSize;
  EFI_STATUS  Status;

  Status = ReadVariable (
             EFI_IMAGE_SECURITY_DATABASE,
             &gEfiImageSecurityDatabaseGuid,
             &Attributes,
             &Db,
             &DbSize
             );
  if (ExpectedSize == 0) {
    if (Status == EFI_NOT_FOUND) {
      return EFI_SUCCESS;
    }

    if (!EFI_ERROR (Status)) {
      FreePool (Db);
    }

    return EFI_COMPROMISED_DATA;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Attributes != SECURE_BOOT_KEY_ATTRIBUTES) ||
      !PkMatches (Db, DbSize, Expected, ExpectedSize))
  {
    Status = EFI_COMPROMISED_DATA;
  }

  FreePool (Db);
  return Status;
}

STATIC
EFI_STATUS
ApplyPkUpdate (
  IN CONST UINT8  *AuthenticatedUpdate,
  IN UINTN        AuthenticatedUpdateSize,
  IN CONST UINT8  *ExpectedPk,
  IN UINTN        ExpectedPkSize
  )
{
  EFI_STATUS  Status;

  Status = gRT->SetVariable (
                  EFI_PLATFORM_KEY_NAME,
                  &gEfiGlobalVariableGuid,
                  SECURE_BOOT_KEY_ATTRIBUTES,
                  AuthenticatedUpdateSize,
                  (VOID *)AuthenticatedUpdate
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return VerifyPk (ExpectedPk, ExpectedPkSize);
}

STATIC
EFI_STATUS
ApplyDbUpdate (
  IN CONST UINT8  *AuthenticatedUpdate,
  IN UINTN        AuthenticatedUpdateSize,
  IN CONST UINT8  *ExpectedDb,
  IN UINTN        ExpectedDbSize
  )
{
  EFI_STATUS  Status;

  Status = gRT->SetVariable (
                  EFI_IMAGE_SECURITY_DATABASE,
                  &gEfiImageSecurityDatabaseGuid,
                  SECURE_BOOT_KEY_ATTRIBUTES,
                  AuthenticatedUpdateSize,
                  (VOID *)AuthenticatedUpdate
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return VerifyDb (ExpectedDb, ExpectedDbSize);
}

STATIC
EFI_STATUS
RecoverDb (
  VOID
  )
{
  UINT32      Attributes;
  UINT8       *Db;
  UINTN       DbSize;
  EFI_STATUS  Status;

  Status = ReadVariable (
             EFI_IMAGE_SECURITY_DATABASE,
             &gEfiImageSecurityDatabaseGuid,
             &Attributes,
             &Db,
             &DbSize
             );
  if (Status == EFI_NOT_FOUND) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!PkMatches (
         Db,
         DbSize,
         gSecureBootKeyAuditDbTbsHashV1,
         gSecureBootKeyAuditDbTbsHashV1Size
         ) &&
      !PkMatches (
         Db,
         DbSize,
         gSecureBootKeyAuditDbTbsHashV2,
         gSecureBootKeyAuditDbTbsHashV2Size
         ))
  {
    FreePool (Db);
    UT_LOG_ERROR ("Refusing to modify an unknown existing db\n");
    return EFI_ACCESS_DENIED;
  }

  FreePool (Db);
  return ApplyDbUpdate (
           gSecureBootKeyAuditDeleteDbByMldsa,
           gSecureBootKeyAuditDeleteDbByMldsaSize,
           NULL,
           0
           );
}

STATIC
EFI_STATUS
RecoverSetupModeInternal (
  VOID
  )
{
  UINT32      Attributes;
  UINT8       *Pk;
  UINTN       PkSize;
  EFI_STATUS  Status;

  Status = RecoverDb ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ReadVariable (
             EFI_PLATFORM_KEY_NAME,
             &gEfiGlobalVariableGuid,
             &Attributes,
             &Pk,
             &PkSize
             );
  if (Status == EFI_NOT_FOUND) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (PkMatches (
        Pk,
        PkSize,
        gSecureBootKeyAuditPkA,
        gSecureBootKeyAuditPkASize
        ))
  {
    FreePool (Pk);
    return ApplyPkUpdate (
             gSecureBootKeyAuditRecoveryDeleteByA,
             gSecureBootKeyAuditRecoveryDeleteByASize,
             NULL,
             0
             );
  }

  if (PkMatches (
        Pk,
        PkSize,
        gSecureBootKeyAuditPkB,
        gSecureBootKeyAuditPkBSize
        ))
  {
    FreePool (Pk);
    return ApplyPkUpdate (
             gSecureBootKeyAuditRecoveryDeleteByB,
             gSecureBootKeyAuditRecoveryDeleteByBSize,
             NULL,
             0
             );
  }

  if (PkMatches (
        Pk,
        PkSize,
        gSecureBootKeyAuditPkMldsa,
        gSecureBootKeyAuditPkMldsaSize
        ))
  {
    FreePool (Pk);
    return ApplyPkUpdate (
             gSecureBootKeyAuditRecoveryDeleteByMldsa,
             gSecureBootKeyAuditRecoveryDeleteByMldsaSize,
             NULL,
             0
             );
  }

  FreePool (Pk);
  UT_LOG_ERROR ("Refusing to modify an unknown existing PK\n");
  return EFI_ACCESS_DENIED;
}

STATIC
EFI_STATUS
RecoverSetupMode (
  VOID
  )
{
  EFI_STATUS  Status;

  UT_LOG_INFO ("[AUDIT_RECOVERY] phase=begin\n");
  Status = RecoverSetupModeInternal ();
  UT_LOG_INFO ("[AUDIT_RECOVERY] phase=end status=%r\n", Status);
  return Status;
}

STATIC
CONST CHAR8 *
AuditVariableStateName (
  IN AUDIT_VARIABLE_STATE  State
  )
{
  switch (State) {
    case AuditVariableStateAbsent:
      return "ABSENT";
    case AuditVariableStatePkA:
      return "PK_A";
    case AuditVariableStatePkB:
      return "PK_B";
    case AuditVariableStatePkMldsa:
      return "PK_MLDSA87";
    case AuditVariableStateDbV1:
      return "DB_TBS_V1";
    case AuditVariableStateDbV2:
      return "DB_TBS_V2";
    case AuditVariableStateUnknown:
      return "UNKNOWN";
    default:
      return "UNREAD";
  }
}

STATIC
EFI_STATUS
CaptureBaselineSnapshot (
  OUT AUDIT_BASELINE_SNAPSHOT  *Snapshot
  )
{
  UINT32      Attributes;
  UINT8       *Data;
  UINTN       DataSize;
  EFI_STATUS  Status;

  ZeroMem (Snapshot, sizeof (*Snapshot));
  Snapshot->SetupMode  = 0xFF;
  Snapshot->SecureBoot = 0xFF;
  Snapshot->PkState    = AuditVariableStateUnread;
  Snapshot->DbState    = AuditVariableStateUnread;

  Status = ReadModeVariable (EFI_SETUP_MODE_NAME, &Snapshot->SetupMode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ReadModeVariable (EFI_SECURE_BOOT_MODE_NAME, &Snapshot->SecureBoot);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ReadVariable (
             EFI_PLATFORM_KEY_NAME,
             &gEfiGlobalVariableGuid,
             &Attributes,
             &Data,
             &DataSize
             );
  if (Status == EFI_NOT_FOUND) {
    Snapshot->PkState = AuditVariableStateAbsent;
  } else if (EFI_ERROR (Status)) {
    return Status;
  } else {
    Snapshot->PkAttributes = Attributes;
    Snapshot->PkSize       = DataSize;
    if (PkMatches (Data, DataSize, gSecureBootKeyAuditPkA, gSecureBootKeyAuditPkASize)) {
      Snapshot->PkState = AuditVariableStatePkA;
    } else if (PkMatches (Data, DataSize, gSecureBootKeyAuditPkB, gSecureBootKeyAuditPkBSize)) {
      Snapshot->PkState = AuditVariableStatePkB;
    } else if (PkMatches (
                 Data,
                 DataSize,
                 gSecureBootKeyAuditPkMldsa,
                 gSecureBootKeyAuditPkMldsaSize
                 ))
    {
      Snapshot->PkState = AuditVariableStatePkMldsa;
    } else {
      Snapshot->PkState = AuditVariableStateUnknown;
    }

    FreePool (Data);
  }

  Status = ReadVariable (
             EFI_IMAGE_SECURITY_DATABASE,
             &gEfiImageSecurityDatabaseGuid,
             &Attributes,
             &Data,
             &DataSize
             );
  if (Status == EFI_NOT_FOUND) {
    Snapshot->DbState = AuditVariableStateAbsent;
  } else if (EFI_ERROR (Status)) {
    return Status;
  } else {
    Snapshot->DbAttributes = Attributes;
    Snapshot->DbSize       = DataSize;
    if (PkMatches (
          Data,
          DataSize,
          gSecureBootKeyAuditDbTbsHashV1,
          gSecureBootKeyAuditDbTbsHashV1Size
          ))
    {
      Snapshot->DbState = AuditVariableStateDbV1;
    } else if (PkMatches (
                 Data,
                 DataSize,
                 gSecureBootKeyAuditDbTbsHashV2,
                 gSecureBootKeyAuditDbTbsHashV2Size
                 ))
    {
      Snapshot->DbState = AuditVariableStateDbV2;
    } else {
      Snapshot->DbState = AuditVariableStateUnknown;
    }

    FreePool (Data);
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InspectSetupModeBaseline (
  OUT AUDIT_BASELINE_SNAPSHOT  *Snapshot
  )
{
  EFI_STATUS  Status;

  Status = CaptureBaselineSnapshot (Snapshot);
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("[AUDIT_EVIDENCE] stage=preflight capture_status=%r\n", Status);
    return Status;
  }

  UT_LOG_INFO (
    "[AUDIT_EVIDENCE] stage=preflight setup_mode=%u secure_boot=%u "
    "pk_state=%a pk_attributes=0x%08x pk_size=%u "
    "db_state=%a db_attributes=0x%08x db_size=%u\n",
    Snapshot->SetupMode,
    Snapshot->SecureBoot,
    AuditVariableStateName (Snapshot->PkState),
    Snapshot->PkAttributes,
    Snapshot->PkSize,
    AuditVariableStateName (Snapshot->DbState),
    Snapshot->DbAttributes,
    Snapshot->DbSize
    );

  if ((Snapshot->SetupMode == SETUP_MODE) &&
      (Snapshot->SecureBoot == SECURE_BOOT_MODE_DISABLE) &&
      (Snapshot->PkState == AuditVariableStateAbsent) &&
      (Snapshot->DbState == AuditVariableStateAbsent))
  {
    return EFI_SUCCESS;
  }

  if ((Snapshot->PkState == AuditVariableStateUnknown) ||
      (Snapshot->PkState == AuditVariableStateUnread) ||
      (Snapshot->DbState == AuditVariableStateUnknown) ||
      (Snapshot->DbState == AuditVariableStateUnread))
  {
    UT_LOG_ERROR ("Refusing to recover an unknown Secure Boot state\n");
    return EFI_ACCESS_DENIED;
  }

  UT_LOG_WARNING ("Recognized interrupted test state requires explicit recovery\n");
  return EFI_NOT_READY;
}

STATIC
EFI_STATUS
RequireSetupModeBaseline (
  VOID
  )
{
  AUDIT_BASELINE_SNAPSHOT  Snapshot;

  return InspectSetupModeBaseline (&Snapshot);
}

STATIC
UNIT_TEST_STATUS
EFIAPI
InterruptedStateRecoveryTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  AUDIT_BASELINE_SNAPSHOT  Snapshot;
  EFI_STATUS               RecoveryStatus;
  UNIT_TEST_STATUS         Result;
  EFI_STATUS               Status;

  Result = UNIT_TEST_ERROR_TEST_FAILED;

  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The recovery test baseline is unsafe: %r\n", Status);
    goto Exit;
  }

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditSetupPkA,
             gSecureBootKeyAuditSetupPkASize,
             gSecureBootKeyAuditPkA,
             gSecureBootKeyAuditPkASize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Unable to create the simulated interrupted state: %r\n", Status);
    goto Exit;
  }

  Status = InspectSetupModeBaseline (&Snapshot);
  if (Status != EFI_NOT_READY) {
    UT_LOG_ERROR ("Read-only preflight did not classify the interrupted state: %r\n", Status);
    goto Exit;
  }

  Status = VerifyPk (gSecureBootKeyAuditPkA, gSecureBootKeyAuditPkASize);
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Read-only preflight modified the recognized PK state: %r\n", Status);
    goto Exit;
  }

  UT_LOG_INFO ("[AUDIT_EVIDENCE] interrupted_state_preserved_before_recovery=TRUE\n");
  Status = RecoverSetupMode ();
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Explicit recovery of the interrupted state failed: %r\n", Status);
    goto Exit;
  }

  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Explicit recovery did not restore the baseline: %r\n", Status);
    goto Exit;
  }

  Result = UNIT_TEST_PASSED;

Exit:
  if (Result != UNIT_TEST_PASSED) {
    RecoveryStatus = RecoverSetupMode ();
    if (EFI_ERROR (RecoveryStatus)) {
      UT_LOG_ERROR ("Unable to recover the simulated interrupted state: %r\n", RecoveryStatus);
    }
  }

  return Result;
}

STATIC
VOID
LogSecureBootMode (
  IN CONST CHAR8  *Phase
  )
{
  UINT8  SecureBoot;
  UINT8  SetupMode;

  if (EFI_ERROR (ReadModeVariable (EFI_SETUP_MODE_NAME, &SetupMode))) {
    SetupMode = 0xFF;
  }

  if (EFI_ERROR (ReadModeVariable (EFI_SECURE_BOOT_MODE_NAME, &SecureBoot))) {
    SecureBoot = 0xFF;
  }

  UT_LOG_INFO (
    "%a: SetupMode=%u SecureBoot=%u\n",
    Phase,
    SetupMode,
    SecureBoot
    );
}

STATIC
EFI_STATUS
RequireSecureBootEnforcement (
  VOID
  )
{
  UINT8       SecureBoot;
  UINT8       SetupMode;
  EFI_STATUS  Status;

  Status = ReadModeVariable (EFI_SETUP_MODE_NAME, &SetupMode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ReadModeVariable (EFI_SECURE_BOOT_MODE_NAME, &SecureBoot);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((SetupMode != USER_MODE) || (SecureBoot != SECURE_BOOT_MODE_ENABLE)) {
    UT_LOG_ERROR (
      "Secure Boot is not enforcing: SetupMode=%u SecureBoot=%u\n",
      SetupMode,
      SecureBoot
      );
    return EFI_SECURITY_VIOLATION;
  }

  UT_LOG_INFO ("Secure Boot enforcement confirmed: SetupMode=0 SecureBoot=1\n");
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
LoadImageExpectDenied (
  IN CONST CHAR8  *Phase,
  IN CONST VOID   *Image,
  IN UINTN        ImageSize
  )
{
  EFI_HANDLE  LoadedImage;
  EFI_STATUS  Status;
  EFI_STATUS  UnloadStatus;

  LoadedImage = NULL;
  Status      = gBS->LoadImage (
                       FALSE,
                       gImageHandle,
                       (EFI_DEVICE_PATH_PROTOCOL *)&mTestImageDevicePath,
                       (VOID *)Image,
                       ImageSize,
                       &LoadedImage
                       );

  UnloadStatus = EFI_SUCCESS;
  if (LoadedImage != NULL) {
    UnloadStatus = gBS->UnloadImage (LoadedImage);
  }

  if ((Status != EFI_ACCESS_DENIED) && (Status != EFI_SECURITY_VIOLATION)) {
    UT_LOG_ERROR ("%a: expected image denial, received %r\n", Phase, Status);
    return EFI_SECURITY_VIOLATION;
  }

  if (EFI_ERROR (UnloadStatus)) {
    UT_LOG_ERROR ("%a: failed to unload the denied image: %r\n", Phase, UnloadStatus);
    return UnloadStatus;
  }

  UT_LOG_INFO ("%a: image denied as expected: %r\n", Phase, Status);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
BuildTamperedImage (
  OUT UINT8  **TamperedImage
  )
{
  PE_COFF_LOADER_IMAGE_CONTEXT  Context;
  RETURN_STATUS                 ReturnStatus;

  *TamperedImage = NULL;
  ZeroMem (&Context, sizeof (Context));
  Context.Handle    = (VOID *)gSecureBootKeyAuditHelloMldsa;
  Context.ImageRead = PeCoffLoaderImageReadFromMemory;

  ReturnStatus = PeCoffLoaderGetImageInfo (&Context);
  if (RETURN_ERROR (ReturnStatus)) {
    return EFI_LOAD_ERROR;
  }

  if ((Context.SizeOfHeaders == 0) ||
      (Context.SizeOfHeaders >= gSecureBootKeyAuditHelloMldsaSize))
  {
    return EFI_COMPROMISED_DATA;
  }

  *TamperedImage = AllocateCopyPool (
                     gSecureBootKeyAuditHelloMldsaSize,
                     gSecureBootKeyAuditHelloMldsa
                     );
  if (*TamperedImage == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  (*TamperedImage)[Context.SizeOfHeaders] ^= 0xFF;
  return EFI_SUCCESS;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
RsaPkRotationTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS        RecoveryStatus;
  UNIT_TEST_STATUS  Result;
  EFI_STATUS        Status;

  Result = UNIT_TEST_ERROR_TEST_FAILED;

  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The PK baseline is unsafe: %r\n", Status);
    goto Exit;
  }

  LogSecureBootMode ("baseline");

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditSetupPkA,
             gSecureBootKeyAuditSetupPkASize,
             gSecureBootKeyAuditPkA,
             gSecureBootKeyAuditPkASize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Setup Mode enrollment of PK A failed: %r\n", Status);
    goto Exit;
  }

  LogSecureBootMode ("PK A enrolled");

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditPkAToB,
             gSecureBootKeyAuditPkAToBSize,
             gSecureBootKeyAuditPkB,
             gSecureBootKeyAuditPkBSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("PK A could not authorize rotation to PK B: %r\n", Status);
    RecordEcitObservations (
      mRsaUpdateClaims,
      ARRAY_SIZE (mRsaUpdateClaims),
      EcitObservationFailure,
      Status
      );
    goto Exit;
  }

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditPkBToA,
             gSecureBootKeyAuditPkBToASize,
             gSecureBootKeyAuditPkA,
             gSecureBootKeyAuditPkASize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("PK B could not authorize rotation back to PK A: %r\n", Status);
    RecordEcitObservations (
      mRsaUpdateClaims,
      ARRAY_SIZE (mRsaUpdateClaims),
      EcitObservationFailure,
      Status
      );
    goto Exit;
  }

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditDeleteByA,
             gSecureBootKeyAuditDeleteByASize,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("PK A could not authenticate final cleanup: %r\n", Status);
    RecordEcitObservations (
      mRsaUpdateClaims,
      ARRAY_SIZE (mRsaUpdateClaims),
      EcitObservationFailure,
      Status
      );
    goto Exit;
  }

  RecordEcitObservations (
    mRsaUpdateClaims,
    ARRAY_SIZE (mRsaUpdateClaims),
    EcitObservationSuccess,
    EFI_SUCCESS
    );

  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Final Setup Mode baseline was not restored: %r\n", Status);
    goto Exit;
  }

  LogSecureBootMode ("cleanup complete");
  Result = UNIT_TEST_PASSED;

Exit:
  RecoveryStatus = RecoverSetupMode ();
  if (EFI_ERROR (RecoveryStatus)) {
    UT_LOG_ERROR ("Unable to recover the test PK state: %r\n", RecoveryStatus);
    Result = UNIT_TEST_ERROR_TEST_FAILED;
  }

  return Result;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
MldsaSetupEnrollmentTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS        RecoveryStatus;
  UNIT_TEST_STATUS  Result;
  EFI_STATUS        Status;

  Result = UNIT_TEST_ERROR_TEST_FAILED;

  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The PK baseline is unsafe: %r\n", Status);
    goto Exit;
  }

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditSetupPkMldsa,
             gSecureBootKeyAuditSetupPkMldsaSize,
             gSecureBootKeyAuditPkMldsa,
             gSecureBootKeyAuditPkMldsaSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Setup Mode enrollment of the ML-DSA-87 PK failed: %r\n", Status);
    goto Exit;
  }

  LogSecureBootMode ("ML-DSA-87 PK enrolled");

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditDeleteByMldsa,
             gSecureBootKeyAuditDeleteByMldsaSize,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The ML-DSA-87 PK could not authenticate cleanup: %r\n", Status);
    RecordEcitObservations (
      mMldsaUpdateClaims,
      ARRAY_SIZE (mMldsaUpdateClaims),
      EcitObservationFailure,
      Status
      );
    goto Exit;
  }

  RecordEcitObservations (
    mMldsaUpdateClaims,
    ARRAY_SIZE (mMldsaUpdateClaims),
    EcitObservationSuccess,
    EFI_SUCCESS
    );

  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Final Setup Mode baseline was not restored: %r\n", Status);
    goto Exit;
  }

  Result = UNIT_TEST_PASSED;

Exit:
  RecoveryStatus = RecoverSetupMode ();
  if (EFI_ERROR (RecoveryStatus)) {
    UT_LOG_ERROR ("Unable to recover the test PK state: %r\n", RecoveryStatus);
    Result = UNIT_TEST_ERROR_TEST_FAILED;
  }

  return Result;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
RsaToMldsaRotationTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS        RecoveryStatus;
  UNIT_TEST_STATUS  Result;
  EFI_STATUS        Status;

  Result = UNIT_TEST_ERROR_TEST_FAILED;

  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The PK baseline is unsafe: %r\n", Status);
    goto Exit;
  }

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditSetupPkAForMldsa,
             gSecureBootKeyAuditSetupPkAForMldsaSize,
             gSecureBootKeyAuditPkA,
             gSecureBootKeyAuditPkASize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Setup Mode enrollment of RSA PK A failed: %r\n", Status);
    goto Exit;
  }

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditPkAToMldsa,
             gSecureBootKeyAuditPkAToMldsaSize,
             gSecureBootKeyAuditPkMldsa,
             gSecureBootKeyAuditPkMldsaSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("RSA PK A could not authorize the ML-DSA-87 PK: %r\n", Status);
    RecordEcitObservations (
      mRsaUpdateClaims,
      ARRAY_SIZE (mRsaUpdateClaims),
      EcitObservationFailure,
      Status
      );
    goto Exit;
  }

  RecordEcitObservations (
    mRsaUpdateClaims,
    ARRAY_SIZE (mRsaUpdateClaims),
    EcitObservationSuccess,
    EFI_SUCCESS
    );

  LogSecureBootMode ("RSA PK rolled to ML-DSA-87");

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditPkMldsaToA,
             gSecureBootKeyAuditPkMldsaToASize,
             gSecureBootKeyAuditPkA,
             gSecureBootKeyAuditPkASize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The ML-DSA-87 PK could not authorize RSA PK A: %r\n", Status);
    RecordEcitObservations (
      mMldsaUpdateClaims,
      ARRAY_SIZE (mMldsaUpdateClaims),
      EcitObservationFailure,
      Status
      );
    goto Exit;
  }

  RecordEcitObservations (
    mMldsaUpdateClaims,
    ARRAY_SIZE (mMldsaUpdateClaims),
    EcitObservationSuccess,
    EFI_SUCCESS
    );

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditDeleteByAAfterMldsa,
             gSecureBootKeyAuditDeleteByAAfterMldsaSize,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("RSA PK A could not authenticate final cleanup: %r\n", Status);
    RecordEcitObservations (
      mRsaUpdateClaims,
      ARRAY_SIZE (mRsaUpdateClaims),
      EcitObservationFailure,
      Status
      );
    goto Exit;
  }

  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Final Setup Mode baseline was not restored: %r\n", Status);
    goto Exit;
  }

  Result = UNIT_TEST_PASSED;

Exit:
  RecoveryStatus = RecoverSetupMode ();
  if (EFI_ERROR (RecoveryStatus)) {
    UT_LOG_ERROR ("Unable to recover the test PK state: %r\n", RecoveryStatus);
    Result = UNIT_TEST_ERROR_TEST_FAILED;
  }

  return Result;
}

STATIC
UNIT_TEST_STATUS
RunMldsaImageTbsHashTest (
  IN CONST CHAR8    *Version,
  IN ECIT_CLAIM_ID  ImageAuthorizationClaim,
  IN CONST UINT8    *EnrollDb,
  IN UINTN          EnrollDbSize,
  IN CONST UINT8    *ExpectedDb,
  IN UINTN          ExpectedDbSize
  )
{
  EFI_HANDLE        LoadedImage;
  EFI_STATUS        RecoveryStatus;
  UNIT_TEST_STATUS  Result;
  EFI_STATUS        Status;
  UINT8             *TamperedImage;

  LoadedImage   = NULL;
  Result        = UNIT_TEST_ERROR_TEST_FAILED;
  TamperedImage = NULL;

  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The Secure Boot baseline is unsafe: %r\n", Status);
    goto Exit;
  }

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditSetupPkMldsa,
             gSecureBootKeyAuditSetupPkMldsaSize,
             gSecureBootKeyAuditPkMldsa,
             gSecureBootKeyAuditPkMldsaSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Setup Mode enrollment of the ML-DSA-87 PK failed: %r\n", Status);
    goto Exit;
  }

  Status = RequireSecureBootEnforcement ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LoadImageExpectDenied (
             "Before db enrollment",
             gSecureBootKeyAuditHelloMldsa,
             gSecureBootKeyAuditHelloMldsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = ApplyDbUpdate (
             EnrollDb,
             EnrollDbSize,
             ExpectedDb,
             ExpectedDbSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("ML-DSA-87 PK could not enroll the %a TBS hash in db: %r\n", Version, Status);
    RecordEcitObservations (
      mMldsaUpdateClaims,
      ARRAY_SIZE (mMldsaUpdateClaims),
      EcitObservationFailure,
      Status
      );
    goto Exit;
  }

  RecordEcitObservations (
    mMldsaUpdateClaims,
    ARRAY_SIZE (mMldsaUpdateClaims),
    EcitObservationSuccess,
    EFI_SUCCESS
    );

  Status = BuildTamperedImage (&TamperedImage);
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Unable to construct the tampered image control: %r\n", Status);
    goto Exit;
  }

  Status = LoadImageExpectDenied (
             "After tampering",
             TamperedImage,
             gSecureBootKeyAuditHelloMldsaSize
             );
  if (EFI_ERROR (Status)) {
    RecordEcitObservations (
      mMldsaImageVerificationClaims,
      ARRAY_SIZE (mMldsaImageVerificationClaims),
      EcitObservationFailure,
      Status
      );
    goto Exit;
  }

  FreePool (TamperedImage);
  TamperedImage = NULL;

  Status = gBS->LoadImage (
                  FALSE,
                  gImageHandle,
                  (EFI_DEVICE_PATH_PROTOCOL *)&mTestImageDevicePath,
                  (VOID *)gSecureBootKeyAuditHelloMldsa,
                  gSecureBootKeyAuditHelloMldsaSize,
                  &LoadedImage
                  );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("db TBS hash did not authorize the ML-DSA image: %r\n", Status);
    RecordEcitObservation (ImageAuthorizationClaim, EcitObservationFailure, Status);
    RecordEcitObservations (
      mMldsaImageVerificationClaims,
      ARRAY_SIZE (mMldsaImageVerificationClaims),
      EcitObservationFailure,
      Status
      );
    goto Exit;
  }

  RecordEcitObservation (ImageAuthorizationClaim, EcitObservationSuccess, EFI_SUCCESS);
  RecordEcitObservations (
    mMldsaImageVerificationClaims,
    ARRAY_SIZE (mMldsaImageVerificationClaims),
    EcitObservationSuccess,
    EFI_SUCCESS
    );
  UT_LOG_INFO ("%a db TBS hash authorized the ML-DSA-87 image with LoadImage\n", Version);

  Status = gBS->StartImage (LoadedImage, NULL, NULL);
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The authorized ML-DSA image did not start: %r\n", Status);
    goto Exit;
  }

  LoadedImage = NULL;
  UT_LOG_INFO ("The authorized ML-DSA-87 image executed successfully with StartImage\n");

  Status = ApplyDbUpdate (
             gSecureBootKeyAuditDeleteDbByMldsa,
             gSecureBootKeyAuditDeleteDbByMldsaSize,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The ML-DSA-87 PK could not delete the test db: %r\n", Status);
    goto Exit;
  }

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditDeleteByMldsa,
             gSecureBootKeyAuditDeleteByMldsaSize,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The ML-DSA-87 PK could not authenticate cleanup: %r\n", Status);
    goto Exit;
  }

  Result = UNIT_TEST_PASSED;

Exit:
  if (TamperedImage != NULL) {
    FreePool (TamperedImage);
  }

  if (LoadedImage != NULL) {
    gBS->UnloadImage (LoadedImage);
  }

  RecoveryStatus = RecoverSetupMode ();
  if (EFI_ERROR (RecoveryStatus)) {
    UT_LOG_ERROR ("Unable to recover the test PK/db state: %r\n", RecoveryStatus);
    Result = UNIT_TEST_ERROR_TEST_FAILED;
  }

  return Result;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
MldsaImageTbsHashV1Test (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunMldsaImageTbsHashTest (
           "V1",
           EcitImageAuthorizationTbsV1Sha512,
           gSecureBootKeyAuditEnrollDbTbsHashV1,
           gSecureBootKeyAuditEnrollDbTbsHashV1Size,
           gSecureBootKeyAuditDbTbsHashV1,
           gSecureBootKeyAuditDbTbsHashV1Size
           );
}

STATIC
UNIT_TEST_STATUS
EFIAPI
MldsaImageTbsHashV2Test (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunMldsaImageTbsHashTest (
           "V2",
           EcitImageAuthorizationTbsV2Sha512,
           gSecureBootKeyAuditEnrollDbTbsHashV2,
           gSecureBootKeyAuditEnrollDbTbsHashV2Size,
           gSecureBootKeyAuditDbTbsHashV2,
           gSecureBootKeyAuditDbTbsHashV2Size
           );
}

EFI_STATUS
EFIAPI
SecureBootKeyAuditAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      Suite;
  EFI_STATUS                  Status;

  Framework = NULL;
  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  Status = InitUnitTestFramework (
             &Framework,
             UNIT_TEST_APP_NAME,
             gEfiCallerBaseName,
             UNIT_TEST_APP_VERSION
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = CreateUnitTestSuite (
             &Suite,
             Framework,
             "Secure Boot key ECIT audit",
             "SecureBootKeyAudit",
             EcitSuiteSetup,
             NULL
             );
  if (EFI_ERROR (Status)) {
    FreeUnitTestFramework (Framework);
    return Status;
  }

  AddTestCase (
    Suite,
    "Parse ECIT capability claims",
    "EcitClaimsParse",
    EcitClaimsParseTest,
    NULL,
    NULL,
    NULL
    );
  AddTestCase (
    Suite,
    "Classify ECIT claim and observation results",
    "EcitCorrelationClassifier",
    EcitCorrelationClassifierTest,
    NULL,
    NULL,
    NULL
    );
  AddTestCase (
    Suite,
    "Preserve evidence before recovering interrupted test state",
    "InterruptedStateRecovery",
    InterruptedStateRecoveryTest,
    NULL,
    NULL,
    NULL
    );
  AddTestCase (
    Suite,
    "Rotate RSA-2048 PK A to B and back",
    "RsaPkRotation",
    RsaPkRotationTest,
    NULL,
    NULL,
    NULL
    );
  AddTestCase (
    Suite,
    "Enroll ML-DSA-87 PK directly from Setup Mode",
    "MldsaSetupEnrollment",
    MldsaSetupEnrollmentTest,
    NULL,
    NULL,
    NULL
    );
  AddTestCase (
    Suite,
    "Rotate RSA-2048 PK to ML-DSA-87 and back",
    "RsaToMldsaRotation",
    RsaToMldsaRotationTest,
    NULL,
    NULL,
    NULL
    );
  AddTestCase (
    Suite,
    "Authorize an ML-DSA-87 image with a V1 db TBS hash",
    "MldsaImageTbsHashV1",
    MldsaImageTbsHashV1Test,
    NULL,
    NULL,
    NULL
    );
  AddTestCase (
    Suite,
    "Authorize an ML-DSA-87 image with a V2 db TBS hash",
    "MldsaImageTbsHashV2",
    MldsaImageTbsHashV2Test,
    NULL,
    NULL,
    NULL
    );
  AddTestCase (
    Suite,
    "Report ECIT claim and observation correlation",
    "EcitCorrelationReport",
    EcitCorrelationReportTest,
    NULL,
    NULL,
    NULL
    );

  Status = RunAllTestSuites (Framework);
  FreeUnitTestFramework (Framework);
  return Status;
}
