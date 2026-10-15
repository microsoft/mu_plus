/** @file
  Shared Secure Boot key audit state and result types.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AUDIT_TYPES_H_
#define AUDIT_TYPES_H_

#include <Uefi.h>

#include <Library/BaseCryptLib.h>

#define AUDIT_JOURNAL_FIXTURE_ID_SIZE  32

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
  UINT8     ManifestSha256[SHA256_DIGEST_SIZE];
  UINT8     EcitSha256[SHA256_DIGEST_SIZE];
  UINT8     SetupMode;
  UINT8     SecureBoot;
  UINT16    Reserved;
  UINT32    PkState;
  UINT32    PkAttributes;
  UINT64    PkSize;
  UINT32    DbState;
  UINT32    DbAttributes;
  UINT64    DbSize;
} AUDIT_JOURNAL;
#pragma pack ()

#endif
