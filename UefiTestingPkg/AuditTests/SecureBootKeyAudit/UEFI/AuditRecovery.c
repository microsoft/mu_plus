/** @file
  Recognized-state recovery for the Secure Boot key audit.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Guid/ImageAuthentication.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>

#include "AuditRecovery.h"
#include "SecureBootState.h"
#include "TestData.h"

BOOLEAN
RecoveryStateIsRecognized (
  IN CONST AUDIT_BASELINE_SNAPSHOT  *Snapshot
  )
{
  if ((Snapshot->PkState == AuditVariableStateAbsent) &&
      (Snapshot->DbState == AuditVariableStateAbsent) &&
      (Snapshot->DbxState == AuditVariableStateAbsent))
  {
    return (BOOLEAN)(
                     (Snapshot->SetupMode == SETUP_MODE) &&
                     (Snapshot->SecureBoot == SECURE_BOOT_MODE_DISABLE)
                     );
  }

  if ((Snapshot->SetupMode != USER_MODE) ||
      (Snapshot->SecureBoot != SECURE_BOOT_MODE_ENABLE) ||
      (Snapshot->PkAttributes != SECURE_BOOT_KEY_ATTRIBUTES))
  {
    return FALSE;
  }

  if (Snapshot->PkState == AuditVariableStatePkB) {
    return (BOOLEAN)(
                     (Snapshot->DbState == AuditVariableStateAbsent) &&
                     (Snapshot->DbxState == AuditVariableStateAbsent)
                     );
  }

  if (Snapshot->PkState == AuditVariableStatePkA) {
    if (Snapshot->DbState == AuditVariableStateAbsent) {
      return (BOOLEAN)(Snapshot->DbxState == AuditVariableStateAbsent);
    }

    if ((Snapshot->DbState != AuditVariableStateDbCaMigration) &&
        (Snapshot->DbState != AuditVariableStateDbCaCapability) &&
        (Snapshot->DbState != AuditVariableStateDbMultiSigner))
    {
      return FALSE;
    }

    if (Snapshot->DbAttributes != SECURE_BOOT_KEY_ATTRIBUTES) {
      return FALSE;
    }

    if (Snapshot->DbxState == AuditVariableStateAbsent) {
      return TRUE;
    }

    return (BOOLEAN)(
                     ((Snapshot->DbxState == AuditVariableStateDbxRsaSigner) ||
                      (Snapshot->DbxState == AuditVariableStateDbxBothSigners) ||
                      (Snapshot->DbxState == AuditVariableStateDbxMigrationRsa) ||
                      (Snapshot->DbxState == AuditVariableStateDbxMigrationRsaEcdsa) ||
                      (Snapshot->DbxState == AuditVariableStateDbxMigrationAll)) &&
                     (Snapshot->DbxAttributes == SECURE_BOOT_KEY_ATTRIBUTES)
                     );
  }

  if (Snapshot->PkState == AuditVariableStatePkEcdsa) {
    if (Snapshot->DbState == AuditVariableStateAbsent) {
      return (BOOLEAN)(Snapshot->DbxState == AuditVariableStateAbsent);
    }

    if ((Snapshot->DbState != AuditVariableStateDbCaCapability) ||
        (Snapshot->DbAttributes != SECURE_BOOT_KEY_ATTRIBUTES))
    {
      return FALSE;
    }

    if (Snapshot->DbxState == AuditVariableStateAbsent) {
      return TRUE;
    }

    return (BOOLEAN)(
                     (Snapshot->DbxState == AuditVariableStateDbxCaCapability) &&
                     (Snapshot->DbxAttributes == SECURE_BOOT_KEY_ATTRIBUTES)
                     );
  }

  if (Snapshot->PkState != AuditVariableStatePkMldsa) {
    return FALSE;
  }

  if (Snapshot->DbState == AuditVariableStateAbsent) {
    return (BOOLEAN)(Snapshot->DbxState == AuditVariableStateAbsent);
  }

  if ((Snapshot->DbState == AuditVariableStateDbV1) ||
      (Snapshot->DbState == AuditVariableStateDbV2))
  {
    return (BOOLEAN)(
                     (Snapshot->DbAttributes == SECURE_BOOT_KEY_ATTRIBUTES) &&
                     (Snapshot->DbxState == AuditVariableStateAbsent)
                     );
  }

  if ((Snapshot->DbState != AuditVariableStateDbMultiSigner) ||
      (Snapshot->DbAttributes != SECURE_BOOT_KEY_ATTRIBUTES))
  {
    return FALSE;
  }

  if (Snapshot->DbxState == AuditVariableStateAbsent) {
    return TRUE;
  }

  return (BOOLEAN)(
                   ((Snapshot->DbxState == AuditVariableStateDbxRsaSigner) ||
                    (Snapshot->DbxState == AuditVariableStateDbxMldsaSigner) ||
                    (Snapshot->DbxState == AuditVariableStateDbxBothSigners)) &&
                   (Snapshot->DbxAttributes == SECURE_BOOT_KEY_ATTRIBUTES)
                   );
}

STATIC
EFI_STATUS
RecoverDb (
  IN AUDIT_VARIABLE_STATE  PkState
  )
{
  UINT32       Attributes;
  CONST UINT8  *DeleteDb;
  UINTN        DeleteDbSize;
  UINT8        *Db;
  UINTN        DbSize;
  EFI_STATUS   Status;

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

  if (PkMatches (
        Db,
        DbSize,
        gSecureBootKeyAuditDbTbsHashV1,
        gSecureBootKeyAuditDbTbsHashV1Size
        ))
  {
    DeleteDb     = gSecureBootKeyAuditDeleteDbV1ByMldsa;
    DeleteDbSize = gSecureBootKeyAuditDeleteDbV1ByMldsaSize;
  } else if (PkMatches (
               Db,
               DbSize,
               gSecureBootKeyAuditDbTbsHashV2,
               gSecureBootKeyAuditDbTbsHashV2Size
               ))
  {
    DeleteDb     = gSecureBootKeyAuditDeleteDbV2ByMldsa;
    DeleteDbSize = gSecureBootKeyAuditDeleteDbV2ByMldsaSize;
  } else if (PkMatches (
               Db,
               DbSize,
               gSecureBootKeyAuditDbMultiSigner,
               gSecureBootKeyAuditDbMultiSignerSize
               ))
  {
    if (PkState == AuditVariableStatePkA) {
      DeleteDb     = gSecureBootKeyAuditDeleteDbCaMigrationByA;
      DeleteDbSize = gSecureBootKeyAuditDeleteDbCaMigrationByASize;
    } else {
      DeleteDb     = gSecureBootKeyAuditDeleteDbMultiSignerByMldsa;
      DeleteDbSize = gSecureBootKeyAuditDeleteDbMultiSignerByMldsaSize;
    }
  } else if (PkMatches (
               Db,
               DbSize,
               gSecureBootKeyAuditDbCaMigration,
               gSecureBootKeyAuditDbCaMigrationSize
               ))
  {
    DeleteDb     = gSecureBootKeyAuditDeleteDbCaMigrationByA;
    DeleteDbSize = gSecureBootKeyAuditDeleteDbCaMigrationByASize;
  } else if (PkMatches (
               Db,
               DbSize,
               gSecureBootKeyAuditDbCaRsa,
               gSecureBootKeyAuditDbCaRsaSize
               ) ||
             PkMatches (
               Db,
               DbSize,
               gSecureBootKeyAuditDbCaRsaEcdsa,
               gSecureBootKeyAuditDbCaRsaEcdsaSize
               ))
  {
    DeleteDb     = gSecureBootKeyAuditDeleteDbCaMigrationByA;
    DeleteDbSize = gSecureBootKeyAuditDeleteDbCaMigrationByASize;
  } else if (PkMatches (
               Db,
               DbSize,
               gSecureBootKeyAuditDbCaEcdsa,
               gSecureBootKeyAuditDbCaEcdsaSize
               ) ||
             PkMatches (
               Db,
               DbSize,
               gSecureBootKeyAuditDbCaEcdsaMldsa,
               gSecureBootKeyAuditDbCaEcdsaMldsaSize
               ))
  {
    DeleteDb     = gSecureBootKeyAuditDeleteDbCaMigrationByEcdsa;
    DeleteDbSize = gSecureBootKeyAuditDeleteDbCaMigrationByEcdsaSize;
  } else {
    FreePool (Db);
    UT_LOG_ERROR ("Refusing to modify an unknown existing db\n");
    return EFI_ACCESS_DENIED;
  }

  FreePool (Db);
  return ApplyDbUpdate (
           DeleteDb,
           DeleteDbSize,
           NULL,
           0
           );
}

STATIC
EFI_STATUS
RecoverDbx (
  IN AUDIT_VARIABLE_STATE  PkState
  )
{
  UINT32       Attributes;
  CONST UINT8  *DeleteDbx;
  UINTN        DeleteDbxSize;
  UINT8        *Dbx;
  UINTN        DbxSize;
  EFI_STATUS   Status;

  Status = ReadVariable (
             EFI_IMAGE_SECURITY_DATABASE1,
             &gEfiImageSecurityDatabaseGuid,
             &Attributes,
             &Dbx,
             &DbxSize
             );
  if (Status == EFI_NOT_FOUND) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (PkMatches (
        Dbx,
        DbxSize,
        gSecureBootKeyAuditDbxRsaSigner,
        gSecureBootKeyAuditDbxRsaSignerSize
        ) ||
      PkMatches (
        Dbx,
        DbxSize,
        gSecureBootKeyAuditDbxMldsaSigner,
        gSecureBootKeyAuditDbxMldsaSignerSize
        ) ||
      PkMatches (
        Dbx,
        DbxSize,
        gSecureBootKeyAuditDbxBothSigners,
        gSecureBootKeyAuditDbxBothSignersSize
        ))
  {
    if (PkState == AuditVariableStatePkA) {
      DeleteDbx     = gSecureBootKeyAuditDeleteDbxCaMigrationByA;
      DeleteDbxSize = gSecureBootKeyAuditDeleteDbxCaMigrationByASize;
    } else {
      DeleteDbx     = gSecureBootKeyAuditDeleteDbxByMldsa;
      DeleteDbxSize = gSecureBootKeyAuditDeleteDbxByMldsaSize;
    }
  } else if (PkMatches (
               Dbx,
               DbxSize,
               gSecureBootKeyAuditDbxMigrationRsa,
               gSecureBootKeyAuditDbxMigrationRsaSize
               ) ||
             PkMatches (
               Dbx,
               DbxSize,
               gSecureBootKeyAuditDbxMigrationRsaEcdsa,
               gSecureBootKeyAuditDbxMigrationRsaEcdsaSize
               ) ||
             PkMatches (
               Dbx,
               DbxSize,
               gSecureBootKeyAuditDbxMigrationAll,
               gSecureBootKeyAuditDbxMigrationAllSize
               ))
  {
    DeleteDbx     = gSecureBootKeyAuditDeleteDbxCaMigrationByA;
    DeleteDbxSize = gSecureBootKeyAuditDeleteDbxCaMigrationByASize;
  } else if (PkMatches (
               Dbx,
               DbxSize,
               gSecureBootKeyAuditDbCaEcdsa,
               gSecureBootKeyAuditDbCaEcdsaSize
               ) ||
             PkMatches (
               Dbx,
               DbxSize,
               gSecureBootKeyAuditDbCaEcdsaMldsa,
               gSecureBootKeyAuditDbCaEcdsaMldsaSize
               ))
  {
    DeleteDbx     = gSecureBootKeyAuditDeleteDbxCaMigrationByEcdsa;
    DeleteDbxSize = gSecureBootKeyAuditDeleteDbxCaMigrationByEcdsaSize;
  } else {
    FreePool (Dbx);
    UT_LOG_ERROR ("Refusing to modify an unknown existing dbx\n");
    return EFI_ACCESS_DENIED;
  }

  FreePool (Dbx);
  return ApplyDbxUpdate (
           DeleteDbx,
           DeleteDbxSize,
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
  AUDIT_BASELINE_SNAPSHOT  Snapshot;
  EFI_STATUS               Status;

  Status = CaptureBaselineSnapshot (&Snapshot);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!RecoveryStateIsRecognized (&Snapshot)) {
    UT_LOG_ERROR (
      "Refusing recovery from unrecognized state: setup_mode=%u secure_boot=%u "
      "pk_state=%u db_state=%u dbx_state=%u\n",
      Snapshot.SetupMode,
      Snapshot.SecureBoot,
      Snapshot.PkState,
      Snapshot.DbState,
      Snapshot.DbxState
      );
    return EFI_ACCESS_DENIED;
  }

  if (Snapshot.PkState == AuditVariableStateAbsent) {
    return EFI_SUCCESS;
  }

  if (Snapshot.DbxState != AuditVariableStateAbsent) {
    Status = RecoverDbx (Snapshot.PkState);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (Snapshot.DbState != AuditVariableStateAbsent) {
    Status = RecoverDb (Snapshot.PkState);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  switch (Snapshot.PkState) {
    case AuditVariableStatePkA:
      return ApplyPkUpdate (
               gSecureBootKeyAuditRecoveryDeleteByA,
               gSecureBootKeyAuditRecoveryDeleteByASize,
               NULL,
               0
               );
    case AuditVariableStatePkB:
      return ApplyPkUpdate (
               gSecureBootKeyAuditRecoveryDeleteByB,
               gSecureBootKeyAuditRecoveryDeleteByBSize,
               NULL,
               0
               );
    case AuditVariableStatePkMldsa:
      return ApplyPkUpdate (
               gSecureBootKeyAuditRecoveryDeleteByMldsa,
               gSecureBootKeyAuditRecoveryDeleteByMldsaSize,
               NULL,
               0
               );
    case AuditVariableStatePkEcdsa:
      return ApplyPkUpdate (
               gSecureBootKeyAuditRecoveryDeleteByEcdsa,
               gSecureBootKeyAuditRecoveryDeleteByEcdsaSize,
               NULL,
               0
               );
    default:
      return EFI_ACCESS_DENIED;
  }
}

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
