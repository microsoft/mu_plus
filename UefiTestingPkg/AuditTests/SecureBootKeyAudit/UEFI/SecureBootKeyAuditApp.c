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
#include <Library/ShellLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UnitTestLib.h>

#include "AuditRecovery.h"
#include "AuditScenario.h"
#include "AuditTypes.h"
#include "CapabilityPath.h"
#include "EcitClaimRegistry.h"
#include "EcitEvidence.h"
#include "SecureBootState.h"
#include "TestData.h"

#define UNIT_TEST_APP_NAME     "SecureBootKeyAuditApp"
#define UNIT_TEST_APP_VERSION  "0.9"

#define MAX_ECIT_LENGTH  SIZE_1MB

#define AUDIT_JOURNAL_FILE_NAME   L"SecureBootKeyAuditJournal.bin"
#define AUDIT_JOURNAL_SIGNATURE   SIGNATURE_32 ('S', 'B', 'J', 'R')
#define AUDIT_JOURNAL_VERSION     3
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

STATIC EFI_STATUS                        mEcitParseStatus = EFI_NOT_READY;
STATIC CONST EFI_CRYPTO_INDICATOR_TABLE  *mEcitTable;

STATIC ECIT_CLAIM_RESULT  mEcitClaimResults[EcitClaimCount];

STATIC CONST ECIT_CLAIM_ID  mRsaUpdateClaims[] = {
  EcitDbUpdateAuthorizationX509,
  EcitDbUpdateVerificationRsaSha256,
  EcitDbUpdateVerificationSha256,
  EcitDbUpdateVerificationRsaSha256Profile
};

STATIC CONST ECIT_CLAIM_ID  mMldsaUpdateClaims[] = {
  EcitDbUpdateAuthorizationX509,
  EcitDbUpdateVerificationMldsa87,
  EcitDbUpdateVerificationSha512,
  EcitDbUpdateVerificationMldsa87Sha512Profile
};

STATIC CONST ECIT_CLAIM_ID  mEcdsaUpdateClaims[] = {
  EcitDbUpdateAuthorizationX509,
  EcitDbUpdateVerificationEcdsaSha256,
  EcitDbUpdateVerificationSha256,
  EcitDbUpdateVerificationEcdsaSha256Profile
};

STATIC CONST ECIT_CLAIM_ID  mMldsaImageVerificationClaims[] = {
  EcitImageVerificationMldsa87,
  EcitImageVerificationSha512,
  EcitImageVerificationMldsa87Sha512Profile
};

STATIC CONST ECIT_CLAIM_ID  mRsaImageVerificationClaims[] = {
  EcitImageVerificationRsaSha256,
  EcitImageVerificationSha256,
  EcitImageVerificationRsaSha256Profile
};

STATIC CONST ECIT_CLAIM_ID  mEcdsaImageVerificationClaims[] = {
  EcitImageVerificationEcdsaSha256,
  EcitImageVerificationSha256,
  EcitImageVerificationEcdsaSha256Profile
};

STATIC CONST ECIT_CLAIM_ID  mImageAuthorizationFormatClaims[] = {
  EcitImageAuthorizationX509V1,
  EcitImageAuthorizationX509V2,
  EcitImageAuthorizationHashV1Sha1,
  EcitImageAuthorizationHashV1Sha256,
  EcitImageAuthorizationHashV2Sha256,
  EcitImageAuthorizationHashV1Sha384,
  EcitImageAuthorizationHashV2Sha384,
  EcitImageAuthorizationHashV1Sha512,
  EcitImageAuthorizationHashV2Sha512,
  EcitImageAuthorizationTbsV1Sha256,
  EcitImageAuthorizationTbsV2Sha256,
  EcitImageAuthorizationTbsV1Sha384,
  EcitImageAuthorizationTbsV2Sha384,
  EcitImageAuthorizationTbsV1Sha512,
  EcitImageAuthorizationTbsV2Sha512
};

STATIC CONST ECIT_CLAIM_ID  mImageRevocationFormatClaims[] = {
  EcitImageRevocationX509V1,
  EcitImageRevocationX509V2,
  EcitImageRevocationHashV1Sha1,
  EcitImageRevocationHashV1Sha256,
  EcitImageRevocationHashV2Sha256,
  EcitImageRevocationHashV1Sha384,
  EcitImageRevocationHashV2Sha384,
  EcitImageRevocationHashV1Sha512,
  EcitImageRevocationHashV2Sha512,
  EcitImageRevocationTbsV1Sha256,
  EcitImageRevocationTbsV2Sha256,
  EcitImageRevocationTbsV1Sha384,
  EcitImageRevocationTbsV2Sha384,
  EcitImageRevocationTbsV1Sha512,
  EcitImageRevocationTbsV2Sha512
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

  Status = PopulateEcitClaimStates (
             Table,
             mEcitClaimResults,
             ARRAY_SIZE (mEcitClaimResults)
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Unable to populate ECIT claim states: %r\n", Status);
    return Status;
  }

  mEcitTable = Table;

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
  RecordEcitClaimObservation (
    mEcitClaimResults,
    ARRAY_SIZE (mEcitClaimResults),
    ClaimId,
    Observation,
    Status
    );
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
  RecordEcitClaimObservations (
    mEcitClaimResults,
    ARRAY_SIZE (mEcitClaimResults),
    ClaimIds,
    ClaimCount,
    Observation,
    Status
    );
}

STATIC
ECIT_COVERAGE_STATE
CorrelateEcitResult (
  IN ECIT_CLAIM_STATE        Claim,
  IN ECIT_OBSERVATION_STATE  Observation,
  IN EFI_STATUS              ParseStatus
  )
{
  if (EFI_ERROR (ParseStatus) || (Observation == EcitObservationInconclusive)) {
    return EcitCoverageInconclusive;
  }

  if (Claim == EcitClaimUnknown) {
    return EcitCoverageInconclusive;
  }

  if (Claim == EcitClaimAdvertised) {
    if (Observation == EcitObservationNotTested) {
      return EcitCoverageNotTested;
    }

    return (Observation == EcitObservationSuccess) ?
           EcitCoverageVerified :
           EcitCoverageOverReported;
  }

  if (Observation == EcitObservationNotTested) {
    return EcitCoverageNotAdvertised;
  }

  return (Observation == EcitObservationSuccess) ?
         EcitCoverageUnadvertisedSuccess :
         EcitCoverageVerifiedAbsent;
}

STATIC
VOID
EFIAPI
EcitSuiteSetup (
  VOID
  )
{
  mEcitTable = NULL;
  InitializeEcitClaimResults (
    mEcitClaimResults,
    ARRAY_SIZE (mEcitClaimResults)
    );

  mEcitParseStatus = ParseEcitClaims ();
}

STATIC
UNIT_TEST_STATUS
EFIAPI
EcitCorrelationReportTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  ECIT_COVERAGE_STATE  CoverageRows[EcitClaimCount];
  UINTN                Counts[EcitCoverageInconclusive + 1];
  UINTN                Index;
  UINTN                AdvertisedClaims;

  ZeroMem (Counts, sizeof (Counts));
  AdvertisedClaims = 0;

  for (Index = 0; Index < EcitClaimCount; Index++) {
    CoverageRows[Index] = CorrelateEcitResult (
                            mEcitClaimResults[Index].Claim,
                            mEcitClaimResults[Index].Observation,
                            mEcitParseStatus
                            );
    Counts[CoverageRows[Index]]++;
    if (mEcitClaimResults[Index].Claim == EcitClaimAdvertised) {
      AdvertisedClaims++;
    }
  }

  UT_LOG_INFO (
    "[ECIT_SCENARIO_SUMMARY] rows=%u advertised_claims=%u verified=%u over_reported=%u "
    "unadvertised_success=%u verified_absent=%u not_advertised=%u "
    "not_independently_verifiable=%u not_tested=%u inconclusive=%u\n",
    EcitClaimCount,
    AdvertisedClaims,
    Counts[EcitCoverageVerified],
    Counts[EcitCoverageOverReported],
    Counts[EcitCoverageUnadvertisedSuccess],
    Counts[EcitCoverageVerifiedAbsent],
    Counts[EcitCoverageNotAdvertised],
    Counts[EcitCoverageNotIndependentlyVerifiable],
    Counts[EcitCoverageNotTested],
    Counts[EcitCoverageInconclusive]
    );

  LogEcitEvidence (
    mEcitTable,
    mEcitClaimResults,
    CoverageRows,
    ARRAY_SIZE (mEcitClaimResults),
    mEcitParseStatus
    );

  if (EFI_ERROR (mEcitParseStatus) ||
      (Counts[EcitCoverageOverReported] != 0) ||
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
  STATIC CONST struct {
    UINT8    Mask;
    UINT8    Count;
    UINT8    Algorithms[AUDIT_MAX_CAPABILITY_PATH_LENGTH];
  } mPathCases[] = {
    { AUDIT_IMAGE_ALGORITHM_RSA,    1,                            { AUDIT_IMAGE_ALGORITHM_RSA,    0, 0 }
    },
    { AUDIT_IMAGE_ALGORITHM_ECDSA,  1,                            { AUDIT_IMAGE_ALGORITHM_ECDSA,  0, 0 }
    },
    { AUDIT_IMAGE_ALGORITHM_ML_DSA, 1,                            { AUDIT_IMAGE_ALGORITHM_ML_DSA, 0, 0 }
    },
    {
      AUDIT_IMAGE_ALGORITHM_RSA | AUDIT_IMAGE_ALGORITHM_ECDSA,
      2,
      { AUDIT_IMAGE_ALGORITHM_RSA,    AUDIT_IMAGE_ALGORITHM_ECDSA,  0 }
    },
    {
      AUDIT_IMAGE_ALGORITHM_RSA | AUDIT_IMAGE_ALGORITHM_ML_DSA,
      2,
      { AUDIT_IMAGE_ALGORITHM_RSA,    AUDIT_IMAGE_ALGORITHM_ML_DSA, 0 }
    },
    {
      AUDIT_IMAGE_ALGORITHM_ECDSA | AUDIT_IMAGE_ALGORITHM_ML_DSA,
      2,
      { AUDIT_IMAGE_ALGORITHM_ECDSA,  AUDIT_IMAGE_ALGORITHM_ML_DSA, 0 }
    },
    {
      AUDIT_IMAGE_ALGORITHM_ALL,
      3,
      {
        AUDIT_IMAGE_ALGORITHM_RSA,
        AUDIT_IMAGE_ALGORITHM_ECDSA,
        AUDIT_IMAGE_ALGORITHM_ML_DSA
      }
    }
  };
  AUDIT_CAPABILITY_PATH  Path;
  UINTN                  AlgorithmIndex;
  UINTN                  CaseIndex;

  UT_ASSERT_EQUAL (
    CorrelateEcitResult (EcitClaimAdvertised, EcitObservationSuccess, EFI_SUCCESS),
    EcitCoverageVerified
    );
  UT_ASSERT_EQUAL (
    CorrelateEcitResult (EcitClaimAdvertised, EcitObservationFailure, EFI_SUCCESS),
    EcitCoverageOverReported
    );
  UT_ASSERT_EQUAL (
    CorrelateEcitResult (EcitClaimAbsent, EcitObservationSuccess, EFI_SUCCESS),
    EcitCoverageUnadvertisedSuccess
    );
  UT_ASSERT_EQUAL (
    CorrelateEcitResult (EcitClaimAbsent, EcitObservationFailure, EFI_SUCCESS),
    EcitCoverageVerifiedAbsent
    );
  UT_ASSERT_EQUAL (
    CorrelateEcitResult (EcitClaimAbsent, EcitObservationNotTested, EFI_SUCCESS),
    EcitCoverageNotAdvertised
    );
  UT_ASSERT_EQUAL (
    CorrelateEcitResult (EcitClaimUnknown, EcitObservationSuccess, EFI_SUCCESS),
    EcitCoverageInconclusive
    );
  UT_ASSERT_EQUAL (
    CorrelateEcitResult (EcitClaimAdvertised, EcitObservationNotTested, EFI_SUCCESS),
    EcitCoverageNotTested
    );
  UT_ASSERT_EQUAL (
    CorrelateEcitResult (EcitClaimAdvertised, EcitObservationInconclusive, EFI_SUCCESS),
    EcitCoverageInconclusive
    );
  UT_ASSERT_EQUAL (
    FindEcitOidPairClaim (
      &gEfiEcitFeatureSbImageVerificationGuid,
      "2.16.840.1.101.3.4.3.19",
      "2.16.840.1.101.3.4.2.3"
      ),
    EcitImageVerificationMldsa87Sha512Profile
    );
  UT_ASSERT_EQUAL (
    FindEcitOidPairClaim (
      &gEfiEcitFeatureSbImageVerificationGuid,
      "2.16.840.1.101.3.4.3.19",
      "2.16.840.1.101.3.4.2.1"
      ),
    EcitClaimCount
    );

  UT_ASSERT_STATUS_EQUAL (
    BuildImageCapabilityPath (0, FALSE, &Path),
    EFI_NOT_FOUND
    );
  UT_ASSERT_STATUS_EQUAL (
    BuildImageCapabilityPath (AUDIT_IMAGE_ALGORITHM_ALL + 1, FALSE, &Path),
    EFI_INVALID_PARAMETER
    );

  for (CaseIndex = 0; CaseIndex < ARRAY_SIZE (mPathCases); CaseIndex++) {
    UT_ASSERT_STATUS_EQUAL (
      BuildImageCapabilityPath (mPathCases[CaseIndex].Mask, FALSE, &Path),
      EFI_SUCCESS
      );
    UT_ASSERT_EQUAL (Path.Count, mPathCases[CaseIndex].Count);
    for (AlgorithmIndex = 0; AlgorithmIndex < Path.Count; AlgorithmIndex++) {
      UT_ASSERT_EQUAL (
        Path.Algorithms[AlgorithmIndex],
        mPathCases[CaseIndex].Algorithms[AlgorithmIndex]
        );
    }
  }

  UT_ASSERT_STATUS_EQUAL (
    BuildImageCapabilityPath (AUDIT_IMAGE_ALGORITHM_RSA, TRUE, &Path),
    EFI_SUCCESS
    );
  UT_ASSERT_EQUAL (Path.Count, 3);
  UT_ASSERT_EQUAL (Path.Algorithms[0], AUDIT_IMAGE_ALGORITHM_RSA);
  UT_ASSERT_EQUAL (Path.Algorithms[1], AUDIT_IMAGE_ALGORITHM_ECDSA);
  UT_ASSERT_EQUAL (Path.Algorithms[2], AUDIT_IMAGE_ALGORITHM_ML_DSA);

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
      { SETUP_MODE, SECURE_BOOT_MODE_DISABLE, AuditVariableStateAbsent,  0,                          0, AuditVariableStateAbsent,         0,                          0, AuditVariableStateAbsent,               0,                          0 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkA,     SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateAbsent,         0,                          0, AuditVariableStateAbsent,               0,                          0 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkB,     SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateAbsent,         0,                          0, AuditVariableStateAbsent,               0,                          0 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateAbsent,         0,                          0, AuditVariableStateAbsent,               0,                          0 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkEcdsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateAbsent,         0,                          0, AuditVariableStateAbsent,               0,                          0 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbV1,           SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateAbsent,               0,                          0 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbV2,           SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateAbsent,               0,                          0 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbMultiSigner,  SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbxRsaSigner,         SECURE_BOOT_KEY_ATTRIBUTES, 1 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkA,     SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbCaMigration,  SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbxMigrationRsaEcdsa, SECURE_BOOT_KEY_ATTRIBUTES, 1 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkEcdsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbCaCapability, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbxCaCapability,      SECURE_BOOT_KEY_ATTRIBUTES, 1 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStateUnknown, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbV1,           SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateAbsent,               0,                          0 }, FALSE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkA,     SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbV1,           SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateAbsent,               0,                          0 }, FALSE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateUnknown,        SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateAbsent,               0,                          0 }, FALSE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, 0,                          1, AuditVariableStateAbsent,         0,                          0, AuditVariableStateAbsent,               0,                          0 }, FALSE
    },
    {
      { SETUP_MODE, SECURE_BOOT_MODE_DISABLE, AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateAbsent,         0,                          0, AuditVariableStateAbsent,               0,                          0 }, FALSE
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
  Journal->SetupMode     = Snapshot->SetupMode;
  Journal->SecureBoot    = Snapshot->SecureBoot;
  Journal->PkState       = Snapshot->PkState;
  Journal->PkAttributes  = Snapshot->PkAttributes;
  Journal->PkSize        = Snapshot->PkSize;
  Journal->DbState       = Snapshot->DbState;
  Journal->DbAttributes  = Snapshot->DbAttributes;
  Journal->DbSize        = Snapshot->DbSize;
  Journal->DbxState      = Snapshot->DbxState;
  Journal->DbxAttributes = Snapshot->DbxAttributes;
  Journal->DbxSize       = Snapshot->DbxSize;
}

STATIC
EFI_STATUS
CompareJournalState (
  IN CONST AUDIT_JOURNAL            *Journal,
  IN CONST AUDIT_BASELINE_SNAPSHOT  *Snapshot
  )
{
  if ((Journal->SetupMode != Snapshot->SetupMode) ||
      (Journal->SecureBoot != Snapshot->SecureBoot) ||
      (Journal->PkState != (UINT32)Snapshot->PkState) ||
      (Journal->PkAttributes != Snapshot->PkAttributes) ||
      (Journal->PkSize != Snapshot->PkSize) ||
      (Journal->DbState != (UINT32)Snapshot->DbState) ||
      (Journal->DbAttributes != Snapshot->DbAttributes) ||
      (Journal->DbSize != Snapshot->DbSize) ||
      (Journal->DbxState != (UINT32)Snapshot->DbxState) ||
      (Journal->DbxAttributes != Snapshot->DbxAttributes) ||
      (Journal->DbxSize != Snapshot->DbxSize))
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
  UINT8   Marker[AUDIT_MANIFEST_SHA256_SIZE];
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
    "framework_context=RESTORED ecit_reread=VALID state_reread=MATCH\n",
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
LoadAndStartImage (
  IN CONST CHAR8  *Phase,
  IN CONST VOID   *Image,
  IN UINTN        ImageSize
  )
{
  EFI_HANDLE  LoadedImage;
  EFI_STATUS  Status;

  LoadedImage = NULL;
  Status      = gBS->LoadImage (
                       FALSE,
                       gImageHandle,
                       (EFI_DEVICE_PATH_PROTOCOL *)&mTestImageDevicePath,
                       (VOID *)Image,
                       ImageSize,
                       &LoadedImage
                       );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("%a: image load failed: %r\n", Phase, Status);
    return Status;
  }

  Status = gBS->StartImage (LoadedImage, NULL, NULL);
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("%a: image start failed: %r\n", Phase, Status);
    gBS->UnloadImage (LoadedImage);
    return Status;
  }

  UT_LOG_INFO ("%a: image loaded and executed\n", Phase);
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
  EnrollScenario.CoveredClaims           = mMldsaUpdateClaims;
  EnrollScenario.CoveredClaimCount       = ARRAY_SIZE (mMldsaUpdateClaims);

  Status = ExecuteAuthVariableScenario (
             &EnrollScenario,
             mEcitClaimResults,
             ARRAY_SIZE (mEcitClaimResults)
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

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

  Status = ExecuteAuthVariableScenario (
             &ReplayScenario,
             mEcitClaimResults,
             ARRAY_SIZE (mEcitClaimResults)
             );
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
  IN UINTN          ExpectedDbSize,
  IN CONST UINT8    *DeleteDb,
  IN UINTN          DeleteDbSize
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
    RecordEcitObservation (ImageAuthorizationClaim, EcitObservationFailure, Status);
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
             DeleteDb,
             DeleteDbSize,
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
           gSecureBootKeyAuditDbTbsHashV1Size,
           gSecureBootKeyAuditDeleteDbV1ByMldsa,
           gSecureBootKeyAuditDeleteDbV1ByMldsaSize
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
           gSecureBootKeyAuditDbTbsHashV2Size,
           gSecureBootKeyAuditDeleteDbV2ByMldsa,
           gSecureBootKeyAuditDeleteDbV2ByMldsaSize
           );
}

STATIC
UNIT_TEST_STATUS
EFIAPI
MultiSignatureRevocationTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN           ImageEvidenceAttempted;
  EFI_STATUS        RecoveryStatus;
  UNIT_TEST_STATUS  Result;
  EFI_STATUS        Status;

  ImageEvidenceAttempted = FALSE;
  Result                 = UNIT_TEST_ERROR_TEST_FAILED;

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
             "RSA-first multi-signed image before db enrollment",
             gSecureBootKeyAuditHelloRsaMldsa,
             gSecureBootKeyAuditHelloRsaMldsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LoadImageExpectDenied (
             "ML-DSA-first multi-signed image before db enrollment",
             gSecureBootKeyAuditHelloMldsaRsa,
             gSecureBootKeyAuditHelloMldsaRsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = ApplyDbUpdate (
             gSecureBootKeyAuditEnrollDbMultiSigner,
             gSecureBootKeyAuditEnrollDbMultiSignerSize,
             gSecureBootKeyAuditDbMultiSigner,
             gSecureBootKeyAuditDbMultiSignerSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not enroll both image signers in db: %r\n", Status);
    goto Exit;
  }

  RecordEcitObservations (
    mMldsaUpdateClaims,
    ARRAY_SIZE (mMldsaUpdateClaims),
    EcitObservationSuccess,
    EFI_SUCCESS
    );
  ImageEvidenceAttempted = TRUE;

  Status = LoadAndStartImage (
             "RSA-first multi-signed image with both signers authorized",
             gSecureBootKeyAuditHelloRsaMldsa,
             gSecureBootKeyAuditHelloRsaMldsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LoadAndStartImage (
             "ML-DSA-first multi-signed image with both signers authorized",
             gSecureBootKeyAuditHelloMldsaRsa,
             gSecureBootKeyAuditHelloMldsaRsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = ApplyDbxUpdate (
             gSecureBootKeyAuditEnrollDbxRsaSigner,
             gSecureBootKeyAuditEnrollDbxRsaSignerSize,
             gSecureBootKeyAuditDbxRsaSigner,
             gSecureBootKeyAuditDbxRsaSignerSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not revoke the RSA image signer: %r\n", Status);
    goto Exit;
  }

  Status = LoadAndStartImage (
             "ML-DSA-first control after RSA signer revocation",
             gSecureBootKeyAuditHelloMldsaRsa,
             gSecureBootKeyAuditHelloMldsaRsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LoadAndStartImage (
             "RSA-first image after RSA signer revocation",
             gSecureBootKeyAuditHelloRsaMldsa,
             gSecureBootKeyAuditHelloRsaMldsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = ApplyDbxUpdate (
             gSecureBootKeyAuditEnrollDbxMldsaSigner,
             gSecureBootKeyAuditEnrollDbxMldsaSignerSize,
             gSecureBootKeyAuditDbxMldsaSigner,
             gSecureBootKeyAuditDbxMldsaSignerSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not revoke the ML-DSA image signer: %r\n", Status);
    goto Exit;
  }

  Status = LoadAndStartImage (
             "RSA-first control after ML-DSA signer revocation",
             gSecureBootKeyAuditHelloRsaMldsa,
             gSecureBootKeyAuditHelloRsaMldsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LoadAndStartImage (
             "ML-DSA-first image after ML-DSA signer revocation",
             gSecureBootKeyAuditHelloMldsaRsa,
             gSecureBootKeyAuditHelloMldsaRsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = ApplyDbxUpdate (
             gSecureBootKeyAuditEnrollDbxBothSigners,
             gSecureBootKeyAuditEnrollDbxBothSignersSize,
             gSecureBootKeyAuditDbxBothSigners,
             gSecureBootKeyAuditDbxBothSignersSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not revoke both image signers: %r\n", Status);
    goto Exit;
  }

  Status = LoadImageExpectDenied (
             "RSA-first image after both signer revocations",
             gSecureBootKeyAuditHelloRsaMldsa,
             gSecureBootKeyAuditHelloRsaMldsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LoadImageExpectDenied (
             "ML-DSA-first image after both signer revocations",
             gSecureBootKeyAuditHelloMldsaRsa,
             gSecureBootKeyAuditHelloMldsaRsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  RecordEcitObservation (
    EcitImageAuthorizationTbsV2Sha512,
    EcitObservationSuccess,
    EFI_SUCCESS
    );
  RecordEcitObservation (
    EcitImageRevocationTbsV2Sha512,
    EcitObservationSuccess,
    EFI_SUCCESS
    );
  RecordEcitObservations (
    mRsaImageVerificationClaims,
    ARRAY_SIZE (mRsaImageVerificationClaims),
    EcitObservationSuccess,
    EFI_SUCCESS
    );
  RecordEcitObservations (
    mMldsaImageVerificationClaims,
    ARRAY_SIZE (mMldsaImageVerificationClaims),
    EcitObservationSuccess,
    EFI_SUCCESS
    );

  Status = ApplyDbxUpdate (
             gSecureBootKeyAuditDeleteDbxByMldsa,
             gSecureBootKeyAuditDeleteDbxByMldsaSize,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not delete the multi-signature test dbx: %r\n", Status);
    goto Exit;
  }

  Status = ApplyDbUpdate (
             gSecureBootKeyAuditDeleteDbMultiSignerByMldsa,
             gSecureBootKeyAuditDeleteDbMultiSignerByMldsaSize,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not delete the multi-signature test db: %r\n", Status);
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
  if ((Result != UNIT_TEST_PASSED) && ImageEvidenceAttempted) {
    RecordEcitObservation (
      EcitImageAuthorizationTbsV2Sha512,
      EcitObservationFailure,
      Status
      );
    RecordEcitObservation (
      EcitImageRevocationTbsV2Sha512,
      EcitObservationFailure,
      Status
      );
    RecordEcitObservations (
      mRsaImageVerificationClaims,
      ARRAY_SIZE (mRsaImageVerificationClaims),
      EcitObservationFailure,
      Status
      );
    RecordEcitObservations (
      mMldsaImageVerificationClaims,
      ARRAY_SIZE (mMldsaImageVerificationClaims),
      EcitObservationFailure,
      Status
      );
  }

  RecoveryStatus = RecoverSetupMode ();
  if (EFI_ERROR (RecoveryStatus)) {
    UT_LOG_ERROR ("Unable to recover the multi-signature test state: %r\n", RecoveryStatus);
    Result = UNIT_TEST_ERROR_TEST_FAILED;
  }

  return Result;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
RunThreeCaRetirementTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN           ImageEvidenceAttempted;
  EFI_STATUS        RecoveryStatus;
  UNIT_TEST_STATUS  Result;
  EFI_STATUS        Status;

  ImageEvidenceAttempted = FALSE;
  Result                 = UNIT_TEST_ERROR_TEST_FAILED;

  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The Secure Boot baseline is unsafe: %r\n", Status);
    goto Exit;
  }

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditSetupPkA,
             gSecureBootKeyAuditSetupPkASize,
             gSecureBootKeyAuditPkA,
             gSecureBootKeyAuditPkASize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Setup Mode enrollment of RSA PK A failed: %r\n", Status);
    goto Exit;
  }

  Status = RequireSecureBootEnforcement ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LoadImageExpectDenied (
             "Three-CA image before db enrollment",
             gSecureBootKeyAuditHelloRsaEcdsaMldsa,
             gSecureBootKeyAuditHelloRsaEcdsaMldsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = ApplyDbUpdate (
             gSecureBootKeyAuditEnrollDbCaMigration,
             gSecureBootKeyAuditEnrollDbCaMigrationSize,
             gSecureBootKeyAuditDbCaMigration,
             gSecureBootKeyAuditDbCaMigrationSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not authorize all three image CAs in db: %r\n", Status);
    goto Exit;
  }

  RecordEcitObservations (
    mRsaUpdateClaims,
    ARRAY_SIZE (mRsaUpdateClaims),
    EcitObservationSuccess,
    EFI_SUCCESS
    );
  ImageEvidenceAttempted = TRUE;

  Status = LoadAndStartImage (
             "RSA-only control before CA retirement",
             gSecureBootKeyAuditHelloRsa,
             gSecureBootKeyAuditHelloRsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LoadAndStartImage (
             "ECDSA-only control before CA retirement",
             gSecureBootKeyAuditHelloEcdsa,
             gSecureBootKeyAuditHelloEcdsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LoadAndStartImage (
             "ML-DSA-only control before CA retirement",
             gSecureBootKeyAuditHelloMldsa,
             gSecureBootKeyAuditHelloMldsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LoadAndStartImage (
             "Three-CA image before CA retirement",
             gSecureBootKeyAuditHelloRsaEcdsaMldsa,
             gSecureBootKeyAuditHelloRsaEcdsaMldsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = ApplyDbxUpdate (
             gSecureBootKeyAuditEnrollDbxMigrationRsa,
             gSecureBootKeyAuditEnrollDbxMigrationRsaSize,
             gSecureBootKeyAuditDbxMigrationRsa,
             gSecureBootKeyAuditDbxMigrationRsaSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not retire the RSA image CA: %r\n", Status);
    goto Exit;
  }

  Status = LoadImageExpectDenied (
             "RSA-only control after RSA CA retirement",
             gSecureBootKeyAuditHelloRsa,
             gSecureBootKeyAuditHelloRsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LoadAndStartImage (
             "Three-CA image after RSA CA retirement",
             gSecureBootKeyAuditHelloRsaEcdsaMldsa,
             gSecureBootKeyAuditHelloRsaEcdsaMldsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = ApplyDbxUpdate (
             gSecureBootKeyAuditEnrollDbxMigrationRsaEcdsa,
             gSecureBootKeyAuditEnrollDbxMigrationRsaEcdsaSize,
             gSecureBootKeyAuditDbxMigrationRsaEcdsa,
             gSecureBootKeyAuditDbxMigrationRsaEcdsaSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not retire the ECDSA image CA cumulatively: %r\n", Status);
    goto Exit;
  }

  Status = LoadImageExpectDenied (
             "ECDSA-only control after ECDSA CA retirement",
             gSecureBootKeyAuditHelloEcdsa,
             gSecureBootKeyAuditHelloEcdsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LoadAndStartImage (
             "ML-DSA-only control after RSA and ECDSA CA retirement",
             gSecureBootKeyAuditHelloMldsa,
             gSecureBootKeyAuditHelloMldsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LoadAndStartImage (
             "Three-CA image after RSA and ECDSA CA retirement",
             gSecureBootKeyAuditHelloRsaEcdsaMldsa,
             gSecureBootKeyAuditHelloRsaEcdsaMldsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = ApplyDbxUpdate (
             gSecureBootKeyAuditEnrollDbxMigrationAll,
             gSecureBootKeyAuditEnrollDbxMigrationAllSize,
             gSecureBootKeyAuditDbxMigrationAll,
             gSecureBootKeyAuditDbxMigrationAllSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not retire the final ML-DSA image CA: %r\n", Status);
    goto Exit;
  }

  Status = LoadImageExpectDenied (
             "ML-DSA-only control after final CA retirement",
             gSecureBootKeyAuditHelloMldsa,
             gSecureBootKeyAuditHelloMldsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LoadImageExpectDenied (
             "Three-CA image after final CA retirement",
             gSecureBootKeyAuditHelloRsaEcdsaMldsa,
             gSecureBootKeyAuditHelloRsaEcdsaMldsaSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  RecordEcitObservation (
    EcitImageAuthorizationTbsV2Sha512,
    EcitObservationSuccess,
    EFI_SUCCESS
    );
  RecordEcitObservation (
    EcitImageRevocationTbsV2Sha512,
    EcitObservationSuccess,
    EFI_SUCCESS
    );
  RecordEcitObservations (
    mRsaImageVerificationClaims,
    ARRAY_SIZE (mRsaImageVerificationClaims),
    EcitObservationSuccess,
    EFI_SUCCESS
    );
  RecordEcitObservations (
    mEcdsaImageVerificationClaims,
    ARRAY_SIZE (mEcdsaImageVerificationClaims),
    EcitObservationSuccess,
    EFI_SUCCESS
    );
  RecordEcitObservations (
    mMldsaImageVerificationClaims,
    ARRAY_SIZE (mMldsaImageVerificationClaims),
    EcitObservationSuccess,
    EFI_SUCCESS
    );

  Status = ApplyDbxUpdate (
             gSecureBootKeyAuditDeleteDbxCaMigrationByA,
             gSecureBootKeyAuditDeleteDbxCaMigrationByASize,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not delete the cumulative CA migration dbx: %r\n", Status);
    goto Exit;
  }

  Status = ApplyDbUpdate (
             gSecureBootKeyAuditDeleteDbCaMigrationByA,
             gSecureBootKeyAuditDeleteDbCaMigrationByASize,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not delete the cumulative CA migration db: %r\n", Status);
    goto Exit;
  }

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditDeleteByA,
             gSecureBootKeyAuditDeleteByASize,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("RSA PK A could not authenticate cleanup: %r\n", Status);
    goto Exit;
  }

  Result = UNIT_TEST_PASSED;

Exit:
  if ((Result != UNIT_TEST_PASSED) && ImageEvidenceAttempted) {
    RecordEcitObservation (
      EcitImageAuthorizationTbsV2Sha512,
      EcitObservationFailure,
      Status
      );
    RecordEcitObservation (
      EcitImageRevocationTbsV2Sha512,
      EcitObservationFailure,
      Status
      );
    RecordEcitObservations (
      mRsaImageVerificationClaims,
      ARRAY_SIZE (mRsaImageVerificationClaims),
      EcitObservationFailure,
      Status
      );
    RecordEcitObservations (
      mEcdsaImageVerificationClaims,
      ARRAY_SIZE (mEcdsaImageVerificationClaims),
      EcitObservationFailure,
      Status
      );
    RecordEcitObservations (
      mMldsaImageVerificationClaims,
      ARRAY_SIZE (mMldsaImageVerificationClaims),
      EcitObservationFailure,
      Status
      );
  }

  RecoveryStatus = RecoverSetupMode ();
  if (EFI_ERROR (RecoveryStatus)) {
    UT_LOG_ERROR ("Unable to recover the cumulative CA migration state: %r\n", RecoveryStatus);
    Result = UNIT_TEST_ERROR_TEST_FAILED;
  }

  return Result;
}

typedef struct {
  CONST UINT8    *Update;
  UINTN          UpdateSize;
  CONST UINT8    *Expected;
  UINTN          ExpectedSize;
  UINT8          RetiredAlgorithm;
} CAPABILITY_RETIREMENT_STEP;

typedef struct {
  UINT8                         Mask;
  UINT8                         AuthorityAlgorithm;
  CONST UINT8                   *SetupPk;
  UINTN                         SetupPkSize;
  CONST UINT8                   *ExpectedPk;
  UINTN                         ExpectedPkSize;
  CONST UINT8                   *EnrollDb;
  UINTN                         EnrollDbSize;
  CONST UINT8                   *ExpectedDb;
  UINTN                         ExpectedDbSize;
  CONST UINT8                   *MultiImage;
  UINTN                         MultiImageSize;
  CAPABILITY_RETIREMENT_STEP    Steps[AUDIT_MAX_CAPABILITY_PATH_LENGTH];
  UINTN                         StepCount;
  CONST UINT8                   *DeleteDbx;
  UINTN                         DeleteDbxSize;
  CONST UINT8                   *DeleteDb;
  UINTN                         DeleteDbSize;
  CONST UINT8                   *DeletePk;
  UINTN                         DeletePkSize;
} CAPABILITY_RETIREMENT_FIXTURE;

STATIC
CONST CHAR8 *
GetAlgorithmName (
  IN UINT8  Algorithm
  )
{
  switch (Algorithm) {
    case AUDIT_IMAGE_ALGORITHM_RSA:
      return "RSA";
    case AUDIT_IMAGE_ALGORITHM_ECDSA:
      return "ECDSA";
    case AUDIT_IMAGE_ALGORITHM_ML_DSA:
      return "ML-DSA";
    default:
      return "UNKNOWN";
  }
}

STATIC
EFI_STATUS
GetAlgorithmImage (
  IN UINT8         Algorithm,
  OUT CONST CHAR8  **Name,
  OUT CONST UINT8  **Image,
  OUT UINTN        *ImageSize
  )
{
  *Name = GetAlgorithmName (Algorithm);
  switch (Algorithm) {
    case AUDIT_IMAGE_ALGORITHM_RSA:
      *Image     = gSecureBootKeyAuditHelloRsa;
      *ImageSize = gSecureBootKeyAuditHelloRsaSize;
      return EFI_SUCCESS;
    case AUDIT_IMAGE_ALGORITHM_ECDSA:
      *Image     = gSecureBootKeyAuditHelloEcdsa;
      *ImageSize = gSecureBootKeyAuditHelloEcdsaSize;
      return EFI_SUCCESS;
    case AUDIT_IMAGE_ALGORITHM_ML_DSA:
      *Image     = gSecureBootKeyAuditHelloMldsa;
      *ImageSize = gSecureBootKeyAuditHelloMldsaSize;
      return EFI_SUCCESS;
    default:
      return EFI_INVALID_PARAMETER;
  }
}

STATIC
VOID
RecordAlgorithmImageObservation (
  IN UINT8                   Algorithm,
  IN ECIT_OBSERVATION_STATE  Observation,
  IN EFI_STATUS              Status
  )
{
  switch (Algorithm) {
    case AUDIT_IMAGE_ALGORITHM_RSA:
      RecordEcitObservations (
        mRsaImageVerificationClaims,
        ARRAY_SIZE (mRsaImageVerificationClaims),
        Observation,
        Status
        );
      break;
    case AUDIT_IMAGE_ALGORITHM_ECDSA:
      RecordEcitObservations (
        mEcdsaImageVerificationClaims,
        ARRAY_SIZE (mEcdsaImageVerificationClaims),
        Observation,
        Status
        );
      break;
    case AUDIT_IMAGE_ALGORITHM_ML_DSA:
      RecordEcitObservations (
        mMldsaImageVerificationClaims,
        ARRAY_SIZE (mMldsaImageVerificationClaims),
        Observation,
        Status
        );
      break;
    default:
      break;
  }
}

STATIC
VOID
RecordAuthorityUpdateObservation (
  IN UINT8                   Algorithm,
  IN ECIT_OBSERVATION_STATE  Observation,
  IN EFI_STATUS              Status
  )
{
  switch (Algorithm) {
    case AUDIT_IMAGE_ALGORITHM_RSA:
      RecordEcitObservations (
        mRsaUpdateClaims,
        ARRAY_SIZE (mRsaUpdateClaims),
        Observation,
        Status
        );
      break;
    case AUDIT_IMAGE_ALGORITHM_ECDSA:
      RecordEcitObservations (
        mEcdsaUpdateClaims,
        ARRAY_SIZE (mEcdsaUpdateClaims),
        Observation,
        Status
        );
      break;
    case AUDIT_IMAGE_ALGORITHM_ML_DSA:
      RecordEcitObservations (
        mMldsaUpdateClaims,
        ARRAY_SIZE (mMldsaUpdateClaims),
        Observation,
        Status
        );
      break;
    default:
      break;
  }
}

STATIC
UNIT_TEST_STATUS
EFIAPI
CapabilityAuthorityPathTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  AUDIT_CAPABILITY_PATH  Path;
  CONST UINT8            *DeletePk;
  UINTN                  DeletePkSize;
  CONST UINT8            *ExpectedPk;
  UINTN                  ExpectedPkSize;
  CONST UINT8            *SetupPk;
  UINTN                  SetupPkSize;
  CONST UINT8            *Transition;
  UINTN                  TransitionSize;
  UINT8                  CurrentAlgorithm;
  UINT8                  Mask;
  EFI_STATUS             RecoveryStatus;
  UNIT_TEST_STATUS       Result;
  EFI_STATUS             Status;
  UINTN                  Index;

  Result = UNIT_TEST_ERROR_TEST_FAILED;
  Mask   = GetAdvertisedUpdateAlgorithmMask (
             mEcitClaimResults,
             ARRAY_SIZE (mEcitClaimResults)
             );
  UT_LOG_INFO ("Advertised update-algorithm mask: 0x%x\n", Mask);
  if (Mask == 0) {
    UT_LOG_INFO ("No fixture-backed update algorithm is advertised; authority path is not applicable\n");
    return UNIT_TEST_PASSED;
  }

  Status = BuildImageCapabilityPath (Mask, FALSE, &Path);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  switch (Path.Algorithms[0]) {
    case AUDIT_IMAGE_ALGORITHM_RSA:
      SetupPk        = gSecureBootKeyAuditSetupPkA;
      SetupPkSize    = gSecureBootKeyAuditSetupPkASize;
      ExpectedPk     = gSecureBootKeyAuditPkA;
      ExpectedPkSize = gSecureBootKeyAuditPkASize;
      break;
    case AUDIT_IMAGE_ALGORITHM_ECDSA:
      SetupPk        = gSecureBootKeyAuditSetupPkEcdsa;
      SetupPkSize    = gSecureBootKeyAuditSetupPkEcdsaSize;
      ExpectedPk     = gSecureBootKeyAuditPkEcdsa;
      ExpectedPkSize = gSecureBootKeyAuditPkEcdsaSize;
      break;
    case AUDIT_IMAGE_ALGORITHM_ML_DSA:
      SetupPk        = gSecureBootKeyAuditSetupPkMldsa;
      SetupPkSize    = gSecureBootKeyAuditSetupPkMldsaSize;
      ExpectedPk     = gSecureBootKeyAuditPkMldsa;
      ExpectedPkSize = gSecureBootKeyAuditPkMldsaSize;
      break;
    default:
      return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = ApplyPkUpdate (SetupPk, SetupPkSize, ExpectedPk, ExpectedPkSize);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = RequireSecureBootEnforcement ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  CurrentAlgorithm = Path.Algorithms[0];
  for (Index = 1; Index < Path.Count; Index++) {
    Transition     = NULL;
    TransitionSize = 0;
    ExpectedPk     = NULL;
    ExpectedPkSize = 0;

    if ((CurrentAlgorithm == AUDIT_IMAGE_ALGORITHM_RSA) &&
        (Path.Algorithms[Index] == AUDIT_IMAGE_ALGORITHM_ECDSA))
    {
      Transition     = gSecureBootKeyAuditPkAToEcdsa;
      TransitionSize = gSecureBootKeyAuditPkAToEcdsaSize;
      ExpectedPk     = gSecureBootKeyAuditPkEcdsa;
      ExpectedPkSize = gSecureBootKeyAuditPkEcdsaSize;
    } else if ((CurrentAlgorithm == AUDIT_IMAGE_ALGORITHM_RSA) &&
               (Path.Algorithms[Index] == AUDIT_IMAGE_ALGORITHM_ML_DSA))
    {
      Transition     = gSecureBootKeyAuditPkAToMldsa;
      TransitionSize = gSecureBootKeyAuditPkAToMldsaSize;
      ExpectedPk     = gSecureBootKeyAuditPkMldsa;
      ExpectedPkSize = gSecureBootKeyAuditPkMldsaSize;
    } else if ((CurrentAlgorithm == AUDIT_IMAGE_ALGORITHM_ECDSA) &&
               (Path.Algorithms[Index] == AUDIT_IMAGE_ALGORITHM_ML_DSA))
    {
      Transition     = gSecureBootKeyAuditPkEcdsaToMldsa;
      TransitionSize = gSecureBootKeyAuditPkEcdsaToMldsaSize;
      ExpectedPk     = gSecureBootKeyAuditPkMldsa;
      ExpectedPkSize = gSecureBootKeyAuditPkMldsaSize;
    }

    if (Transition == NULL) {
      Status = EFI_UNSUPPORTED;
      goto Exit;
    }

    UT_LOG_INFO (
      "Applying authority transition %a -> %a\n",
      GetAlgorithmName (CurrentAlgorithm),
      GetAlgorithmName (Path.Algorithms[Index])
      );
    Status = ApplyPkUpdate (
               Transition,
               TransitionSize,
               ExpectedPk,
               ExpectedPkSize
               );
    if (EFI_ERROR (Status)) {
      UT_LOG_ERROR (
        "Authority transition %a -> %a failed: %r\n",
        GetAlgorithmName (CurrentAlgorithm),
        GetAlgorithmName (Path.Algorithms[Index]),
        Status
        );
      RecordAuthorityUpdateObservation (
        CurrentAlgorithm,
        EcitObservationFailure,
        Status
        );
      goto Exit;
    }

    RecordAuthorityUpdateObservation (
      CurrentAlgorithm,
      EcitObservationSuccess,
      EFI_SUCCESS
      );
    CurrentAlgorithm = Path.Algorithms[Index];
  }

  switch (CurrentAlgorithm) {
    case AUDIT_IMAGE_ALGORITHM_RSA:
      DeletePk     = gSecureBootKeyAuditDeleteByA;
      DeletePkSize = gSecureBootKeyAuditDeleteByASize;
      break;
    case AUDIT_IMAGE_ALGORITHM_ECDSA:
      if (Path.Count == 1) {
        DeletePk     = gSecureBootKeyAuditDeleteByEcdsa;
        DeletePkSize = gSecureBootKeyAuditDeleteByEcdsaSize;
      } else {
        DeletePk     = gSecureBootKeyAuditDeleteByEcdsaAfterTransition;
        DeletePkSize = gSecureBootKeyAuditDeleteByEcdsaAfterTransitionSize;
      }

      break;
    case AUDIT_IMAGE_ALGORITHM_ML_DSA:
      if (Path.Count == 1) {
        DeletePk     = gSecureBootKeyAuditDeleteByMldsa;
        DeletePkSize = gSecureBootKeyAuditDeleteByMldsaSize;
      } else {
        DeletePk     = gSecureBootKeyAuditDeleteByMldsaAfterTransition;
        DeletePkSize = gSecureBootKeyAuditDeleteByMldsaAfterTransitionSize;
      }

      break;
    default:
      Status = EFI_UNSUPPORTED;
      goto Exit;
  }

  UT_LOG_INFO (
    "Applying final %a-authorized PK deletion\n",
    GetAlgorithmName (CurrentAlgorithm)
    );
  Status = ApplyPkUpdate (DeletePk, DeletePkSize, NULL, 0);
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR (
      "Final %a-authorized PK deletion failed: %r\n",
      GetAlgorithmName (CurrentAlgorithm),
      Status
      );
    RecordAuthorityUpdateObservation (
      CurrentAlgorithm,
      EcitObservationFailure,
      Status
      );
    goto Exit;
  }

  RecordAuthorityUpdateObservation (
    CurrentAlgorithm,
    EcitObservationSuccess,
    EFI_SUCCESS
    );
  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Result = UNIT_TEST_PASSED;

Exit:
  RecoveryStatus = RecoverSetupMode ();
  if (EFI_ERROR (RecoveryStatus)) {
    UT_LOG_ERROR ("Unable to recover the capability authority path: %r\n", RecoveryStatus);
    Result = UNIT_TEST_ERROR_TEST_FAILED;
  }

  return Result;
}

STATIC
EFI_STATUS
SelectCapabilityRetirementFixture (
  IN UINT8                           Mask,
  OUT CAPABILITY_RETIREMENT_FIXTURE  *Fixture
  )
{
  ZeroMem (Fixture, sizeof (*Fixture));
  Fixture->Mask = Mask;

  if ((Mask & AUDIT_IMAGE_ALGORITHM_RSA) != 0) {
    Fixture->AuthorityAlgorithm = AUDIT_IMAGE_ALGORITHM_RSA;
    Fixture->SetupPk            = gSecureBootKeyAuditSetupPkA;
    Fixture->SetupPkSize        = gSecureBootKeyAuditSetupPkASize;
    Fixture->ExpectedPk         = gSecureBootKeyAuditPkA;
    Fixture->ExpectedPkSize     = gSecureBootKeyAuditPkASize;
    Fixture->DeleteDbx          = gSecureBootKeyAuditDeleteDbxCaMigrationByA;
    Fixture->DeleteDbxSize      = gSecureBootKeyAuditDeleteDbxCaMigrationByASize;
    Fixture->DeleteDb           = gSecureBootKeyAuditDeleteDbCaMigrationByA;
    Fixture->DeleteDbSize       = gSecureBootKeyAuditDeleteDbCaMigrationByASize;
    Fixture->DeletePk           = gSecureBootKeyAuditDeleteByA;
    Fixture->DeletePkSize       = gSecureBootKeyAuditDeleteByASize;
  } else if ((Mask & AUDIT_IMAGE_ALGORITHM_ECDSA) != 0) {
    Fixture->AuthorityAlgorithm = AUDIT_IMAGE_ALGORITHM_ECDSA;
    Fixture->SetupPk            = gSecureBootKeyAuditSetupPkEcdsa;
    Fixture->SetupPkSize        = gSecureBootKeyAuditSetupPkEcdsaSize;
    Fixture->ExpectedPk         = gSecureBootKeyAuditPkEcdsa;
    Fixture->ExpectedPkSize     = gSecureBootKeyAuditPkEcdsaSize;
    Fixture->DeleteDbx          = gSecureBootKeyAuditDeleteDbxCaMigrationByEcdsa;
    Fixture->DeleteDbxSize      = gSecureBootKeyAuditDeleteDbxCaMigrationByEcdsaSize;
    Fixture->DeleteDb           = gSecureBootKeyAuditDeleteDbCaMigrationByEcdsa;
    Fixture->DeleteDbSize       = gSecureBootKeyAuditDeleteDbCaMigrationByEcdsaSize;
    Fixture->DeletePk           = gSecureBootKeyAuditDeleteByEcdsa;
    Fixture->DeletePkSize       = gSecureBootKeyAuditDeleteByEcdsaSize;
  } else if ((Mask & AUDIT_IMAGE_ALGORITHM_ML_DSA) != 0) {
    Fixture->AuthorityAlgorithm = AUDIT_IMAGE_ALGORITHM_ML_DSA;
    Fixture->SetupPk            = gSecureBootKeyAuditSetupPkMldsa;
    Fixture->SetupPkSize        = gSecureBootKeyAuditSetupPkMldsaSize;
    Fixture->ExpectedPk         = gSecureBootKeyAuditPkMldsa;
    Fixture->ExpectedPkSize     = gSecureBootKeyAuditPkMldsaSize;
    Fixture->DeleteDbx          = gSecureBootKeyAuditDeleteDbxByMldsa;
    Fixture->DeleteDbxSize      = gSecureBootKeyAuditDeleteDbxByMldsaSize;
    Fixture->DeleteDb           = gSecureBootKeyAuditDeleteDbMultiSignerByMldsa;
    Fixture->DeleteDbSize       = gSecureBootKeyAuditDeleteDbMultiSignerByMldsaSize;
    Fixture->DeletePk           = gSecureBootKeyAuditDeleteByMldsa;
    Fixture->DeletePkSize       = gSecureBootKeyAuditDeleteByMldsaSize;
  } else {
    return EFI_NOT_FOUND;
  }

  switch (Mask) {
    case AUDIT_IMAGE_ALGORITHM_RSA:
      Fixture->EnrollDb                  = gSecureBootKeyAuditEnrollDbCaRsaByA;
      Fixture->EnrollDbSize              = gSecureBootKeyAuditEnrollDbCaRsaByASize;
      Fixture->ExpectedDb                = gSecureBootKeyAuditDbCaRsa;
      Fixture->ExpectedDbSize            = gSecureBootKeyAuditDbCaRsaSize;
      Fixture->MultiImage                = gSecureBootKeyAuditHelloRsa;
      Fixture->MultiImageSize            = gSecureBootKeyAuditHelloRsaSize;
      Fixture->Steps[0].Update           = gSecureBootKeyAuditEnrollDbxCaRsaByA;
      Fixture->Steps[0].UpdateSize       = gSecureBootKeyAuditEnrollDbxCaRsaByASize;
      Fixture->Steps[0].Expected         = gSecureBootKeyAuditDbxRsaSigner;
      Fixture->Steps[0].ExpectedSize     = gSecureBootKeyAuditDbxRsaSignerSize;
      Fixture->Steps[0].RetiredAlgorithm = AUDIT_IMAGE_ALGORITHM_RSA;
      Fixture->StepCount                 = 1;
      break;
    case AUDIT_IMAGE_ALGORITHM_ECDSA:
      Fixture->EnrollDb                  = gSecureBootKeyAuditEnrollDbCaEcdsaByEcdsa;
      Fixture->EnrollDbSize              = gSecureBootKeyAuditEnrollDbCaEcdsaByEcdsaSize;
      Fixture->ExpectedDb                = gSecureBootKeyAuditDbCaEcdsa;
      Fixture->ExpectedDbSize            = gSecureBootKeyAuditDbCaEcdsaSize;
      Fixture->MultiImage                = gSecureBootKeyAuditHelloEcdsa;
      Fixture->MultiImageSize            = gSecureBootKeyAuditHelloEcdsaSize;
      Fixture->Steps[0].Update           = gSecureBootKeyAuditEnrollDbxCaEcdsaByEcdsa;
      Fixture->Steps[0].UpdateSize       = gSecureBootKeyAuditEnrollDbxCaEcdsaByEcdsaSize;
      Fixture->Steps[0].Expected         = gSecureBootKeyAuditDbCaEcdsa;
      Fixture->Steps[0].ExpectedSize     = gSecureBootKeyAuditDbCaEcdsaSize;
      Fixture->Steps[0].RetiredAlgorithm = AUDIT_IMAGE_ALGORITHM_ECDSA;
      Fixture->StepCount                 = 1;
      break;
    case AUDIT_IMAGE_ALGORITHM_ML_DSA:
      Fixture->EnrollDb                  = gSecureBootKeyAuditEnrollDbTbsHashV2;
      Fixture->EnrollDbSize              = gSecureBootKeyAuditEnrollDbTbsHashV2Size;
      Fixture->ExpectedDb                = gSecureBootKeyAuditDbTbsHashV2;
      Fixture->ExpectedDbSize            = gSecureBootKeyAuditDbTbsHashV2Size;
      Fixture->MultiImage                = gSecureBootKeyAuditHelloMldsa;
      Fixture->MultiImageSize            = gSecureBootKeyAuditHelloMldsaSize;
      Fixture->Steps[0].Update           = gSecureBootKeyAuditEnrollDbxMldsaSigner;
      Fixture->Steps[0].UpdateSize       = gSecureBootKeyAuditEnrollDbxMldsaSignerSize;
      Fixture->Steps[0].Expected         = gSecureBootKeyAuditDbxMldsaSigner;
      Fixture->Steps[0].ExpectedSize     = gSecureBootKeyAuditDbxMldsaSignerSize;
      Fixture->Steps[0].RetiredAlgorithm = AUDIT_IMAGE_ALGORITHM_ML_DSA;
      Fixture->StepCount                 = 1;
      break;
    case AUDIT_IMAGE_ALGORITHM_RSA | AUDIT_IMAGE_ALGORITHM_ECDSA:
      Fixture->EnrollDb                  = gSecureBootKeyAuditEnrollDbCaRsaEcdsaByA;
      Fixture->EnrollDbSize              = gSecureBootKeyAuditEnrollDbCaRsaEcdsaByASize;
      Fixture->ExpectedDb                = gSecureBootKeyAuditDbCaRsaEcdsa;
      Fixture->ExpectedDbSize            = gSecureBootKeyAuditDbCaRsaEcdsaSize;
      Fixture->MultiImage                = gSecureBootKeyAuditHelloRsaEcdsa;
      Fixture->MultiImageSize            = gSecureBootKeyAuditHelloRsaEcdsaSize;
      Fixture->Steps[0].Update           = gSecureBootKeyAuditEnrollDbxCaRsaByA;
      Fixture->Steps[0].UpdateSize       = gSecureBootKeyAuditEnrollDbxCaRsaByASize;
      Fixture->Steps[0].Expected         = gSecureBootKeyAuditDbxRsaSigner;
      Fixture->Steps[0].ExpectedSize     = gSecureBootKeyAuditDbxRsaSignerSize;
      Fixture->Steps[0].RetiredAlgorithm = AUDIT_IMAGE_ALGORITHM_RSA;
      Fixture->Steps[1].Update           = gSecureBootKeyAuditEnrollDbxMigrationRsaEcdsa;
      Fixture->Steps[1].UpdateSize       = gSecureBootKeyAuditEnrollDbxMigrationRsaEcdsaSize;
      Fixture->Steps[1].Expected         = gSecureBootKeyAuditDbxMigrationRsaEcdsa;
      Fixture->Steps[1].ExpectedSize     = gSecureBootKeyAuditDbxMigrationRsaEcdsaSize;
      Fixture->Steps[1].RetiredAlgorithm = AUDIT_IMAGE_ALGORITHM_ECDSA;
      Fixture->StepCount                 = 2;
      break;
    case AUDIT_IMAGE_ALGORITHM_RSA | AUDIT_IMAGE_ALGORITHM_ML_DSA:
      Fixture->EnrollDb                  = gSecureBootKeyAuditEnrollDbCaRsaMldsaByA;
      Fixture->EnrollDbSize              = gSecureBootKeyAuditEnrollDbCaRsaMldsaByASize;
      Fixture->ExpectedDb                = gSecureBootKeyAuditDbMultiSigner;
      Fixture->ExpectedDbSize            = gSecureBootKeyAuditDbMultiSignerSize;
      Fixture->MultiImage                = gSecureBootKeyAuditHelloRsaMldsa;
      Fixture->MultiImageSize            = gSecureBootKeyAuditHelloRsaMldsaSize;
      Fixture->Steps[0].Update           = gSecureBootKeyAuditEnrollDbxCaRsaByA;
      Fixture->Steps[0].UpdateSize       = gSecureBootKeyAuditEnrollDbxCaRsaByASize;
      Fixture->Steps[0].Expected         = gSecureBootKeyAuditDbxRsaSigner;
      Fixture->Steps[0].ExpectedSize     = gSecureBootKeyAuditDbxRsaSignerSize;
      Fixture->Steps[0].RetiredAlgorithm = AUDIT_IMAGE_ALGORITHM_RSA;
      Fixture->Steps[1].Update           = gSecureBootKeyAuditEnrollDbxCaRsaMldsaByA;
      Fixture->Steps[1].UpdateSize       = gSecureBootKeyAuditEnrollDbxCaRsaMldsaByASize;
      Fixture->Steps[1].Expected         = gSecureBootKeyAuditDbxBothSigners;
      Fixture->Steps[1].ExpectedSize     = gSecureBootKeyAuditDbxBothSignersSize;
      Fixture->Steps[1].RetiredAlgorithm = AUDIT_IMAGE_ALGORITHM_ML_DSA;
      Fixture->StepCount                 = 2;
      break;
    case AUDIT_IMAGE_ALGORITHM_ECDSA | AUDIT_IMAGE_ALGORITHM_ML_DSA:
      Fixture->EnrollDb                  = gSecureBootKeyAuditEnrollDbCaEcdsaMldsaByEcdsa;
      Fixture->EnrollDbSize              = gSecureBootKeyAuditEnrollDbCaEcdsaMldsaByEcdsaSize;
      Fixture->ExpectedDb                = gSecureBootKeyAuditDbCaEcdsaMldsa;
      Fixture->ExpectedDbSize            = gSecureBootKeyAuditDbCaEcdsaMldsaSize;
      Fixture->MultiImage                = gSecureBootKeyAuditHelloEcdsaMldsa;
      Fixture->MultiImageSize            = gSecureBootKeyAuditHelloEcdsaMldsaSize;
      Fixture->Steps[0].Update           = gSecureBootKeyAuditEnrollDbxCaEcdsaByEcdsa;
      Fixture->Steps[0].UpdateSize       = gSecureBootKeyAuditEnrollDbxCaEcdsaByEcdsaSize;
      Fixture->Steps[0].Expected         = gSecureBootKeyAuditDbCaEcdsa;
      Fixture->Steps[0].ExpectedSize     = gSecureBootKeyAuditDbCaEcdsaSize;
      Fixture->Steps[0].RetiredAlgorithm = AUDIT_IMAGE_ALGORITHM_ECDSA;
      Fixture->Steps[1].Update           = gSecureBootKeyAuditEnrollDbxCaEcdsaMldsaByEcdsa;
      Fixture->Steps[1].UpdateSize       = gSecureBootKeyAuditEnrollDbxCaEcdsaMldsaByEcdsaSize;
      Fixture->Steps[1].Expected         = gSecureBootKeyAuditDbCaEcdsaMldsa;
      Fixture->Steps[1].ExpectedSize     = gSecureBootKeyAuditDbCaEcdsaMldsaSize;
      Fixture->Steps[1].RetiredAlgorithm = AUDIT_IMAGE_ALGORITHM_ML_DSA;
      Fixture->StepCount                 = 2;
      break;
    default:
      return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

STATIC
UNIT_TEST_STATUS
RunCapabilityRetirementTest (
  IN UINT8  Mask
  )
{
  AUDIT_CAPABILITY_PATH          Path;
  CAPABILITY_RETIREMENT_FIXTURE  Fixture;
  BOOLEAN                        ImageEvidenceAttempted;
  CONST UINT8                    *Image;
  UINTN                          ImageSize;
  CONST CHAR8                    *Name;
  EFI_STATUS                     RecoveryStatus;
  UNIT_TEST_STATUS               Result;
  EFI_STATUS                     Status;
  UINTN                          Index;

  ImageEvidenceAttempted = FALSE;
  Result                 = UNIT_TEST_ERROR_TEST_FAILED;

  Status = BuildImageCapabilityPath (Mask, FALSE, &Path);
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not build the advertised capability path: %r\n", Status);
    goto Exit;
  }

  Status = SelectCapabilityRetirementFixture (Mask, &Fixture);
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("No fixture covers advertised capability mask 0x%x: %r\n", Mask, Status);
    goto Exit;
  }

  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = ApplyPkUpdate (
             Fixture.SetupPk,
             Fixture.SetupPkSize,
             Fixture.ExpectedPk,
             Fixture.ExpectedPkSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not enroll the selected update authority: %r\n", Status);
    goto Exit;
  }

  Status = RequireSecureBootEnforcement ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LoadImageExpectDenied (
             "Capability image before db enrollment",
             Fixture.MultiImage,
             Fixture.MultiImageSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = ApplyDbUpdate (
             Fixture.EnrollDb,
             Fixture.EnrollDbSize,
             Fixture.ExpectedDb,
             Fixture.ExpectedDbSize
             );
  if (EFI_ERROR (Status)) {
    RecordAuthorityUpdateObservation (
      Fixture.AuthorityAlgorithm,
      EcitObservationFailure,
      Status
      );
    goto Exit;
  }

  RecordAuthorityUpdateObservation (
    Fixture.AuthorityAlgorithm,
    EcitObservationSuccess,
    EFI_SUCCESS
    );
  ImageEvidenceAttempted = TRUE;

  for (Index = 0; Index < Path.Count; Index++) {
    Status = GetAlgorithmImage (
               Path.Algorithms[Index],
               &Name,
               &Image,
               &ImageSize
               );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    Status = LoadAndStartImage (Name, Image, ImageSize);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
  }

  Status = LoadAndStartImage (
             "Selected capability image before retirement",
             Fixture.MultiImage,
             Fixture.MultiImageSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  for (Index = 0; Index < Fixture.StepCount; Index++) {
    Status = ApplyDbxUpdate (
               Fixture.Steps[Index].Update,
               Fixture.Steps[Index].UpdateSize,
               Fixture.Steps[Index].Expected,
               Fixture.Steps[Index].ExpectedSize
               );
    if (EFI_ERROR (Status)) {
      UT_LOG_ERROR ("Could not apply retirement step %u: %r\n", (UINT32)Index, Status);
      goto Exit;
    }

    Status = GetAlgorithmImage (
               Fixture.Steps[Index].RetiredAlgorithm,
               &Name,
               &Image,
               &ImageSize
               );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    Status = LoadImageExpectDenied (Name, Image, ImageSize);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    if (Index + 1 < Fixture.StepCount) {
      Status = GetAlgorithmImage (
                 Path.Algorithms[Index + 1],
                 &Name,
                 &Image,
                 &ImageSize
                 );
      if (EFI_ERROR (Status)) {
        goto Exit;
      }

      Status = LoadAndStartImage (Name, Image, ImageSize);
      if (EFI_ERROR (Status)) {
        goto Exit;
      }

      Status = LoadAndStartImage (
                 "Selected capability image after partial retirement",
                 Fixture.MultiImage,
                 Fixture.MultiImageSize
                 );
    } else {
      Status = LoadImageExpectDenied (
                 "Selected capability image after final retirement",
                 Fixture.MultiImage,
                 Fixture.MultiImageSize
                 );
    }

    if (EFI_ERROR (Status)) {
      goto Exit;
    }
  }

  for (Index = 0; Index < Path.Count; Index++) {
    RecordAlgorithmImageObservation (
      Path.Algorithms[Index],
      EcitObservationSuccess,
      EFI_SUCCESS
      );
  }

  RecordEcitObservation (
    EcitImageAuthorizationTbsV2Sha512,
    EcitObservationSuccess,
    EFI_SUCCESS
    );
  RecordEcitObservation (
    EcitImageRevocationTbsV2Sha512,
    EcitObservationSuccess,
    EFI_SUCCESS
    );

  Status = ApplyDbxUpdate (
             Fixture.DeleteDbx,
             Fixture.DeleteDbxSize,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = ApplyDbUpdate (
             Fixture.DeleteDb,
             Fixture.DeleteDbSize,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = ApplyPkUpdate (
             Fixture.DeletePk,
             Fixture.DeletePkSize,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Result = UNIT_TEST_PASSED;

Exit:
  if (ImageEvidenceAttempted && (Result != UNIT_TEST_PASSED)) {
    for (Index = 0; Index < Path.Count; Index++) {
      RecordAlgorithmImageObservation (
        Path.Algorithms[Index],
        EcitObservationFailure,
        Status
        );
    }
  }

  RecoveryStatus = RecoverSetupMode ();
  if (EFI_ERROR (RecoveryStatus)) {
    UT_LOG_ERROR ("Unable to recover the capability-selected CA state: %r\n", RecoveryStatus);
    Result = UNIT_TEST_ERROR_TEST_FAILED;
  }

  return Result;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
CumulativeCaRetirementTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8  Mask;

  Mask = GetAdvertisedImageAlgorithmMask (
           mEcitClaimResults,
           ARRAY_SIZE (mEcitClaimResults)
           );
  UT_LOG_INFO ("Advertised image-algorithm mask: 0x%x\n", Mask);

  if (Mask == 0) {
    UT_LOG_INFO ("No fixture-backed image algorithm is advertised; migration is not applicable\n");
    return UNIT_TEST_PASSED;
  }

  if (Mask == AUDIT_IMAGE_ALGORITHM_ALL) {
    return RunThreeCaRetirementTest (Context);
  }

  return RunCapabilityRetirementTest (Mask);
}

STATIC
UNIT_TEST_STATUS
EFIAPI
ImageFormatSweepTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  CONST SECURE_BOOT_FORMAT_SWEEP_CASE  *Case;
  UINTN                                Index;
  BOOLEAN                              OperationalFailure;
  EFI_STATUS                           RecoveryStatus;
  UNIT_TEST_STATUS                     Result;
  EFI_STATUS                           Status;

  Result             = UNIT_TEST_ERROR_TEST_FAILED;
  OperationalFailure = FALSE;

  if ((gSecureBootKeyAuditFormatSweepCaseCount !=
       ARRAY_SIZE (mImageAuthorizationFormatClaims)) ||
      (gSecureBootKeyAuditFormatSweepCaseCount !=
       ARRAY_SIZE (mImageRevocationFormatClaims)))
  {
    UT_LOG_ERROR ("The generated format sweep does not match the ECIT claim registry\n");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditSetupPkA,
             gSecureBootKeyAuditSetupPkASize,
             gSecureBootKeyAuditPkA,
             gSecureBootKeyAuditPkASize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not enroll the format-sweep PK: %r\n", Status);
    goto Exit;
  }

  Status = RequireSecureBootEnforcement ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  for (Index = 0; Index < gSecureBootKeyAuditFormatSweepCaseCount; Index++) {
    if (mEcitClaimResults[mImageAuthorizationFormatClaims[Index]].Claim !=
        EcitClaimAdvertised)
    {
      continue;
    }

    Case   = &gSecureBootKeyAuditFormatSweepCases[Index];
    Status = ApplyDbUpdate (
               Case->EnrollDb,
               Case->EnrollDbSize,
               Case->Esl,
               Case->EslSize
               );
    if (EFI_ERROR (Status)) {
      UT_LOG_ERROR ("%a authorization enrollment failed: %r\n", Case->Name, Status);
      RecordEcitObservation (
        mImageAuthorizationFormatClaims[Index],
        EcitObservationFailure,
        Status
        );
      goto Exit;
    }

    Status = LoadAndStartImage (
               Case->Name,
               gSecureBootKeyAuditHelloRsa,
               gSecureBootKeyAuditHelloRsaSize
               );
    if (EFI_ERROR (Status)) {
      RecordEcitObservation (
        mImageAuthorizationFormatClaims[Index],
        EcitObservationFailure,
        Status
        );
      OperationalFailure = TRUE;
    } else {
      RecordEcitObservation (
        mImageAuthorizationFormatClaims[Index],
        EcitObservationSuccess,
        EFI_SUCCESS
        );
    }

    Status = ApplyDbUpdate (
               Case->DeleteDb,
               Case->DeleteDbSize,
               NULL,
               0
               );
    if (EFI_ERROR (Status)) {
      UT_LOG_ERROR ("%a authorization cleanup failed: %r\n", Case->Name, Status);
      goto Exit;
    }
  }

  Status = ApplyDbUpdate (
             gSecureBootKeyAuditEnrollDbForRevocationSweep,
             gSecureBootKeyAuditEnrollDbForRevocationSweepSize,
             gSecureBootKeyAuditFormatSweepCases[0].Esl,
             gSecureBootKeyAuditFormatSweepCases[0].EslSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not authorize the revocation-sweep image: %r\n", Status);
    goto Exit;
  }

  for (Index = 0; Index < gSecureBootKeyAuditFormatSweepCaseCount; Index++) {
    if (mEcitClaimResults[mImageRevocationFormatClaims[Index]].Claim !=
        EcitClaimAdvertised)
    {
      continue;
    }

    Case   = &gSecureBootKeyAuditFormatSweepCases[Index];
    Status = ApplyDbxUpdate (
               Case->EnrollDbx,
               Case->EnrollDbxSize,
               Case->Esl,
               Case->EslSize
               );
    if (EFI_ERROR (Status)) {
      UT_LOG_ERROR ("%a revocation enrollment failed: %r\n", Case->Name, Status);
      RecordEcitObservation (
        mImageRevocationFormatClaims[Index],
        EcitObservationFailure,
        Status
        );
      goto Exit;
    }

    Status = LoadImageExpectDenied (
               Case->Name,
               gSecureBootKeyAuditHelloRsa,
               gSecureBootKeyAuditHelloRsaSize
               );
    if (EFI_ERROR (Status)) {
      RecordEcitObservation (
        mImageRevocationFormatClaims[Index],
        EcitObservationFailure,
        Status
        );
      OperationalFailure = TRUE;
    } else {
      RecordEcitObservation (
        mImageRevocationFormatClaims[Index],
        EcitObservationSuccess,
        EFI_SUCCESS
        );
    }

    Status = ApplyDbxUpdate (
               Case->DeleteDbx,
               Case->DeleteDbxSize,
               NULL,
               0
               );
    if (EFI_ERROR (Status)) {
      UT_LOG_ERROR ("%a revocation cleanup failed: %r\n", Case->Name, Status);
      goto Exit;
    }
  }

  Status = ApplyDbUpdate (
             gSecureBootKeyAuditDeleteDbForRevocationSweep,
             gSecureBootKeyAuditDeleteDbForRevocationSweepSize,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not clean up the revocation-sweep db: %r\n", Status);
    goto Exit;
  }

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditRecoveryDeleteByA,
             gSecureBootKeyAuditRecoveryDeleteByASize,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not clean up the format-sweep PK: %r\n", Status);
    goto Exit;
  }

  Status = RequireSetupModeBaseline ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Result = OperationalFailure ? UNIT_TEST_ERROR_TEST_FAILED : UNIT_TEST_PASSED;

Exit:
  RecoveryStatus = RecoverSetupMode ();
  if (EFI_ERROR (RecoveryStatus)) {
    UT_LOG_ERROR ("Unable to recover the format-sweep state: %r\n", RecoveryStatus);
    Result = UNIT_TEST_ERROR_TEST_FAILED;
  }

  return Result;
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
    "Continue multi-signature verification after individual signer revocation",
    "MultiSignatureRevocation",
    MultiSignatureRevocationTest,
    NULL,
    NULL,
    NULL
    );
  AddTestCase (
    Suite,
    "Retire RSA, ECDSA, and ML-DSA image CAs cumulatively",
    "CumulativeCaRetirement",
    CumulativeCaRetirementTest,
    NULL,
    NULL,
    NULL
    );
  AddTestCase (
    Suite,
    "Traverse every advertised Secure Boot update authority",
    "CapabilityAuthorityPath",
    CapabilityAuthorityPathTest,
    NULL,
    NULL,
    NULL
    );
  AddTestCase (
    Suite,
    "Exercise every advertised image authorization and revocation format",
    "ImageFormatSweep",
    ImageFormatSweepTest,
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
