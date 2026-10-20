/** @file
  Shared Secure Boot key audit state and result types.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AUDIT_TYPES_H_
#define AUDIT_TYPES_H_

#include <Uefi.h>

#define AUDIT_JOURNAL_FIXTURE_ID_SIZE  32
#define AUDIT_MANIFEST_SHA256_SIZE     32

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
  EcitCoverageUnadvertisedSuccess,
  EcitCoverageVerifiedAbsent,
  EcitCoverageNotAdvertised,
  EcitCoverageNotIndependentlyVerifiable,
  EcitCoverageNotTested,
  EcitCoverageInconclusive
} ECIT_COVERAGE_STATE;

typedef enum {
  EcitDbUpdateAuthorizationX509,
  EcitDbUpdateVerificationRsaSha256,
  EcitDbUpdateVerificationEcdsaSha256,
  EcitDbUpdateVerificationSha256,
  EcitDbUpdateVerificationMldsa87,
  EcitDbUpdateVerificationSha512,
  EcitDbUpdateVerificationRsaSha256Profile,
  EcitDbUpdateVerificationEcdsaSha256Profile,
  EcitDbUpdateVerificationMldsa87Sha512Profile,
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
  EcitImageAuthorizationTbsV2Sha512,
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
  EcitImageRevocationTbsV2Sha512,
  EcitImageVerificationRsaSha256,
  EcitImageVerificationEcdsaSha256,
  EcitImageVerificationSha256,
  EcitImageVerificationMldsa87,
  EcitImageVerificationSha512,
  EcitImageVerificationRsaSha256Profile,
  EcitImageVerificationEcdsaSha256Profile,
  EcitImageVerificationMldsa87Sha512Profile,
  EcitClaimCount
} ECIT_CLAIM_ID;

typedef struct {
  CONST CHAR8               *Name;
  ECIT_CLAIM_STATE          Claim;
  ECIT_OBSERVATION_STATE    Observation;
  EFI_STATUS                ObservationStatus;
} ECIT_CLAIM_RESULT;

typedef enum {
  AuditVariableStateUnread,
  AuditVariableStateAbsent,
  AuditVariableStatePkA,
  AuditVariableStatePkB,
  AuditVariableStatePkMldsa,
  AuditVariableStatePkEcdsa,
  AuditVariableStateDbV1,
  AuditVariableStateDbV2,
  AuditVariableStateDbMultiSigner,
  AuditVariableStateDbCaMigration,
  AuditVariableStateDbCaCapability,
  AuditVariableStateDbxRsaSigner,
  AuditVariableStateDbxMldsaSigner,
  AuditVariableStateDbxBothSigners,
  AuditVariableStateDbxMigrationRsa,
  AuditVariableStateDbxMigrationRsaEcdsa,
  AuditVariableStateDbxMigrationAll,
  AuditVariableStateDbxCaCapability,
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
  AUDIT_VARIABLE_STATE    DbxState;
  UINT32                  DbxAttributes;
  UINTN                   DbxSize;
} AUDIT_BASELINE_SNAPSHOT;

#pragma pack (1)
typedef enum {
  AuditJournalPrepared = 1,
  AuditJournalRebootPending,
  AuditJournalPersistentlyVerified,
  AuditJournalFailed
} AUDIT_JOURNAL_PHASE;

typedef struct {
  UINT32    Signature;
  UINT16    Version;
  UINT16    Size;
  UINT32    Crc32;
  UINT32    Phase;
  UINT32    BootCount;
  UINT64    PriorResult;
  CHAR8     FixtureId[AUDIT_JOURNAL_FIXTURE_ID_SIZE];
  UINT8     ManifestSha256[AUDIT_MANIFEST_SHA256_SIZE];
  UINT8     SetupMode;
  UINT8     SecureBoot;
  UINT16    Reserved;
  UINT32    PkState;
  UINT32    PkAttributes;
  UINT64    PkSize;
  UINT32    DbState;
  UINT32    DbAttributes;
  UINT64    DbSize;
  UINT32    DbxState;
  UINT32    DbxAttributes;
  UINT64    DbxSize;
} AUDIT_JOURNAL;
#pragma pack ()

#endif
