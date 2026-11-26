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
#include <Library/PcdLib.h>
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
#include "EcitTable.h"
#include "SecureBootState.h"
#include "TestData.h"

#define UNIT_TEST_APP_NAME     "SecureBootCryptoAuditApp"
#define UNIT_TEST_APP_VERSION  "0.12"

#if defined (MDE_CPU_X64)
#define AUDIT_TARGET_ARCH  "X64"
#elif defined (MDE_CPU_AARCH64)
#define AUDIT_TARGET_ARCH  "AARCH64"
#elif defined (MDE_CPU_IA32)
#define AUDIT_TARGET_ARCH  "IA32"
#elif defined (MDE_CPU_ARM)
#define AUDIT_TARGET_ARCH  "ARM"
#else
#define AUDIT_TARGET_ARCH  "UNKNOWN"
#endif

#define AUDIT_FIRMWARE_VENDOR_MAX_CHARS  64
#define AUDIT_MANIFEST_SHA256_HEX_SIZE   ((AUDIT_MANIFEST_SHA256_SIZE * 2) + 1)

#define ECIT_CLAIM_CONTEXT(ClaimId)  ((UNIT_TEST_CONTEXT)(UINTN)((ClaimId) + 1))

#define AUDIT_JOURNAL_FILE_NAME   L"SecureBootCryptoAuditJournal.bin"
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

/**
  Locates and validates the ECIT, then initializes the audit claim states.

  @retval EFI_SUCCESS           The ECIT was parsed and claim states were populated.
  @retval EFI_NOT_FOUND         The ECIT configuration table was not published.
  @retval EFI_COMPROMISED_DATA  The ECIT structure or contents are invalid.
  @retval Others                An error returned while locating or processing the ECIT.
**/
STATIC
EFI_STATUS
ParseEcitClaims (
  VOID
  )
{
  EFI_CRYPTO_INDICATOR_TABLE  *Table;
  EFI_STATUS                  Status;

  Status = EfiGetSystemConfigurationTable (
             &gEfiCryptoIndicatorTableGuid,
             (VOID **)&Table
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("ECIT was not published: %r\n", Status);
    return Status;
  }

  Status = ValidateEcitTable (Table, (Table == NULL) ? 0 : Table->Length);
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("ECIT table validation failed: %r\n", Status);
    return Status;
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

/**
  Records an operational observation for one ECIT claim.

  @param[in] ClaimId                 The ECIT claim to update.
  @param[in] Observation             The observed operational result.
  @param[in] Status                  The status associated with the observation or operation.
**/
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

/**
  Records the same operational observation for multiple ECIT claims.

  @param[in] ClaimIds                The ECIT claims to update.
  @param[in] ClaimCount              The number of entries in ClaimIds.
  @param[in] Observation             The observed operational result.
  @param[in] Status                  The status associated with the observation or operation.
**/
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

/**
  Correlates an advertised ECIT claim with its observed behavior.

  @param[in] Claim                   The advertised state of the ECIT claim.
  @param[in] Observation             The observed operational result.
  @param[in] ParseStatus             The ECIT parsing status.

  @return The coverage state derived from the claim, observation, and parse status.
**/
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

/**
  Initializes ECIT claim tracking before the audit suite runs.
**/
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

/**
  Determines whether a character is valid in an audit evidence token.

  @param[in] Character               The character to classify.

  @retval TRUE  The character is valid in an evidence token.
  @retval FALSE The character must be replaced.
**/
STATIC
BOOLEAN
IsAuditEvidenceTokenCharacter (
  IN CHAR16  Character
  )
{
  return (((Character >= L'a') && (Character <= L'z')) ||
          ((Character >= L'A') && (Character <= L'Z')) ||
          ((Character >= L'0') && (Character <= L'9')) ||
          (Character == L'.') ||
          (Character == L'-') ||
          (Character == L'_'));
}

/**
  Converts a firmware vendor string into a bounded audit evidence token.

  @param[in] FirmwareVendor          The firmware vendor string.
  @param[out] Vendor                  The resulting ASCII evidence token.
  @param[in] VendorSize              The capacity of Vendor in bytes.
  @param[out] Truncated               Set to TRUE if FirmwareVendor does not fit in Vendor.

  @retval EFI_SUCCESS           Vendor contains a valid nonempty token.
  @retval EFI_INVALID_PARAMETER An input or output parameter is invalid.
  @retval EFI_COMPROMISED_DATA  FirmwareVendor produced an empty token.
**/
STATIC
EFI_STATUS
FormatFirmwareVendor (
  IN  CONST CHAR16  *FirmwareVendor,
  OUT CHAR8         *Vendor,
  IN  UINTN         VendorSize,
  OUT BOOLEAN       *Truncated
  )
{
  UINTN  Index;

  if ((FirmwareVendor == NULL) || (Vendor == NULL) ||
      (VendorSize < 2) || (Truncated == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < (VendorSize - 1); Index++) {
    if (FirmwareVendor[Index] == L'\0') {
      break;
    }

    Vendor[Index] = IsAuditEvidenceTokenCharacter (FirmwareVendor[Index]) ?
                    (CHAR8)FirmwareVendor[Index] :
                    '_';
  }

  Vendor[Index] = '\0';
  *Truncated    = (FirmwareVendor[Index] != L'\0');
  return (Index == 0) ? EFI_COMPROMISED_DATA : EFI_SUCCESS;
}

/**
  Formats a byte sequence as a lowercase hexadecimal string.

  @param[in] Bytes                   The bytes to format.
  @param[in] ByteCount               The number of bytes to format.
  @param[out] Hex                     The output buffer, sized for two characters per byte and a terminator.
**/
STATIC
VOID
FormatBytesAsHex (
  IN  CONST UINT8  *Bytes,
  IN  UINTN        ByteCount,
  OUT CHAR8        *Hex
  )
{
  STATIC CONST CHAR8  HexDigits[] = "0123456789abcdef";
  UINTN               Index;

  for (Index = 0; Index < ByteCount; Index++) {
    Hex[(Index * 2)]     = HexDigits[Bytes[Index] >> 4];
    Hex[(Index * 2) + 1] = HexDigits[Bytes[Index] & 0x0F];
  }

  Hex[ByteCount * 2] = '\0';
}

/**
  Logs the firmware and embedded fixture identity for the audit run.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
AuditRunIdentityTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  CHAR8       FixtureId[AUDIT_JOURNAL_FIXTURE_ID_SIZE + 1];
  CHAR8       FirmwareVendor[AUDIT_FIRMWARE_VENDOR_MAX_CHARS + 1];
  CHAR8       ManifestSha256[AUDIT_MANIFEST_SHA256_HEX_SIZE];
  EFI_STATUS  Status;
  BOOLEAN     VendorTruncated;

  if ((gST == NULL) || (gST->FirmwareVendor == NULL)) {
    UT_LOG_ERROR ("The UEFI system table does not provide firmware identity\n");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  if ((gSecureBootKeyAuditFixtureIdSize == 0) ||
      (gSecureBootKeyAuditFixtureIdSize > AUDIT_JOURNAL_FIXTURE_ID_SIZE) ||
      (gSecureBootKeyAuditManifestSha256Size != AUDIT_MANIFEST_SHA256_SIZE))
  {
    UT_LOG_ERROR ("The embedded fixture identity is malformed\n");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = FormatFirmwareVendor (
             gST->FirmwareVendor,
             FirmwareVendor,
             sizeof (FirmwareVendor),
             &VendorTruncated
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The firmware vendor cannot be represented in audit evidence: %r\n", Status);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  ZeroMem (FixtureId, sizeof (FixtureId));
  CopyMem (
    FixtureId,
    gSecureBootKeyAuditFixtureId,
    gSecureBootKeyAuditFixtureIdSize
    );
  FormatBytesAsHex (
    gSecureBootKeyAuditManifestSha256,
    gSecureBootKeyAuditManifestSha256Size,
    ManifestSha256
    );

  UT_LOG_INFO (
    "[AUDIT_RUN] app=%a version=%a arch=%a firmware_vendor=%a "
    "firmware_vendor_truncated=%a firmware_revision=0x%08x "
    "uefi_revision=0x%08x fixture=%a manifest_sha256=%a mode=%a\n",
    UNIT_TEST_APP_NAME,
    UNIT_TEST_APP_VERSION,
    AUDIT_TARGET_ARCH,
    FirmwareVendor,
    VendorTruncated ? "TRUE" : "FALSE",
    gST->FirmwareRevision,
    gST->Hdr.Revision,
    FixtureId,
    ManifestSha256,
    FixedPcdGetBool (PcdSecureBootCryptoAuditProbeAll) ? "ROBUSTNESS_PROBE_ALL" : "CAPABILITY_GATED"
    );

  return UNIT_TEST_PASSED;
}

/**
  Emits ECIT evidence and verifies that correlation found no mismatches.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
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

  ZeroMem (Counts, sizeof (Counts));

  for (Index = 0; Index < EcitClaimCount; Index++) {
    CoverageRows[Index] = CorrelateEcitResult (
                            mEcitClaimResults[Index].Claim,
                            mEcitClaimResults[Index].Observation,
                            mEcitParseStatus
                            );
    Counts[CoverageRows[Index]]++;
  }

  LogEcitEvidence (
    mEcitTable,
    mEcitParseStatus
    );

  if (EFI_ERROR (mEcitParseStatus) ||
      (Counts[EcitCoverageOverReported] != 0) ||
      (Counts[EcitCoverageInconclusive] != 0))
  {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

/**
  Reports and evaluates the correlation result for one ECIT claim.

  @param[in] Context                 The encoded ECIT claim identifier.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_PREREQUISITE_NOT_MET A required advertised capability is unavailable.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
EcitClaimTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  ECIT_CLAIM_ID        ClaimId;
  ECIT_COVERAGE_STATE  Coverage;
  ECIT_CLAIM_RESULT    *Result;

  ClaimId = (ECIT_CLAIM_ID)((UINTN)Context - 1);
  if (ClaimId >= EcitClaimCount) {
    UT_LOG_ERROR ("Invalid ECIT claim test context: %p\n", Context);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Result   = &mEcitClaimResults[ClaimId];
  Coverage = CorrelateEcitResult (
               Result->Claim,
               Result->Observation,
               mEcitParseStatus
               );
  UT_LOG_INFO (
    "[ECIT_CLAIM] name=%a claim=%a observation=%a validation=%a status=0x%Lx\n",
    Result->Name,
    EcitClaimStateName (Result->Claim),
    EcitObservationStateName (Result->Observation),
    EcitCoverageStateName (Coverage),
    (UINT64)Result->ObservationStatus
    );

  switch (Coverage) {
    case EcitCoverageVerified:
    case EcitCoverageVerifiedAbsent:
      return UNIT_TEST_PASSED;
    case EcitCoverageNotTested:
    case EcitCoverageNotAdvertised:
    case EcitCoverageNotIndependentlyVerifiable:
      return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
    case EcitCoverageUnadvertisedSuccess:
      if (FixedPcdGetBool (
            PcdSecureBootCryptoAuditAllowUnadvertisedSuccess
            ))
      {
        UT_LOG_WARNING (
          "Accepted an unadvertised successful operation by policy\n"
          );
        return UNIT_TEST_PASSED;
      }

      UT_LOG_ERROR (
        "Successful unadvertised operations require an explicit policy opt-in\n"
        );
      return UNIT_TEST_ERROR_TEST_FAILED;
    case EcitCoverageOverReported:
    case EcitCoverageInconclusive:
    default:
      return UNIT_TEST_ERROR_TEST_FAILED;
  }
}

/**
  Verifies advertised profile coverage for an ECIT feature.

  @param[in] Context                 The ECIT feature GUID to evaluate.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_PREREQUISITE_NOT_MET A required advertised capability is unavailable.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
EcitProfileCoverageTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  ECIT_COVERAGE_STATE  CoverageRows[EcitClaimCount];
  ECIT_PROFILE_COUNTS  Counts;
  CONST EFI_GUID       *Feature;
  UINTN                Index;
  EFI_STATUS           Status;

  Feature = (CONST EFI_GUID *)Context;
  for (Index = 0; Index < EcitClaimCount; Index++) {
    CoverageRows[Index] = CorrelateEcitResult (
                            mEcitClaimResults[Index].Claim,
                            mEcitClaimResults[Index].Observation,
                            mEcitParseStatus
                            );
  }

  Status = GetEcitProfileCounts (
             mEcitTable,
             Feature,
             CoverageRows,
             &Counts
             );
  if (Status == EFI_NOT_FOUND) {
    UT_LOG_INFO ("ECIT feature %g is not advertised\n", Feature);
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Could not derive ECIT profile coverage: %r\n", Status);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  UT_LOG_INFO (
    "[ECIT_PROFILE_COVERAGE] feature=%g advertised=%u tested=%u "
    "not_tested=%u mismatches=%u\n",
    Feature,
    Counts.Advertised,
    Counts.Tested,
    Counts.NotTested,
    Counts.Mismatches
    );
  if (Counts.Mismatches != 0) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  if (Counts.NotTested != 0) {
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  return UNIT_TEST_PASSED;
}

/**
  Verifies that the ECIT claims were parsed successfully.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
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

/**
  Checks whether ECIT parsing succeeded before an ECIT-dependent test.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_PREREQUISITE_NOT_MET A required advertised capability is unavailable.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
RequireParsedEcit (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  if (EFI_ERROR (mEcitParseStatus)) {
    UT_LOG_INFO ("Skipping ECIT-dependent test because ECIT is unavailable: %r\n", mEcitParseStatus);
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  return UNIT_TEST_PASSED;
}

/**
  Requires a complete advertised authenticated-update profile before enrollment.

  @param[in] RequiredMask  Authority algorithms needed by the scenario and cleanup.

  @retval UNIT_TEST_PASSED                     All required profiles are advertised.
  @retval UNIT_TEST_ERROR_PREREQUISITE_NOT_MET A required profile is unavailable.
**/
STATIC
UNIT_TEST_STATUS
RequireUpdateAuthorities (
  IN UINT8  RequiredMask
  )
{
  UINT8  Mask;

  if (FixedPcdGetBool (PcdSecureBootCryptoAuditProbeAll) &&
      (!EFI_ERROR (mEcitParseStatus) || (mEcitParseStatus == EFI_NOT_FOUND)))
  {
    UT_LOG_WARNING ("ROBUSTNESS: probing update profiles 0x%x without capability gating; cleanup may fail\n", RequiredMask);
    return UNIT_TEST_PASSED;
  }

  if (RequireParsedEcit (NULL) != UNIT_TEST_PASSED) {
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  Mask = GetAdvertisedUpdateAlgorithmMask (
           mEcitClaimResults,
           ARRAY_SIZE (mEcitClaimResults)
           );
  if ((Mask & RequiredMask) != RequiredMask) {
    UT_LOG_INFO (
      "Skipping enrollment: required update profiles 0x%x, advertised 0x%x\n",
      RequiredMask,
      Mask
      );
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
RequireOperationalEcit (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  if (FixedPcdGetBool (PcdSecureBootCryptoAuditProbeAll) && (mEcitParseStatus == EFI_NOT_FOUND)) {
    UT_LOG_WARNING ("ROBUSTNESS: running without ECIT; no conformance conclusion is possible\n");
    return UNIT_TEST_PASSED;
  }

  return RequireParsedEcit (Context);
}

/**
  Requires the ML-DSA authenticated-update profile used by enrollment and cleanup.

  @param[in] Context  The unit-test context supplied by the framework.

  @return The authenticated-update prerequisite result.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
RequireMldsaUpdate (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RequireUpdateAuthorities (AUDIT_IMAGE_ALGORITHM_ML_DSA);
}

/**
  Requires both authenticated-update profiles used by RSA/ML-DSA rotation.

  @param[in] Context  The unit-test context supplied by the framework.

  @return The authenticated-update prerequisite result.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
RequireRsaMldsaUpdates (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RequireUpdateAuthorities (
           AUDIT_IMAGE_ALGORITHM_RSA | AUDIT_IMAGE_ALGORITHM_ML_DSA
           );
}

/**
  Tests prerequisite gates without changing firmware variables.

  @param[in] Context  The unit-test context supplied by the framework.

  @return The prerequisite-gate test result.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
PrerequisiteGateTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  STATIC CONST struct {
    EFI_STATUS    ParseStatus;
    BOOLEAN       Advertised;
    BOOLEAN       UpdateAllowed[2];
    BOOLEAN       OperationalAllowed[2];
  } Cases[] = {
    { EFI_SUCCESS,          TRUE,  { TRUE,  TRUE  }, { TRUE,  TRUE  }
    },
    { EFI_SUCCESS,          FALSE, { FALSE, TRUE  }, { TRUE,  TRUE  }
    },
    { EFI_NOT_FOUND,        FALSE, { FALSE, TRUE  }, { FALSE, TRUE  }
    },
    { EFI_COMPROMISED_DATA, TRUE,  { FALSE, FALSE }, { FALSE, FALSE }
    },
    { EFI_NOT_READY,        TRUE,  { FALSE, FALSE }, { FALSE, FALSE }
    },
    { EFI_DEVICE_ERROR,     TRUE,  { FALSE, FALSE }, { FALSE, FALSE }
    }
  };
  ECIT_CLAIM_RESULT  SavedClaims[EcitClaimCount];
  EFI_STATUS         SavedParseStatus;
  UNIT_TEST_STATUS   UpdateResults[ARRAY_SIZE (Cases)];
  UNIT_TEST_STATUS   OperationalResults[ARRAY_SIZE (Cases)];
  UINTN              CaseIndex;
  UINTN              ClaimIndex;
  UINTN              Mode;

  Mode             = FixedPcdGetBool (PcdSecureBootCryptoAuditProbeAll) ? 1 : 0;
  SavedParseStatus = mEcitParseStatus;
  CopyMem (SavedClaims, mEcitClaimResults, sizeof (SavedClaims));
  for (CaseIndex = 0; CaseIndex < ARRAY_SIZE (Cases); CaseIndex++) {
    mEcitParseStatus = Cases[CaseIndex].ParseStatus;
    for (ClaimIndex = 0; ClaimIndex < EcitClaimCount; ClaimIndex++) {
      mEcitClaimResults[ClaimIndex].Claim = Cases[CaseIndex].Advertised ?
                                            EcitClaimAdvertised : EcitClaimAbsent;
    }

    UpdateResults[CaseIndex]      = RequireUpdateAuthorities (AUDIT_IMAGE_ALGORITHM_ALL);
    OperationalResults[CaseIndex] = RequireOperationalEcit (NULL);
  }

  mEcitParseStatus = SavedParseStatus;
  CopyMem (mEcitClaimResults, SavedClaims, sizeof (SavedClaims));
  for (CaseIndex = 0; CaseIndex < ARRAY_SIZE (Cases); CaseIndex++) {
    UT_LOG_INFO (
      "Checking prerequisite gates: probe_all=%u parse_status=%r advertised=%u\n",
      Mode,
      Cases[CaseIndex].ParseStatus,
      Cases[CaseIndex].Advertised
      );
    UT_ASSERT_EQUAL (
      UpdateResults[CaseIndex],
      Cases[CaseIndex].UpdateAllowed[Mode] ? UNIT_TEST_PASSED : UNIT_TEST_ERROR_PREREQUISITE_NOT_MET
      );
    UT_ASSERT_EQUAL (
      OperationalResults[CaseIndex],
      Cases[CaseIndex].OperationalAllowed[Mode] ? UNIT_TEST_PASSED : UNIT_TEST_ERROR_PREREQUISITE_NOT_MET
      );
  }

  return UNIT_TEST_PASSED;
}

/**
  Validates ECIT correlation and capability-path classification helpers.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
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
  STATIC CONST struct {
    CONST ECIT_CLAIM_ID    *Claims;
    UINTN                  ClaimCount;
    UINT8                  Algorithm;
  } UpdateCases[] = {
    { mRsaUpdateClaims,   ARRAY_SIZE (mRsaUpdateClaims),   AUDIT_IMAGE_ALGORITHM_RSA    },
    { mEcdsaUpdateClaims, ARRAY_SIZE (mEcdsaUpdateClaims), AUDIT_IMAGE_ALGORITHM_ECDSA  },
    { mMldsaUpdateClaims, ARRAY_SIZE (mMldsaUpdateClaims), AUDIT_IMAGE_ALGORITHM_ML_DSA }
  };
  AUDIT_CAPABILITY_PATH  Path;
  ECIT_CLAIM_RESULT      Claims[EcitClaimCount];
  UINTN                  AlgorithmIndex;
  UINTN                  CaseIndex;
  UINTN                  ClaimIndex;
  UINTN                  MissingIndex;

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

  for (CaseIndex = 0; CaseIndex < ARRAY_SIZE (UpdateCases); CaseIndex++) {
    InitializeEcitClaimResults (Claims, ARRAY_SIZE (Claims));
    UT_ASSERT_EQUAL (GetAdvertisedUpdateAlgorithmMask (Claims, ARRAY_SIZE (Claims)), 0);
    for (ClaimIndex = 0; ClaimIndex < UpdateCases[CaseIndex].ClaimCount; ClaimIndex++) {
      Claims[UpdateCases[CaseIndex].Claims[ClaimIndex]].Claim = EcitClaimAdvertised;
    }

    UT_ASSERT_EQUAL (
      GetAdvertisedUpdateAlgorithmMask (Claims, ARRAY_SIZE (Claims)),
      UpdateCases[CaseIndex].Algorithm
      );
    for (MissingIndex = 0; MissingIndex < UpdateCases[CaseIndex].ClaimCount; MissingIndex++) {
      Claims[UpdateCases[CaseIndex].Claims[MissingIndex]].Claim = EcitClaimAbsent;
      UT_ASSERT_EQUAL (GetAdvertisedUpdateAlgorithmMask (Claims, ARRAY_SIZE (Claims)), 0);
      Claims[UpdateCases[CaseIndex].Claims[MissingIndex]].Claim = EcitClaimUnknown;
      UT_ASSERT_EQUAL (GetAdvertisedUpdateAlgorithmMask (Claims, ARRAY_SIZE (Claims)), 0);
      Claims[UpdateCases[CaseIndex].Claims[MissingIndex]].Claim = EcitClaimAdvertised;
    }
  }

  UT_ASSERT_EQUAL (GetAdvertisedUpdateAlgorithmMask (NULL, EcitClaimCount), 0);
  UT_ASSERT_EQUAL (GetAdvertisedUpdateAlgorithmMask (Claims, EcitClaimCount - 1), 0);

  return UNIT_TEST_PASSED;
}

/**
  Validates classification of recognized Secure Boot recovery states.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
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
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbV2,           SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbxMldsaSigner,       SECURE_BOOT_KEY_ATTRIBUTES, 1 }, TRUE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbV2,           SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbxMldsaSigner,       0,                          1 }, FALSE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbV2,           0,                          1, AuditVariableStateDbxMldsaSigner,       SECURE_BOOT_KEY_ATTRIBUTES, 1 }, FALSE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, 0,                          1, AuditVariableStateDbV2,           SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbxMldsaSigner,       SECURE_BOOT_KEY_ATTRIBUTES, 1 }, FALSE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbV1,           SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbxMldsaSigner,       SECURE_BOOT_KEY_ATTRIBUTES, 1 }, FALSE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbV2,           SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbxRsaSigner,         SECURE_BOOT_KEY_ATTRIBUTES, 1 }, FALSE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbV2,           SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbxBothSigners,       SECURE_BOOT_KEY_ATTRIBUTES, 1 }, FALSE
    },
    {
      { USER_MODE,  SECURE_BOOT_MODE_ENABLE,  AuditVariableStatePkMldsa, SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateDbV2,           SECURE_BOOT_KEY_ATTRIBUTES, 1, AuditVariableStateUnknown,              SECURE_BOOT_KEY_ATTRIBUTES, 1 }, FALSE
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

/**
  Returns the evidence name for an audit journal phase.

  @param[in] Phase                   The audit phase name or journal phase to process.

  @return A static phase name string, or "UNKNOWN" for an invalid phase.
**/
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

/**
  Recalculates and stores the audit journal CRC32.

  @param[in, out] Journal                 The audit journal to process.

  @retval EFI_SUCCESS The CRC was calculated and stored.
  @retval Others      The boot service could not calculate the CRC.
**/
STATIC
EFI_STATUS
CalculateAuditJournalCrc (
  IN OUT AUDIT_JOURNAL  *Journal
  )
{
  Journal->Crc32 = 0;
  return gBS->CalculateCrc32 (Journal, sizeof (*Journal), &Journal->Crc32);
}

/**
  Validates the structure, fixture identity, and CRC of an audit journal.

  @param[in] Journal                 The audit journal to process.

  @retval EFI_SUCCESS              The journal is valid.
  @retval EFI_COMPROMISED_DATA     The journal structure or CRC is invalid.
  @retval EFI_INCOMPATIBLE_VERSION The journal belongs to a different fixture.
  @retval Others                   The journal CRC could not be calculated.
**/
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

/**
  Reads and validates the persistent audit journal.

  @param[out] Journal                 The audit journal to process.

  @retval EFI_SUCCESS         The journal was read and validated.
  @retval EFI_NOT_FOUND       The journal file does not exist.
  @retval EFI_BAD_BUFFER_SIZE The journal file has an unexpected size.
  @retval Others              A file or validation operation failed.
**/
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

/**
  Updates the CRC and writes the persistent audit journal.

  @param[in, out] Journal                 The audit journal to process.

  @retval EFI_SUCCESS The journal was written and flushed.
  @retval Others      The CRC, file, write, or flush operation failed.
**/
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

/**
  Initializes a new audit journal from the current baseline snapshot.

  @param[out] Journal                 The audit journal to process.
  @param[in] Snapshot                The baseline snapshot to compare with or copy.
**/
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

/**
  Compares journaled Secure Boot state with a fresh baseline snapshot.

  @param[in] Journal                 The audit journal to process.
  @param[in] Snapshot                The baseline snapshot to compare with or copy.

  @retval EFI_SUCCESS          The snapshot matches the journaled state.
  @retval EFI_COMPROMISED_DATA The state differs from the journal.
**/
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

/**
  Creates the volatile marker used to prove a cold reboot occurred.

  @retval EFI_SUCCESS The volatile reboot marker was created.
  @retval Others      The marker variable could not be written.
**/
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

/**
  Checks whether the volatile reboot marker was cleared by a reset.

  @retval EFI_NOT_FOUND The reboot marker was cleared by reset.
  @retval Others        The marker remains present or could not be queried.
**/
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

/**
  Deletes the volatile audit reboot marker.
**/
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

/**
  Verifies audit identity and Secure Boot state across a cold reboot.

  @param[in] Context                 The restored audit journal context after reboot, or NULL on the first boot.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
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

/**
  Verifies that interrupted state is preserved until explicit recovery.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
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

/**
  Logs the current SetupMode and SecureBoot variable values.

  @param[in] Phase                   The phase label used in the log message.
**/
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

/**
  Verifies that the platform is enforcing Secure Boot in User Mode.

  @retval EFI_SUCCESS            Secure Boot is enforcing in User Mode.
  @retval EFI_SECURITY_VIOLATION Secure Boot is not enforcing.
  @retval Others                 A mode variable could not be read.
**/
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

/**
  Loads an image and verifies that Secure Boot denies it.

  @param[in] Phase                   The operation label used in audit messages.
  @param[in] Image                   The image buffer.
  @param[in] ImageSize               Receives the size of the selected image in bytes.

  @retval EFI_SUCCESS            The image was denied and any handle was unloaded.
  @retval EFI_SECURITY_VIOLATION The image was not denied as expected.
  @retval Others                 The denied image could not be unloaded.
**/
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

/**
  Loads and starts an image expected to be authorized by Secure Boot.

  @param[in] Phase                   The operation label used in audit messages.
  @param[in] Image                   The image buffer.
  @param[in] ImageSize               Receives the size of the selected image in bytes.

  @retval EFI_SUCCESS The image loaded and started successfully.
  @retval Others      Loading or starting the image failed.
**/
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

/**
  Creates a modified copy of the embedded ML-DSA test image.

  @param[out] TamperedImage           Receives the allocated tampered image buffer.

  @retval EFI_SUCCESS          The tampered image was allocated and returned.
  @retval EFI_LOAD_ERROR       The embedded image could not be parsed.
  @retval EFI_COMPROMISED_DATA The embedded image layout is invalid.
  @retval EFI_OUT_OF_RESOURCES The image copy could not be allocated.
**/
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

/**
  Verifies that replaying an authenticated PK update is rejected atomically.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
AuthenticatedVariableReplayTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
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
             gSecureBootKeyAuditSetupPkA,
             gSecureBootKeyAuditSetupPkASize,
             gSecureBootKeyAuditPkA,
             gSecureBootKeyAuditPkASize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Unable to establish the RSA replay-test authority: %r\n", Status);
    goto Exit;
  }

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditPkAToB,
             gSecureBootKeyAuditPkAToBSize,
             gSecureBootKeyAuditPkB,
             gSecureBootKeyAuditPkBSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Unable to apply the original RSA PK update: %r\n", Status);
    goto Exit;
  }

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditPkBToA,
             gSecureBootKeyAuditPkBToASize,
             gSecureBootKeyAuditPkA,
             gSecureBootKeyAuditPkASize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Unable to restore PK A before the replay probe: %r\n", Status);
    goto Exit;
  }

  ZeroMem (&ReplayScenario, sizeof (ReplayScenario));
  ReplayScenario.ScenarioId               = "rsa-pk-update-replay-rejected";
  ReplayScenario.VariableName             = EFI_PLATFORM_KEY_NAME;
  ReplayScenario.VendorGuid               = &gEfiGlobalVariableGuid;
  ReplayScenario.Attributes               = SECURE_BOOT_KEY_ATTRIBUTES;
  ReplayScenario.AuthenticatedUpdate      = gSecureBootKeyAuditPkAToB;
  ReplayScenario.AuthenticatedUpdateSize  = gSecureBootKeyAuditPkAToBSize;
  ReplayScenario.ExpectedStatus           = EFI_SECURITY_VIOLATION;
  ReplayScenario.ExpectedBefore           = gSecureBootKeyAuditPkA;
  ReplayScenario.ExpectedBeforeSize       = gSecureBootKeyAuditPkASize;
  ReplayScenario.ExpectedBeforeAttributes = SECURE_BOOT_KEY_ATTRIBUTES;
  ReplayScenario.ExpectedAfter            = gSecureBootKeyAuditPkA;
  ReplayScenario.ExpectedAfterSize        = gSecureBootKeyAuditPkASize;
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

/**
  Exercises RSA-authenticated PK rotation and restores Setup Mode.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
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

/**
  Exercises direct ML-DSA PK enrollment and authenticated cleanup.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
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

/**
  Exercises PK rotation between RSA and ML-DSA authorities.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
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

/**
  Exercises ML-DSA image authorization with a selected TBS hash format.

  @param[in] Version                 The TBS hash format version label.
  @param[in] ImageAuthorizationClaim The ECIT image-authorization claim being tested.
  @param[in] EnrollDb                The authenticated update that enrolls the test db entry.
  @param[in] EnrollDbSize            The size of EnrollDb in bytes.
  @param[in] ExpectedDb              The expected db contents after enrollment.
  @param[in] ExpectedDbSize          The size of ExpectedDb in bytes.
  @param[in] DeleteDb                The authenticated update that deletes the test db entry.
  @param[in] DeleteDbSize            The size of DeleteDb in bytes.

  @retval UNIT_TEST_PASSED           The image authorization scenario succeeded.
  @retval UNIT_TEST_ERROR_TEST_FAILED The scenario or cleanup failed.
**/
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

/**
  Exercises ML-DSA image authorization with a version 1 TBS hash.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
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

/**
  Exercises ML-DSA image authorization with a version 2 TBS hash.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
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

/**
  Verifies multi-signature image behavior as individual signers are revoked.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
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

/**
  Exercises cumulative retirement of RSA, ECDSA, and ML-DSA image CAs.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
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

/**
  Returns the display name for an audit image algorithm.

  @param[in] Algorithm               The audit image algorithm identifier.

  @return A static algorithm name string, or "UNKNOWN" for an invalid identifier.
**/
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

/**
  Returns the embedded test image for an audit image algorithm.

  @param[in] Algorithm               The audit image algorithm identifier.
  @param[out] Name                    Receives the algorithm display name.
  @param[out] Image                   Receives the embedded image buffer.
  @param[out] ImageSize               Receives the size of the selected image in bytes.

  @retval EFI_SUCCESS           The image information was returned.
  @retval EFI_INVALID_PARAMETER Algorithm is not supported.
**/
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

/**
  Records image-verification observations for an algorithm.

  @param[in] Algorithm               The audit image algorithm identifier.
  @param[in] Observation             The observed operational result.
  @param[in] Status                  The status associated with the observation or operation.
**/
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

/**
  Records database-update observations for an authority algorithm.

  @param[in] Algorithm               The audit image algorithm identifier.
  @param[in] Observation             The observed operational result.
  @param[in] Status                  The status associated with the observation or operation.
**/
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

/**
  Traverses every advertised Secure Boot update-authority algorithm.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
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
  if ((Mask == 0) && !FixedPcdGetBool (PcdSecureBootCryptoAuditProbeAll)) {
    UT_LOG_INFO ("No fixture-backed update algorithm is advertised; authority path is not applicable\n");
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  Status = BuildImageCapabilityPath (Mask, FixedPcdGetBool (PcdSecureBootCryptoAuditProbeAll), &Path);
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

/**
  Selects test data for an advertised image-algorithm mask.

  @param[in] Mask                    The advertised image-algorithm mask.
  @param[out] Fixture                 Receives the selected capability-retirement fixture.

  @retval EFI_SUCCESS     A fixture was selected.
  @retval EFI_NOT_FOUND   Mask contains no supported authority algorithm.
  @retval EFI_UNSUPPORTED No fixture covers Mask.
**/
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

/**
  Exercises CA retirement for a selected capability mask.

  @param[in] Mask                    The advertised image-algorithm mask.

  @retval UNIT_TEST_PASSED           The retirement scenario succeeded.
  @retval UNIT_TEST_ERROR_TEST_FAILED The scenario or cleanup failed.
**/
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

  Result = RequireUpdateAuthorities (Fixture.AuthorityAlgorithm);
  if (Result != UNIT_TEST_PASSED) {
    return Result;
  }

  Result = UNIT_TEST_ERROR_TEST_FAILED;
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

/**
  Exercises CA retirement for the advertised image capabilities.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_PREREQUISITE_NOT_MET A required advertised capability is unavailable.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
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

  if (FixedPcdGetBool (PcdSecureBootCryptoAuditProbeAll)) {
    Mask = AUDIT_IMAGE_ALGORITHM_ALL;
  }

  if (Mask == 0) {
    UT_LOG_INFO ("No fixture-backed image algorithm is advertised; migration is not applicable\n");
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  if (Mask == AUDIT_IMAGE_ALGORITHM_ALL) {
    if (RequireUpdateAuthorities (AUDIT_IMAGE_ALGORITHM_RSA) != UNIT_TEST_PASSED) {
      return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
    }

    return RunThreeCaRetirementTest (Context);
  }

  return RunCapabilityRetirementTest (Mask);
}

/**
  Exercises every advertised image authorization and revocation format.

  @param[in] Context                 The unit-test context supplied by the framework.

  @retval UNIT_TEST_PASSED                     The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED           The test detected an error or mismatch.
**/
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
    if (!FixedPcdGetBool (PcdSecureBootCryptoAuditProbeAll) &&
        (mEcitClaimResults[mImageAuthorizationFormatClaims[Index]].Claim != EcitClaimAdvertised))
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
    if (!FixedPcdGetBool (PcdSecureBootCryptoAuditProbeAll) &&
        (mEcitClaimResults[mImageRevocationFormatClaims[Index]].Claim != EcitClaimAdvertised))
    {
      continue;
    }

    Case   = &gSecureBootKeyAuditFormatSweepCases[Index];
    Status = RequireSecureBootEnforcement ();
    if (!EFI_ERROR (Status)) {
      Status = VerifyDb (
                 gSecureBootKeyAuditFormatSweepCases[0].Esl,
                 gSecureBootKeyAuditFormatSweepCases[0].EslSize
                 );
    }

    if (!EFI_ERROR (Status)) {
      Status = VerifyDbx (NULL, 0);
    }

    if (!EFI_ERROR (Status)) {
      Status = LoadAndStartImage (
                 "Revocation-sweep positive control with dbx empty",
                 gSecureBootKeyAuditHelloRsa,
                 gSecureBootKeyAuditHelloRsaSize
                 );
    }

    if (EFI_ERROR (Status)) {
      UT_LOG_ERROR ("%a revocation positive control failed: %r\n", Case->Name, Status);
      RecordEcitObservation (
        mImageRevocationFormatClaims[Index],
        EcitObservationInconclusive,
        Status
        );
      goto Exit;
    }

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

typedef struct {
  CHAR8                 *Description;
  CHAR8                 *ClassName;
  UNIT_TEST_FUNCTION    Function;
  UNIT_TEST_FUNCTION    Prerequisite;
} AUDIT_TEST_CASE;

STATIC CONST AUDIT_TEST_CASE  mAuditTestCases[] = {
  {
    "Report audit run identity",
    "AuditRunIdentity",
    AuditRunIdentityTest,
    NULL
  },
  {
    "Parse ECIT capability claims",
    "EcitClaimsParse",
    EcitClaimsParseTest,
    NULL
  },
  {
    "Check capability and robustness prerequisite gates without mutation",
    "PrerequisiteGate",
    PrerequisiteGateTest,
    NULL
  },
  {
    "Classify ECIT claim and observation results",
    "EcitCorrelationClassifier",
    EcitCorrelationClassifierTest,
    NULL
  },
  {
    "Reject unsafe Secure Boot recovery states before mutation",
    "RecoveryStateClassifier",
    RecoveryStateClassifierTest,
    NULL
  },
  {
    "Persist audit identity and state across a cold reboot",
    "PersistentJournalReboot",
    PersistentJournalRebootTest,
    RequireParsedEcit
  },
  {
    "Preserve evidence before recovering interrupted test state",
    "InterruptedStateRecovery",
    InterruptedStateRecoveryTest,
    NULL
  },
  {
    "Rotate RSA-2048 PK A to B and back",
    "RsaPkRotation",
    RsaPkRotationTest,
    NULL
  },
  {
    "Enroll ML-DSA-87 PK directly from Setup Mode",
    "MldsaSetupEnrollment",
    MldsaSetupEnrollmentTest,
    RequireMldsaUpdate
  },
  {
    "Reject replayed RSA-authenticated PK updates atomically",
    "AuthenticatedVariableReplay",
    AuthenticatedVariableReplayTest,
    NULL
  },
  {
    "Rotate RSA-2048 PK to ML-DSA-87 and back",
    "RsaToMldsaRotation",
    RsaToMldsaRotationTest,
    RequireRsaMldsaUpdates
  },
  {
    "Authorize an ML-DSA-87 image with a V1 db TBS hash",
    "MldsaImageTbsHashV1",
    MldsaImageTbsHashV1Test,
    RequireMldsaUpdate
  },
  {
    "Authorize an ML-DSA-87 image with a V2 db TBS hash",
    "MldsaImageTbsHashV2",
    MldsaImageTbsHashV2Test,
    RequireMldsaUpdate
  },
  {
    "Continue multi-signature verification after individual signer revocation",
    "MultiSignatureRevocation",
    MultiSignatureRevocationTest,
    RequireMldsaUpdate
  },
  {
    "Retire RSA, ECDSA, and ML-DSA image CAs cumulatively",
    "CumulativeCaRetirement",
    CumulativeCaRetirementTest,
    RequireOperationalEcit
  },
  {
    "Traverse every advertised Secure Boot update authority",
    "CapabilityAuthorityPath",
    CapabilityAuthorityPathTest,
    RequireOperationalEcit
  },
  {
    "Exercise every advertised image authorization and revocation format",
    "ImageFormatSweep",
    ImageFormatSweepTest,
    RequireOperationalEcit
  },
  {
    "Report ECIT claim and observation correlation",
    "EcitCorrelationReport",
    EcitCorrelationReportTest,
    RequireParsedEcit
  }
};

/**
  Initializes and runs the Secure Boot key audit unit-test application.

  @param[in] ImageHandle             The firmware-allocated image handle.
  @param[in] SystemTable             The EFI system table.

  @retval EFI_SUCCESS The audit test suites completed successfully.
  @retval Others      Framework initialization, test registration, or execution failed.
**/
EFI_STATUS
EFIAPI
SecureBootCryptoAuditAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      Suite;
  CHAR8                       ClaimDescription[128];
  CONST CHAR8                 *ClaimName;
  EFI_STATUS                  Status;
  UINTN                       Index;

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
             "Secure Boot crypto audit",
             "SecureBootKeyAudit",
             EcitSuiteSetup,
             NULL
             );
  if (EFI_ERROR (Status)) {
    FreeUnitTestFramework (Framework);
    return Status;
  }

  for (Index = 0; Index < ARRAY_SIZE (mAuditTestCases); Index++) {
    Status = AddTestCase (
               Suite,
               mAuditTestCases[Index].Description,
               mAuditTestCases[Index].ClassName,
               mAuditTestCases[Index].Function,
               mAuditTestCases[Index].Prerequisite,
               NULL,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "Unable to register %a: %r\n",
        mAuditTestCases[Index].ClassName,
        Status
        ));
      FreeUnitTestFramework (Framework);
      return Status;
    }
  }

  for (Index = 0; Index < EcitClaimCount; Index++) {
    ClaimName = GetEcitClaimName ((ECIT_CLAIM_ID)Index);
    if (ClaimName == NULL) {
      FreeUnitTestFramework (Framework);
      return EFI_COMPROMISED_DATA;
    }

    Status = AsciiStrCpyS (
               ClaimDescription,
               sizeof (ClaimDescription),
               ClaimName
               );
    if (EFI_ERROR (Status)) {
      FreeUnitTestFramework (Framework);
      return Status;
    }

    Status = AddTestCase (
               Suite,
               ClaimDescription,
               "EcitClaim",
               EcitClaimTest,
               RequireParsedEcit,
               NULL,
               ECIT_CLAIM_CONTEXT (Index)
               );
    if (EFI_ERROR (Status)) {
      FreeUnitTestFramework (Framework);
      return Status;
    }
  }

  Status = AddTestCase (
             Suite,
             "Image verification advertised profile coverage",
             "EcitProfileCoverage",
             EcitProfileCoverageTest,
             RequireParsedEcit,
             NULL,
             &gEfiEcitFeatureSbImageVerificationGuid
             );
  if (EFI_ERROR (Status)) {
    FreeUnitTestFramework (Framework);
    return Status;
  }

  Status = AddTestCase (
             Suite,
             "Database update verification advertised profile coverage",
             "EcitProfileCoverage",
             EcitProfileCoverageTest,
             RequireParsedEcit,
             NULL,
             &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid
             );
  if (EFI_ERROR (Status)) {
    FreeUnitTestFramework (Framework);
    return Status;
  }

  Status = RunAllTestSuites (Framework);
  FreeUnitTestFramework (Framework);
  return Status;
}
