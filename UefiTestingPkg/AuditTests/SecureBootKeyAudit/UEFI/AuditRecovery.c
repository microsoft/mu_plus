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
      (Snapshot->DbState == AuditVariableStateAbsent))
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

  if ((Snapshot->PkState == AuditVariableStatePkA) ||
      (Snapshot->PkState == AuditVariableStatePkB))
  {
    return (BOOLEAN)(Snapshot->DbState == AuditVariableStateAbsent);
  }

  if (Snapshot->PkState != AuditVariableStatePkMldsa) {
    return FALSE;
  }

  if (Snapshot->DbState == AuditVariableStateAbsent) {
    return TRUE;
  }

  return (BOOLEAN)(
                   ((Snapshot->DbState == AuditVariableStateDbV1) ||
                    (Snapshot->DbState == AuditVariableStateDbV2)) &&
                   (Snapshot->DbAttributes == SECURE_BOOT_KEY_ATTRIBUTES)
                   );
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
  AUDIT_BASELINE_SNAPSHOT  Snapshot;
  EFI_STATUS               Status;

  Status = CaptureBaselineSnapshot (&Snapshot);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!RecoveryStateIsRecognized (&Snapshot)) {
    UT_LOG_ERROR (
      "Refusing recovery from unrecognized state: setup_mode=%u secure_boot=%u "
      "pk_state=%u db_state=%u\n",
      Snapshot.SetupMode,
      Snapshot.SecureBoot,
      Snapshot.PkState,
      Snapshot.DbState
      );
    return EFI_ACCESS_DENIED;
  }

  if (Snapshot.PkState == AuditVariableStateAbsent) {
    return EFI_SUCCESS;
  }

  if (Snapshot.DbState != AuditVariableStateAbsent) {
    Status = RecoverDb ();
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
