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
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UnitTestLib.h>

#include "TestData.h"

#define UNIT_TEST_APP_NAME     "SecureBootKeyAuditApp"
#define UNIT_TEST_APP_VERSION  "0.2"

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
ValidateEcitClaims (
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

  if ((Authorization.Data == NULL) ||
      (Verification.Data == NULL) ||
      (ImageAuthorization.Data == NULL) ||
      (ImageVerification.Data == NULL))
  {
    UT_LOG_ERROR ("ECIT is missing a required Secure Boot entry\n");
    return EFI_NOT_FOUND;
  }

  if (!GuidPayloadContains (&Authorization, &gEfiCertX509Guid)) {
    UT_LOG_ERROR ("ECIT does not advertise X.509 database-update authorization\n");
    return EFI_UNSUPPORTED;
  }

  if (!CsvContainsToken (Verification.Data, Verification.Size, SHA256_WITH_RSA_OID) ||
      !CsvContainsToken (Verification.Data, Verification.Size, SHA256_OID) ||
      !CsvContainsToken (Verification.Data, Verification.Size, ML_DSA_87_OID) ||
      !CsvContainsToken (Verification.Data, Verification.Size, SHA512_OID))
  {
    UT_LOG_ERROR ("ECIT does not advertise the RSA/SHA-256 and ML-DSA-87/SHA-512 update profiles\n");
    return EFI_UNSUPPORTED;
  }

  if (!GuidPayloadContains (&ImageAuthorization, &gEfiCertX509Sha512Guid) ||
      !GuidPayloadContains (&ImageAuthorization, &gEfiCertV2X509Sha512Guid))
  {
    UT_LOG_ERROR ("ECIT does not advertise V1 and V2 SHA-512 TBS-hash image authorization\n");
    return EFI_UNSUPPORTED;
  }

  if (!CsvContainsToken (
         ImageVerification.Data,
         ImageVerification.Size,
         ML_DSA_87_OID
         ) ||
      !CsvContainsToken (
         ImageVerification.Data,
         ImageVerification.Size,
         SHA512_OID
         ))
  {
    UT_LOG_ERROR ("ECIT does not advertise ML-DSA-87/SHA-512 image verification\n");
    return EFI_UNSUPPORTED;
  }

  UT_LOG_INFO ("ECIT advertises the required update and image capabilities\n");
  return EFI_SUCCESS;
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
RecoverSetupMode (
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
RequireSetupModeBaseline (
  VOID
  )
{
  UINT8       SetupMode;
  EFI_STATUS  Status;

  Status = RecoverSetupMode ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ReadModeVariable (EFI_SETUP_MODE_NAME, &SetupMode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SetupMode != SETUP_MODE) {
    UT_LOG_ERROR ("SetupMode is %u; expected a disposable Setup Mode baseline\n", SetupMode);
    return EFI_ACCESS_DENIED;
  }

  return VerifyPk (NULL, 0);
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

  Status = ValidateEcitClaims ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

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
    goto Exit;
  }

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

  Status = ValidateEcitClaims ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

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
EFIAPI
RsaToMldsaRotationTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS        RecoveryStatus;
  UNIT_TEST_STATUS  Result;
  EFI_STATUS        Status;

  Result = UNIT_TEST_ERROR_TEST_FAILED;

  Status = ValidateEcitClaims ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

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
    goto Exit;
  }

  LogSecureBootMode ("RSA PK rolled to ML-DSA-87");

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditPkMldsaToA,
             gSecureBootKeyAuditPkMldsaToASize,
             gSecureBootKeyAuditPkA,
             gSecureBootKeyAuditPkASize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The ML-DSA-87 PK could not authorize RSA PK A: %r\n", Status);
    goto Exit;
  }

  Status = ApplyPkUpdate (
             gSecureBootKeyAuditDeleteByAAfterMldsa,
             gSecureBootKeyAuditDeleteByAAfterMldsaSize,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("RSA PK A could not authenticate final cleanup: %r\n", Status);
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
  IN CONST CHAR8  *Version,
  IN CONST UINT8  *EnrollDb,
  IN UINTN        EnrollDbSize,
  IN CONST UINT8  *ExpectedDb,
  IN UINTN        ExpectedDbSize
  )
{
  EFI_HANDLE        LoadedImage;
  EFI_STATUS        RecoveryStatus;
  UNIT_TEST_STATUS  Result;
  EFI_STATUS        Status;

  LoadedImage = NULL;
  Result      = UNIT_TEST_ERROR_TEST_FAILED;

  Status = ValidateEcitClaims ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

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

  Status = ApplyDbUpdate (
             EnrollDb,
             EnrollDbSize,
             ExpectedDb,
             ExpectedDbSize
             );
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("ML-DSA-87 PK could not enroll the %a TBS hash in db: %r\n", Version, Status);
    goto Exit;
  }

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
    goto Exit;
  }

  Status = gBS->StartImage (LoadedImage, NULL, NULL);
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("The authorized ML-DSA image did not start: %r\n", Status);
    goto Exit;
  }

  LoadedImage = NULL;
  UT_LOG_INFO ("%a db TBS hash authorized and started the ML-DSA-87 image\n", Version);

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
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    FreeUnitTestFramework (Framework);
    return Status;
  }

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

  Status = RunAllTestSuites (Framework);
  FreeUnitTestFramework (Framework);
  return Status;
}
