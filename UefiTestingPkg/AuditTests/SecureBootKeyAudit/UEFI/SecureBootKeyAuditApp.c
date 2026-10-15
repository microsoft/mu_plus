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
#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeCoffLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UnitTestLib.h>

#include "AuditRecovery.h"
#include "AuditScenario.h"
#include "AuditTypes.h"
#include "SecureBootState.h"
#include "TestData.h"

#define UNIT_TEST_APP_NAME     "SecureBootKeyAuditApp"
#define UNIT_TEST_APP_VERSION  "0.7"

#define SHA256_WITH_RSA_OID  "1.2.840.113549.1.1.11"
#define SHA256_OID           "2.16.840.1.101.3.4.2.1"
#define SHA512_OID           "2.16.840.1.101.3.4.2.3"
#define ML_DSA_87_OID        "2.16.840.1.101.3.4.3.19"

#define MAX_ECIT_LENGTH  SIZE_1MB

#define AUDIT_JOURNAL_FILE_NAME   L"SecureBootKeyAuditJournal.bin"
#define AUDIT_JOURNAL_SIGNATURE   SIGNATURE_32 ('S', 'B', 'J', 'R')
#define AUDIT_JOURNAL_VERSION     1
#define AUDIT_REBOOT_MARKER_NAME  L"SecureBootKeyAuditRebootPending"
#define AUDIT_REBOOT_MARKER_ATTRIBUTES \
  (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

#pragma pack (1)
typedef struct {
  MEMMAP_DEVICE_PATH          MemMap;
  EFI_DEVICE_PATH_PROTOCOL    End;
} TEST_IMAGE_DEVICE_PATH;
#pragma pack ()

STATIC EFI_GUID  mAuditJournalGuid = {
  0x7ec54fb2, 0xef9f, 0x4e03, { 0x81, 0xf0, 0x56, 0xbb, 0xe1, 0x2d, 0xa6, 0xe9 }
};

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
STATIC UINT8       mEcitSha256[SHA256_DIGEST_SIZE];

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

  if (!Sha256HashAll ((CONST UINT8 *)Table, Table->Length, mEcitSha256)) {
    UT_LOG_ERROR ("Unable to hash the parsed ECIT\n");
    return EFI_DEVICE_ERROR;
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

  ZeroMem (mEcitSha256, sizeof (mEcitSha256));
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
UNIT_TEST_STATUS
EFIAPI
RecoveryStateClassifierTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  STATIC CONST struct {
    AUDIT_BASELINE_SNAPSHOT    Snapshot;
    BOOLEAN                    Expected;
  } Cases[] = {
    {
      { SETUP_MODE, SECURE_BOOT_MODE_DISABLE, AuditVariableStateAbsent,  0,                          0, AuditVariableStateAbsent,  0,                          0 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkA,     SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateAbsent,  0,                          0 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkB,     SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateAbsent,  0,                          0 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateAbsent,  0,                          0 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbV1,    SECURE_BOOT_KEY_ATTRIBUTES, 1 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbV2,    SECURE_BOOT_KEY_ATTRIBUTES, 1 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStateUnknown, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbV1,    SECURE_BOOT_KEY_ATTRIBUTES, 1 }, FALSE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkA,     SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbV1,    SECURE_BOOT_KEY_ATTRIBUTES, 1 }, FALSE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateUnknown, SECURE_BOOT_KEY_ATTRIBUTES, 1 }, FALSE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, 0,                          1, AuditVariableStateAbsent,  0,                          0 }, FALSE
    },
    {
      { SETUP_MODE, SECURE_BOOT_MODE_DISABLE, AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateAbsent,  0,                          0 }, FALSE
    }
  };
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (Cases); Index++) {
    UT_ASSERT_EQUAL (
      RecoveryStateIsRecognized (&Cases[Index].Snapshot),
      Cases[Index].Expected
      );
  }

  return UNIT_TEST_PASSED;
}

STATIC
CONST CHAR8 *
AuditJournalPhaseName (
  IN AUDIT_JOURNAL_PHASE  Phase
  )
{
  switch (Phase) {
    case AuditJournalPrepared:
      return "PREPARED";
    case AuditJournalRebootPending:
      return "REBOOT_PENDING";
    case AuditJournalPersistentlyVerified:
      return "PERSISTENTLY_VERIFIED";
    case AuditJournalFailed:
      return "FAILED";
    default:
      return "UNKNOWN";
  }
}

STATIC
EFI_STATUS
CalculateAuditJournalCrc (
  IN OUT AUDIT_JOURNAL  *Journal
  )
{
  Journal->Crc32 = 0;
  return gBS->CalculateCrc32 (Journal, sizeof (*Journal), &Journal->Crc32);
}

STATIC
EFI_STATUS
ValidateAuditJournal (
  IN CONST AUDIT_JOURNAL  *Journal
  )
{
  AUDIT_JOURNAL  JournalCopy;
  UINT32         SavedCrc32;
  EFI_STATUS     Status;

  if ((Journal->Signature != AUDIT_JOURNAL_SIGNATURE) ||
      (Journal->Version != AUDIT_JOURNAL_VERSION) ||
      (Journal->Size != sizeof (*Journal)) ||
      (Journal->Reserved != 0) ||
      (Journal->Phase < AuditJournalPrepared) ||
      (Journal->Phase > AuditJournalFailed))
  {
    return EFI_COMPROMISED_DATA;
  }

  if ((gSecureBootKeyAuditFixtureIdSize >= sizeof (Journal->FixtureId)) ||
      (gSecureBootKeyAuditManifestSha256Size != sizeof (Journal->ManifestSha256)) ||
      (CompareMem (
         Journal->FixtureId,
         gSecureBootKeyAuditFixtureId,
         gSecureBootKeyAuditFixtureIdSize
         ) != 0) ||
      (Journal->FixtureId[gSecureBootKeyAuditFixtureIdSize] != '\0') ||
      (CompareMem (
         Journal->ManifestSha256,
         gSecureBootKeyAuditManifestSha256,
         sizeof (Journal->ManifestSha256)
         ) != 0))
  {
    return EFI_INCOMPATIBLE_VERSION;
  }

  CopyMem (&JournalCopy, Journal, sizeof (JournalCopy));
  SavedCrc32 = JournalCopy.Crc32;
  Status     = CalculateAuditJournalCrc (&JournalCopy);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return (SavedCrc32 == JournalCopy.Crc32) ? EFI_SUCCESS : EFI_CRC_ERROR;
}

STATIC
EFI_STATUS
ReadAuditJournal (
  OUT AUDIT_JOURNAL  *Journal
  )
{
  SHELL_FILE_HANDLE  FileHandle;
  UINT64             FileSize;
  UINTN              ReadSize;
  EFI_STATUS         Status;

  FileHandle = NULL;
  ZeroMem (Journal, sizeof (*Journal));

  Status = ShellOpenFileByName (
             AUDIT_JOURNAL_FILE_NAME,
             &FileHandle,
             EFI_FILE_MODE_READ,
             0
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ShellGetFileSize (FileHandle, &FileSize);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if (FileSize != sizeof (*Journal)) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Exit;
  }

  ReadSize = sizeof (*Journal);
  Status   = ShellReadFile (FileHandle, &ReadSize, Journal);
  if (!EFI_ERROR (Status) && (ReadSize != sizeof (*Journal))) {
    Status = EFI_DEVICE_ERROR;
  }

  if (!EFI_ERROR (Status)) {
    Status = ValidateAuditJournal (Journal);
  }

Exit:
  ShellCloseFile (&FileHandle);
  return Status;
}

STATIC
EFI_STATUS
WriteAuditJournal (
  IN OUT AUDIT_JOURNAL  *Journal
  )
{
  SHELL_FILE_HANDLE  FileHandle;
  UINTN              WriteSize;
  EFI_STATUS         Status;

  FileHandle    = NULL;
  Journal->Size = (UINT16)sizeof (*Journal);
  Status        = CalculateAuditJournalCrc (Journal);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ShellOpenFileByName (
             AUDIT_JOURNAL_FILE_NAME,
             &FileHandle,
             EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
             0
             );
  if (Status == EFI_NOT_FOUND) {
    Status = ShellOpenFileByName (
               AUDIT_JOURNAL_FILE_NAME,
               &FileHandle,
               EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
               0
               );
  } else if (!EFI_ERROR (Status)) {
    Status = ShellSetFilePosition (FileHandle, 0);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  WriteSize = sizeof (*Journal);
  Status    = ShellWriteFile (FileHandle, &WriteSize, Journal);
  if (!EFI_ERROR (Status) && (WriteSize != sizeof (*Journal))) {
    Status = EFI_DEVICE_ERROR;
  }

  if (!EFI_ERROR (Status)) {
    Status = ShellFlushFile (FileHandle);
  }

  ShellCloseFile (&FileHandle);
  return Status;
}

STATIC
VOID
InitializeAuditJournal (
  OUT AUDIT_JOURNAL                  *Journal,
  IN  CONST AUDIT_BASELINE_SNAPSHOT  *Snapshot
  )
{
  ZeroMem (Journal, sizeof (*Journal));
  Journal->Signature   = AUDIT_JOURNAL_SIGNATURE;
  Journal->Version     = AUDIT_JOURNAL_VERSION;
  Journal->Phase       = AuditJournalPrepared;
  Journal->BootCount   = 1;
  Journal->PriorResult = EFI_NOT_READY;
  CopyMem (
    Journal->FixtureId,
    gSecureBootKeyAuditFixtureId,
    gSecureBootKeyAuditFixtureIdSize
    );
  CopyMem (
    Journal->ManifestSha256,
    gSecureBootKeyAuditManifestSha256,
    sizeof (Journal->ManifestSha256)
    );
  CopyMem (Journal->EcitSha256, mEcitSha256, sizeof (Journal->EcitSha256));
  Journal->SetupMode    = Snapshot->SetupMode;
  Journal->SecureBoot   = Snapshot->SecureBoot;
  Journal->PkState      = Snapshot->PkState;
  Journal->PkAttributes = Snapshot->PkAttributes;
  Journal->PkSize       = Snapshot->PkSize;
  Journal->DbState      = Snapshot->DbState;
  Journal->DbAttributes = Snapshot->DbAttributes;
  Journal->DbSize       = Snapshot->DbSize;
}

STATIC
EFI_STATUS
CompareJournalState (
  IN CONST AUDIT_JOURNAL            *Journal,
  IN CONST AUDIT_BASELINE_SNAPSHOT  *Snapshot
  )
{
  if ((CompareMem (Journal->EcitSha256, mEcitSha256, sizeof (Journal->EcitSha256)) != 0) ||
      (Journal->SetupMode != Snapshot->SetupMode) ||
      (Journal->SecureBoot != Snapshot->SecureBoot) ||
      (Journal->PkState != (UINT32)Snapshot->PkState) ||
      (Journal->PkAttributes != Snapshot->PkAttributes) ||
      (Journal->PkSize != Snapshot->PkSize) ||
      (Journal->DbState != (UINT32)Snapshot->DbState) ||
      (Journal->DbAttributes != Snapshot->DbAttributes) ||
      (Journal->DbSize != Snapshot->DbSize))
  {
    return EFI_COMPROMISED_DATA;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SetAuditRebootMarker (
  VOID
  )
{
  return gRT->SetVariable (
                AUDIT_REBOOT_MARKER_NAME,
                &mAuditJournalGuid,
                AUDIT_REBOOT_MARKER_ATTRIBUTES,
                gSecureBootKeyAuditManifestSha256Size,
                (VOID *)gSecureBootKeyAuditManifestSha256
                );
}

STATIC
EFI_STATUS
RequireClearedRebootMarker (
  VOID
  )
{
  UINT32  Attributes;
  UINT8   Marker[SHA256_DIGEST_SIZE];
  UINTN   MarkerSize;

  MarkerSize = sizeof (Marker);
  return gRT->GetVariable (
                AUDIT_REBOOT_MARKER_NAME,
                &mAuditJournalGuid,
                &Attributes,
                &MarkerSize,
                Marker
                );
}

STATIC
VOID
DeleteAuditRebootMarker (
  VOID
  )
{
  gRT->SetVariable (
         AUDIT_REBOOT_MARKER_NAME,
         &mAuditJournalGuid,
         0,
         0,
         NULL
         );
}

STATIC
UNIT_TEST_STATUS
EFIAPI
PersistentJournalRebootTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  AUDIT_BASELINE_SNAPSHOT  Snapshot;
  AUDIT_JOURNAL            Journal;
  EFI_STATUS               Status;

  if (EFI_ERROR (mEcitParseStatus)) {
    UT_LOG_ERROR ("ECIT must parse before the reboot journal test: %r\n", mEcitParseStatus);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = ReadAuditJournal (&Journal);
  if (Status == EFI_NOT_FOUND) {
    Status = InspectSetupModeBaseline (&Snapshot);
    if (EFI_ERROR (Status)) {
      UT_LOG_ERROR ("The reboot journal baseline is unsafe: %r\n", Status);
      return UNIT_TEST_ERROR_TEST_FAILED;
    }

    InitializeAuditJournal (&Journal, &Snapshot);
    Status = WriteAuditJournal (&Journal);
    if (EFI_ERROR (Status)) {
      UT_LOG_ERROR ("Unable to persist the PREPARED audit journal: %r\n", Status);
      return UNIT_TEST_ERROR_TEST_FAILED;
    }

    UT_LOG_INFO (
      "[AUDIT_JOURNAL] phase=%a boot=%u fixture=%a prior_status=%r\n",
      AuditJournalPhaseName ((AUDIT_JOURNAL_PHASE)Journal.Phase),
      Journal.BootCount,
      Journal.FixtureId,
      (EFI_STATUS)Journal.PriorResult
      );

    Status = SetAuditRebootMarker ();
    if (EFI_ERROR (Status)) {
      Journal.Phase       = AuditJournalFailed;
      Journal.PriorResult = Status;
      WriteAuditJournal (&Journal);
      UT_LOG_ERROR ("Unable to create the volatile reboot marker: %r\n", Status);
      return UNIT_TEST_ERROR_TEST_FAILED;
    }

    Journal.Phase       = AuditJournalRebootPending;
    Journal.PriorResult = EFI_SUCCESS;
    Status              = WriteAuditJournal (&Journal);
    if (EFI_ERROR (Status)) {
      DeleteAuditRebootMarker ();
      UT_LOG_ERROR ("Unable to persist the REBOOT_PENDING audit journal: %r\n", Status);
      return UNIT_TEST_ERROR_TEST_FAILED;
    }

    UT_LOG_INFO (
      "[AUDIT_JOURNAL] phase=%a boot=%u boundary=before_cold_reset\n",
      AuditJournalPhaseName ((AUDIT_JOURNAL_PHASE)Journal.Phase),
      Journal.BootCount
      );

    Status = SaveFrameworkState (&Journal, sizeof (Journal));
    if (EFI_ERROR (Status)) {
      DeleteAuditRebootMarker ();
      Journal.Phase       = AuditJournalFailed;
      Journal.PriorResult = Status;
      WriteAuditJournal (&Journal);
      UT_LOG_ERROR ("Unable to persist the unit-test framework state: %r\n", Status);
      return UNIT_TEST_ERROR_TEST_FAILED;
    }

    gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);

    DeleteAuditRebootMarker ();
    Journal.Phase       = AuditJournalFailed;
    Journal.PriorResult = EFI_ABORTED;
    WriteAuditJournal (&Journal);
    UT_LOG_ERROR ("ResetSystem returned unexpectedly\n");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The persistent audit journal is invalid: %r\n", Status);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  if ((Journal.Phase != AuditJournalRebootPending) ||
      (Journal.BootCount != 1) ||
      (Context == NULL))
  {
    UT_LOG_ERROR (
      "The audit journal cannot resume: phase=%a boot=%u context=%p\n",
      AuditJournalPhaseName ((AUDIT_JOURNAL_PHASE)Journal.Phase),
      Journal.BootCount,
      Context
      );
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = ValidateAuditJournal ((CONST AUDIT_JOURNAL *)Context);
  if (EFI_ERROR (Status) ||
      (((CONST AUDIT_JOURNAL *)Context)->Phase != AuditJournalRebootPending))
  {
    UT_LOG_ERROR ("The resumed unit-test context is invalid: %r\n", Status);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = RequireClearedRebootMarker ();
  if (Status != EFI_NOT_FOUND) {
    UT_LOG_ERROR ("The volatile reboot marker proves no reset occurred: %r\n", Status);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = InspectSetupModeBaseline (&Snapshot);
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The post-reboot baseline is unsafe: %r\n", Status);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = CompareJournalState (&Journal, &Snapshot);
  if (EFI_ERROR (Status)) {
    Journal.Phase       = AuditJournalFailed;
    Journal.PriorResult = Status;
    WriteAuditJournal (&Journal);
    UT_LOG_ERROR ("ECIT or Secure Boot state changed across reboot: %r\n", Status);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Journal.Phase       = AuditJournalPersistentlyVerified;
  Journal.BootCount   = 2;
  Journal.PriorResult = EFI_SUCCESS;
  Status              = WriteAuditJournal (&Journal);
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Unable to persist the verified reboot journal: %r\n", Status);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  UT_LOG_INFO (
    "[AUDIT_JOURNAL] phase=%a boot=%u boundary=after_cold_reset "
    "framework_context=RESTORED ecit_reread=MATCH state_reread=MATCH\n",
    AuditJournalPhaseName ((AUDIT_JOURNAL_PHASE)Journal.Phase),
    Journal.BootCount
    );
  return UNIT_TEST_PASSED;
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
AuthenticatedVariableReplayTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  AUDIT_AUTH_VARIABLE_SCENARIO  EnrollScenario;
  EFI_STATUS                    RecoveryStatus;
  AUDIT_AUTH_VARIABLE_SCENARIO  ReplayScenario;
  UNIT_TEST_STATUS              Result;
  EFI_STATUS                    Status;

  Result = UNIT_TEST_ERROR_TEST_FAILED;

  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The replay test baseline is unsafe: %r\n", Status);
    goto Exit;
  }

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditSetupPkMldsa,
             gSecureBootKeyAuditSetupPkMldsaSize,
             gSecureBootKeyAuditPkMldsa,
             gSecureBootKeyAuditPkMldsaSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Unable to establish the ML-DSA replay-test authority: %r\n", Status);
    goto Exit;
  }

  ZeroMem (&EnrollScenario, sizeof (EnrollScenario));
  EnrollScenario.ScenarioId              = "db-tbs-v1-enrollment";
  EnrollScenario.VariableName            = EFI_IMAGE_SECURITY_DATABASE;
  EnrollScenario.VendorGuid              = &gEfiImageSecurityDatabaseGuid;
  EnrollScenario.Attributes              = SECURE_BOOT_KEY_ATTRIBUTES;
  EnrollScenario.AuthenticatedUpdate     = gSecureBootKeyAuditEnrollDbTbsHashV1;
  EnrollScenario.AuthenticatedUpdateSize = gSecureBootKeyAuditEnrollDbTbsHashV1Size;
  EnrollScenario.ExpectedStatus          = EFI_SUCCESS;
  EnrollScenario.ExpectedAfter           = gSecureBootKeyAuditDbTbsHashV1;
  EnrollScenario.ExpectedAfterSize       = gSecureBootKeyAuditDbTbsHashV1Size;
  EnrollScenario.ExpectedAfterAttributes = SECURE_BOOT_KEY_ATTRIBUTES;
  EnrollScenario.ExpectMutation          = TRUE;

  Status = ExecuteAuthVariableScenario (&EnrollScenario);
  if (EFI_ERROR (Status)) {
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

  ZeroMem (&ReplayScenario, sizeof (ReplayScenario));
  ReplayScenario.ScenarioId               = "db-tbs-v1-replay-rejected";
  ReplayScenario.VariableName             = EFI_IMAGE_SECURITY_DATABASE;
  ReplayScenario.VendorGuid               = &gEfiImageSecurityDatabaseGuid;
  ReplayScenario.Attributes               = SECURE_BOOT_KEY_ATTRIBUTES;
  ReplayScenario.AuthenticatedUpdate      = gSecureBootKeyAuditEnrollDbTbsHashV1;
  ReplayScenario.AuthenticatedUpdateSize  = gSecureBootKeyAuditEnrollDbTbsHashV1Size;
  ReplayScenario.ExpectedStatus           = EFI_SECURITY_VIOLATION;
  ReplayScenario.ExpectedBefore           = gSecureBootKeyAuditDbTbsHashV1;
  ReplayScenario.ExpectedBeforeSize       = gSecureBootKeyAuditDbTbsHashV1Size;
  ReplayScenario.ExpectedBeforeAttributes = SECURE_BOOT_KEY_ATTRIBUTES;
  ReplayScenario.ExpectedAfter            = gSecureBootKeyAuditDbTbsHashV1;
  ReplayScenario.ExpectedAfterSize        = gSecureBootKeyAuditDbTbsHashV1Size;
  ReplayScenario.ExpectedAfterAttributes  = SECURE_BOOT_KEY_ATTRIBUTES;
  ReplayScenario.ExpectMutation           = FALSE;

  Status = ExecuteAuthVariableScenario (&ReplayScenario);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Result = UNIT_TEST_PASSED;

Exit:
  RecoveryStatus = RecoverSetupMode ();
  if (EFI_ERROR (RecoveryStatus)) {
    UT_LOG_ERROR ("Unable to recover the replay-test PK/db state: %r\n", RecoveryStatus);
    Result = UNIT_TEST_ERROR_TEST_FAILED;
  }

  return Result;
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
    "Reject unsafe Secure Boot recovery states before mutation",
    "RecoveryStateClassifier",
    RecoveryStateClassifierTest,
    NULL,
    NULL,
    NULL
    );
  AddTestCase (
    Suite,
    "Persist audit identity and state across a cold reboot",
    "PersistentJournalReboot",
    PersistentJournalRebootTest,
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
    "Reject replayed authenticated db updates atomically",
    "AuthenticatedVariableReplay",
    AuthenticatedVariableReplayTest,
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
