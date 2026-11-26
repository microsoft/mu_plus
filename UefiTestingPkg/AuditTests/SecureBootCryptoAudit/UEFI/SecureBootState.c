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

/**
  Reads a UEFI variable into a newly allocated buffer.

  @param[in]  Name        Name of the variable to read.
  @param[in]  Guid        Vendor GUID of the variable.
  @param[out] Attributes  Attributes of the variable.
  @param[out] Data        Allocated buffer containing the variable data.
  @param[out] DataSize    Size of the variable data in bytes.

  @retval EFI_SUCCESS           The variable was read successfully.
  @retval EFI_NOT_FOUND         The variable does not exist.
  @retval EFI_OUT_OF_RESOURCES  The data buffer could not be allocated.
  @return                       An error returned by GetVariable().
**/
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

/**
  Reads a one-byte Secure Boot mode variable.

  @param[in]  Name   Name of the mode variable to read.
  @param[out] Value  Value read from the mode variable.

  @retval EFI_SUCCESS  The mode variable was read successfully.
  @return              An error returned by GetVariable().
**/
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

/**
  Compares a Secure Boot variable payload with an expected payload.

  @param[in] Pk            Variable payload to compare.
  @param[in] PkSize        Size of Pk in bytes.
  @param[in] Expected      Expected payload.
  @param[in] ExpectedSize  Size of Expected in bytes.

  @return TRUE  The payloads have equal sizes and contents.
  @return FALSE The payloads differ in size or contents.
**/
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

/**
  Verifies the current platform key against an expected payload.

  @param[in] Expected      Expected platform key, or NULL if it should be absent.
  @param[in] ExpectedSize  Size of Expected in bytes, or zero for absence.

  @retval EFI_SUCCESS           The platform key has the expected state.
  @retval EFI_COMPROMISED_DATA  The platform key differs from the expected state.
  @return                       An error propagated while reading the platform key.
**/
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

/**
  Verifies a Secure Boot signature database against an expected payload.

  @param[in] Name          Name of the signature database variable.
  @param[in] Expected      Expected database payload, or NULL if it should be absent.
  @param[in] ExpectedSize  Size of Expected in bytes, or zero for absence.

  @retval EFI_SUCCESS           The signature database has the expected state.
  @retval EFI_COMPROMISED_DATA  The database differs from the expected state.
  @return                       An error propagated while reading the database.
**/
STATIC
EFI_STATUS
VerifySignatureDatabase (
  IN CONST CHAR16  *Name,
  IN CONST UINT8   *Expected,
  IN UINTN         ExpectedSize
  )
{
  UINT32      Attributes;
  UINT8       *Db;
  UINTN       DbSize;
  EFI_STATUS  Status;

  Status = ReadVariable (
             Name,
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

/**
  Verifies the current db variable against an expected payload.

  @param[in] Expected      Expected db payload, or NULL if it should be absent.
  @param[in] ExpectedSize  Size of Expected in bytes, or zero for absence.

  @retval EFI_SUCCESS           db has the expected state.
  @retval EFI_COMPROMISED_DATA  db differs from the expected state.
  @return                       An error propagated while reading db.
**/
EFI_STATUS
VerifyDb (
  IN CONST UINT8  *Expected,
  IN UINTN        ExpectedSize
  )
{
  return VerifySignatureDatabase (
           EFI_IMAGE_SECURITY_DATABASE,
           Expected,
           ExpectedSize
           );
}

/**
  Verifies the current dbx variable against an expected payload.

  @param[in] Expected      Expected dbx payload, or NULL if it should be absent.
  @param[in] ExpectedSize  Size of Expected in bytes, or zero for absence.

  @retval EFI_SUCCESS           dbx has the expected state.
  @retval EFI_COMPROMISED_DATA  dbx differs from the expected state.
  @return                       An error propagated while reading dbx.
**/
EFI_STATUS
VerifyDbx (
  IN CONST UINT8  *Expected,
  IN UINTN        ExpectedSize
  )
{
  return VerifySignatureDatabase (
           EFI_IMAGE_SECURITY_DATABASE1,
           Expected,
           ExpectedSize
           );
}

/**
  Applies an authenticated platform key update and verifies the result.

  @param[in] AuthenticatedUpdate      Authenticated update payload.
  @param[in] AuthenticatedUpdateSize  Size of AuthenticatedUpdate in bytes.
  @param[in] ExpectedPk               Expected platform key after the update.
  @param[in] ExpectedPkSize           Size of ExpectedPk in bytes.

  @retval EFI_SUCCESS           The update succeeded and produced the expected key.
  @retval EFI_COMPROMISED_DATA  The resulting platform key was not as expected.
  @return                       An error propagated while applying or verifying the update.
**/
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

/**
  Applies an authenticated dbx update and verifies the result.

  @param[in] AuthenticatedUpdate      Authenticated update payload.
  @param[in] AuthenticatedUpdateSize  Size of AuthenticatedUpdate in bytes.
  @param[in] ExpectedDbx              Expected dbx payload after the update.
  @param[in] ExpectedDbxSize          Size of ExpectedDbx in bytes.

  @retval EFI_SUCCESS           The update succeeded and produced the expected dbx.
  @retval EFI_COMPROMISED_DATA  The resulting dbx was not as expected.
  @return                       An error propagated while applying or verifying the update.
**/
EFI_STATUS
ApplyDbxUpdate (
  IN CONST UINT8  *AuthenticatedUpdate,
  IN UINTN        AuthenticatedUpdateSize,
  IN CONST UINT8  *ExpectedDbx,
  IN UINTN        ExpectedDbxSize
  )
{
  EFI_STATUS  Status;

  Status = gRT->SetVariable (
                  EFI_IMAGE_SECURITY_DATABASE1,
                  &gEfiImageSecurityDatabaseGuid,
                  SECURE_BOOT_KEY_ATTRIBUTES,
                  AuthenticatedUpdateSize,
                  (VOID *)AuthenticatedUpdate
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return VerifyDbx (ExpectedDbx, ExpectedDbxSize);
}

/**
  Applies an authenticated db update and verifies the result.

  @param[in] AuthenticatedUpdate      Authenticated update payload.
  @param[in] AuthenticatedUpdateSize  Size of AuthenticatedUpdate in bytes.
  @param[in] ExpectedDb               Expected db payload after the update.
  @param[in] ExpectedDbSize           Size of ExpectedDb in bytes.

  @retval EFI_SUCCESS           The update succeeded and produced the expected db.
  @retval EFI_COMPROMISED_DATA  The resulting db was not as expected.
  @return                       An error propagated while applying or verifying the update.
**/
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

/**
  Returns the audit label for a recognized variable state.

  @param[in] State  Variable state to describe.

  @return A static ASCII label for State, or "UNREAD" for an unhandled value.
**/
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
    case AuditVariableStatePkEcdsa:
      return "PK_ECDSA_P256";
    case AuditVariableStateDbV1:
      return "DB_TBS_V1";
    case AuditVariableStateDbV2:
      return "DB_TBS_V2";
    case AuditVariableStateDbMultiSigner:
      return "DB_MULTI_SIGNER";
    case AuditVariableStateDbCaMigration:
      return "DB_CA_MIGRATION";
    case AuditVariableStateDbCaCapability:
      return "DB_CA_CAPABILITY";
    case AuditVariableStateDbxRsaSigner:
      return "DBX_RSA_SIGNER";
    case AuditVariableStateDbxMldsaSigner:
      return "DBX_MLDSA_SIGNER";
    case AuditVariableStateDbxBothSigners:
      return "DBX_BOTH_SIGNERS";
    case AuditVariableStateDbxMigrationRsa:
      return "DBX_MIGRATION_RSA";
    case AuditVariableStateDbxMigrationRsaEcdsa:
      return "DBX_MIGRATION_RSA_ECDSA";
    case AuditVariableStateDbxMigrationAll:
      return "DBX_MIGRATION_ALL";
    case AuditVariableStateDbxCaCapability:
      return "DBX_CA_CAPABILITY";
    case AuditVariableStateUnknown:
      return "UNKNOWN";
    default:
      return "UNREAD";
  }
}

/**
  Captures the current Secure Boot mode and key database states.

  @param[out] Snapshot  Captured Secure Boot baseline state.

  @retval EFI_SUCCESS  The baseline snapshot was captured successfully.
  @return              An error propagated while reading a Secure Boot variable.
**/
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
  Snapshot->DbxState   = AuditVariableStateUnread;

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
    } else if (PkMatches (
                 Data,
                 DataSize,
                 gSecureBootKeyAuditPkEcdsa,
                 gSecureBootKeyAuditPkEcdsaSize
                 ))
    {
      Snapshot->PkState = AuditVariableStatePkEcdsa;
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
    } else if (PkMatches (
                 Data,
                 DataSize,
                 gSecureBootKeyAuditDbMultiSigner,
                 gSecureBootKeyAuditDbMultiSignerSize
                 ))
    {
      Snapshot->DbState = AuditVariableStateDbMultiSigner;
    } else if (PkMatches (
                 Data,
                 DataSize,
                 gSecureBootKeyAuditDbCaMigration,
                 gSecureBootKeyAuditDbCaMigrationSize
                 ))
    {
      Snapshot->DbState = AuditVariableStateDbCaMigration;
    } else if (PkMatches (
                 Data,
                 DataSize,
                 gSecureBootKeyAuditDbCaRsa,
                 gSecureBootKeyAuditDbCaRsaSize
                 ) ||
               PkMatches (
                 Data,
                 DataSize,
                 gSecureBootKeyAuditDbCaEcdsa,
                 gSecureBootKeyAuditDbCaEcdsaSize
                 ) ||
               PkMatches (
                 Data,
                 DataSize,
                 gSecureBootKeyAuditDbCaRsaEcdsa,
                 gSecureBootKeyAuditDbCaRsaEcdsaSize
                 ) ||
               PkMatches (
                 Data,
                 DataSize,
                 gSecureBootKeyAuditDbCaEcdsaMldsa,
                 gSecureBootKeyAuditDbCaEcdsaMldsaSize
                 ))
    {
      Snapshot->DbState = AuditVariableStateDbCaCapability;
    } else {
      Snapshot->DbState = AuditVariableStateUnknown;
    }

    FreePool (Data);
  }

  Status = ReadVariable (
             EFI_IMAGE_SECURITY_DATABASE1,
             &gEfiImageSecurityDatabaseGuid,
             &Attributes,
             &Data,
             &DataSize
             );
  if (Status == EFI_NOT_FOUND) {
    Snapshot->DbxState = AuditVariableStateAbsent;
  } else if (EFI_ERROR (Status)) {
    return Status;
  } else {
    Snapshot->DbxAttributes = Attributes;
    Snapshot->DbxSize       = DataSize;
    if (PkMatches (
          Data,
          DataSize,
          gSecureBootKeyAuditDbxRsaSigner,
          gSecureBootKeyAuditDbxRsaSignerSize
          ))
    {
      Snapshot->DbxState = AuditVariableStateDbxRsaSigner;
    } else if (PkMatches (
                 Data,
                 DataSize,
                 gSecureBootKeyAuditDbxMldsaSigner,
                 gSecureBootKeyAuditDbxMldsaSignerSize
                 ))
    {
      Snapshot->DbxState = AuditVariableStateDbxMldsaSigner;
    } else if (PkMatches (
                 Data,
                 DataSize,
                 gSecureBootKeyAuditDbxBothSigners,
                 gSecureBootKeyAuditDbxBothSignersSize
                 ))
    {
      Snapshot->DbxState = AuditVariableStateDbxBothSigners;
    } else if (PkMatches (
                 Data,
                 DataSize,
                 gSecureBootKeyAuditDbxMigrationRsa,
                 gSecureBootKeyAuditDbxMigrationRsaSize
                 ))
    {
      Snapshot->DbxState = AuditVariableStateDbxMigrationRsa;
    } else if (PkMatches (
                 Data,
                 DataSize,
                 gSecureBootKeyAuditDbxMigrationRsaEcdsa,
                 gSecureBootKeyAuditDbxMigrationRsaEcdsaSize
                 ))
    {
      Snapshot->DbxState = AuditVariableStateDbxMigrationRsaEcdsa;
    } else if (PkMatches (
                 Data,
                 DataSize,
                 gSecureBootKeyAuditDbxMigrationAll,
                 gSecureBootKeyAuditDbxMigrationAllSize
                 ))
    {
      Snapshot->DbxState = AuditVariableStateDbxMigrationAll;
    } else if (PkMatches (
                 Data,
                 DataSize,
                 gSecureBootKeyAuditDbCaEcdsa,
                 gSecureBootKeyAuditDbCaEcdsaSize
                 ) ||
               PkMatches (
                 Data,
                 DataSize,
                 gSecureBootKeyAuditDbCaEcdsaMldsa,
                 gSecureBootKeyAuditDbCaEcdsaMldsaSize
                 ))
    {
      Snapshot->DbxState = AuditVariableStateDbxCaCapability;
    } else {
      Snapshot->DbxState = AuditVariableStateUnknown;
    }

    FreePool (Data);
  }

  return EFI_SUCCESS;
}

/**
  Captures and validates the Secure Boot setup-mode baseline.

  @param[out] Snapshot  Captured Secure Boot baseline state.

  @retval EFI_SUCCESS        The system is at the empty setup-mode baseline.
  @retval EFI_ACCESS_DENIED  A Secure Boot variable has an unknown state.
  @retval EFI_NOT_READY      A recognized interrupted audit state requires recovery.
  @return                    An error propagated while capturing the baseline.
**/
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
    "db_state=%a db_attributes=0x%08x db_size=%u "
    "dbx_state=%a dbx_attributes=0x%08x dbx_size=%u\n",
    Snapshot->SetupMode,
    Snapshot->SecureBoot,
    AuditVariableStateName (Snapshot->PkState),
    Snapshot->PkAttributes,
    Snapshot->PkSize,
    AuditVariableStateName (Snapshot->DbState),
    Snapshot->DbAttributes,
    Snapshot->DbSize,
    AuditVariableStateName (Snapshot->DbxState),
    Snapshot->DbxAttributes,
    Snapshot->DbxSize
    );

  if ((Snapshot->SetupMode == SETUP_MODE) &&
      (Snapshot->SecureBoot == SECURE_BOOT_MODE_DISABLE) &&
      (Snapshot->PkState == AuditVariableStateAbsent) &&
      (Snapshot->DbState == AuditVariableStateAbsent) &&
      (Snapshot->DbxState == AuditVariableStateAbsent))
  {
    return EFI_SUCCESS;
  }

  if ((Snapshot->PkState == AuditVariableStateUnknown) ||
      (Snapshot->PkState == AuditVariableStateUnread) ||
      (Snapshot->DbState == AuditVariableStateUnknown) ||
      (Snapshot->DbState == AuditVariableStateUnread) ||
      (Snapshot->DbxState == AuditVariableStateUnknown) ||
      (Snapshot->DbxState == AuditVariableStateUnread))
  {
    UT_LOG_ERROR ("Refusing to recover an unknown Secure Boot state\n");
    return EFI_ACCESS_DENIED;
  }

  UT_LOG_WARNING ("Recognized interrupted test state requires explicit recovery\n");
  return EFI_NOT_READY;
}

/**
  Requires the system to be at the empty Secure Boot setup-mode baseline.

  @retval EFI_SUCCESS        The system is at the required baseline.
  @retval EFI_ACCESS_DENIED  A Secure Boot variable has an unknown state.
  @retval EFI_NOT_READY      A recognized interrupted audit state requires recovery.
  @return                    An error propagated while inspecting the baseline.
**/
EFI_STATUS
RequireSetupModeBaseline (
  VOID
  )
{
  AUDIT_BASELINE_SNAPSHOT  Snapshot;

  return InspectSetupModeBaseline (&Snapshot);
}
