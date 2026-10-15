/** @file
  Secure Boot variable state access for the key audit.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Guid/GlobalVariable.h>
#include <Guid/ImageAuthentication.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UnitTestLib.h>

#include "SecureBootState.h"
#include "TestData.h"

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

EFI_STATUS
RequireSetupModeBaseline (
  VOID
  )
{
  AUDIT_BASELINE_SNAPSHOT  Snapshot;

  return InspectSetupModeBaseline (&Snapshot);
}
