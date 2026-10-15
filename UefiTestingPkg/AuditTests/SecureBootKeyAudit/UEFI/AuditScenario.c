/** @file
  Data-driven authenticated-variable scenarios for the Secure Boot key audit.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/BaseCryptLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UnitTestLib.h>

#include "AuditScenario.h"
#include "SecureBootState.h"

typedef struct {
  BOOLEAN    Exists;
  UINT32     Attributes;
  UINT8      *Data;
  UINTN      DataSize;
} AUDIT_VARIABLE_VALUE;

STATIC
VOID
FreeVariableValue (
  IN OUT AUDIT_VARIABLE_VALUE  *Value
  )
{
  if (Value->Data != NULL) {
    FreePool (Value->Data);
  }

  ZeroMem (Value, sizeof (*Value));
}

STATIC
EFI_STATUS
CaptureVariableValue (
  IN  CONST CHAR16          *Name,
  IN  CONST EFI_GUID        *Guid,
  OUT AUDIT_VARIABLE_VALUE  *Value
  )
{
  EFI_STATUS  Status;

  ZeroMem (Value, sizeof (*Value));
  Status = ReadVariable (
             Name,
             Guid,
             &Value->Attributes,
             &Value->Data,
             &Value->DataSize
             );
  if (Status == EFI_NOT_FOUND) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Value->Exists = TRUE;
  return EFI_SUCCESS;
}

STATIC
BOOLEAN
VariableValueMatches (
  IN CONST AUDIT_VARIABLE_VALUE  *Value,
  IN CONST UINT8                 *Expected,
  IN UINTN                       ExpectedSize,
  IN UINT32                      ExpectedAttributes
  )
{
  if (Expected == NULL) {
    return (BOOLEAN)(!Value->Exists && (ExpectedSize == 0));
  }

  return (BOOLEAN)(
                   Value->Exists &&
                   (Value->Attributes == ExpectedAttributes) &&
                   (Value->DataSize == ExpectedSize) &&
                   (CompareMem (Value->Data, Expected, ExpectedSize) == 0)
                   );
}

STATIC
BOOLEAN
VariableValuesEqual (
  IN CONST AUDIT_VARIABLE_VALUE  *First,
  IN CONST AUDIT_VARIABLE_VALUE  *Second
  )
{
  if (First->Exists != Second->Exists) {
    return FALSE;
  }

  if (!First->Exists) {
    return TRUE;
  }

  return (BOOLEAN)(
                   (First->Attributes == Second->Attributes) &&
                   (First->DataSize == Second->DataSize) &&
                   (CompareMem (First->Data, Second->Data, First->DataSize) == 0)
                   );
}

STATIC
EFI_STATUS
LogVariableValue (
  IN CONST CHAR8                 *ScenarioId,
  IN CONST CHAR8                 *Stage,
  IN CONST AUDIT_VARIABLE_VALUE  *Value
  )
{
  STATIC CONST CHAR8  Hex[] = "0123456789abcdef";
  UINT8               Digest[SHA256_DIGEST_SIZE];
  CHAR8               DigestString[(SHA256_DIGEST_SIZE * 2) + 1];
  UINTN               Index;

  if (!Value->Exists) {
    UT_LOG_INFO (
      "[AUDIT_CONFORMANCE] scenario=%a stage=%a state=ABSENT\n",
      ScenarioId,
      Stage
      );
    return EFI_SUCCESS;
  }

  if (!Sha256HashAll (Value->Data, Value->DataSize, Digest)) {
    return EFI_DEVICE_ERROR;
  }

  for (Index = 0; Index < SHA256_DIGEST_SIZE; Index++) {
    DigestString[Index * 2]     = Hex[Digest[Index] >> 4];
    DigestString[Index * 2 + 1] = Hex[Digest[Index] & 0x0F];
  }

  DigestString[sizeof (DigestString) - 1] = '\0';
  UT_LOG_INFO (
    "[AUDIT_CONFORMANCE] scenario=%a stage=%a state=PRESENT "
    "attributes=0x%08x size=%u sha256=%a\n",
    ScenarioId,
    Stage,
    Value->Attributes,
    Value->DataSize,
    DigestString
    );
  return EFI_SUCCESS;
}

EFI_STATUS
ExecuteAuthVariableScenario (
  IN CONST AUDIT_AUTH_VARIABLE_SCENARIO  *Scenario
  )
{
  AUDIT_VARIABLE_VALUE  After;
  AUDIT_VARIABLE_VALUE  Before;
  EFI_STATUS            OperationStatus;
  EFI_STATUS            Status;

  if ((Scenario == NULL) ||
      (Scenario->ScenarioId == NULL) ||
      (Scenario->VariableName == NULL) ||
      (Scenario->VendorGuid == NULL) ||
      (Scenario->AuthenticatedUpdate == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&Before, sizeof (Before));
  ZeroMem (&After, sizeof (After));
  Status = CaptureVariableValue (
             Scenario->VariableName,
             Scenario->VendorGuid,
             &Before
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LogVariableValue (Scenario->ScenarioId, "before", &Before);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if (!VariableValueMatches (
         &Before,
         Scenario->ExpectedBefore,
         Scenario->ExpectedBeforeSize,
         Scenario->ExpectedBeforeAttributes
         ))
  {
    UT_LOG_ERROR (
      "[AUDIT_CONFORMANCE] scenario=%a precondition=FAILED\n",
      Scenario->ScenarioId
      );
    Status = EFI_COMPROMISED_DATA;
    goto Exit;
  }

  OperationStatus = gRT->SetVariable (
                           (CHAR16 *)Scenario->VariableName,
                           (EFI_GUID *)Scenario->VendorGuid,
                           Scenario->Attributes,
                           Scenario->AuthenticatedUpdateSize,
                           (VOID *)Scenario->AuthenticatedUpdate
                           );

  Status = CaptureVariableValue (
             Scenario->VariableName,
             Scenario->VendorGuid,
             &After
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = LogVariableValue (Scenario->ScenarioId, "after", &After);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if (OperationStatus != Scenario->ExpectedStatus) {
    UT_LOG_ERROR (
      "[AUDIT_CONFORMANCE] scenario=%a expected_status=%r actual_status=%r\n",
      Scenario->ScenarioId,
      Scenario->ExpectedStatus,
      OperationStatus
      );
    Status = EFI_COMPROMISED_DATA;
    goto Exit;
  }

  if (!VariableValueMatches (
         &After,
         Scenario->ExpectedAfter,
         Scenario->ExpectedAfterSize,
         Scenario->ExpectedAfterAttributes
         ))
  {
    UT_LOG_ERROR (
      "[AUDIT_CONFORMANCE] scenario=%a postcondition=FAILED\n",
      Scenario->ScenarioId
      );
    Status = EFI_COMPROMISED_DATA;
    goto Exit;
  }

  if (!Scenario->ExpectMutation && !VariableValuesEqual (&Before, &After)) {
    UT_LOG_ERROR (
      "[AUDIT_CONFORMANCE] scenario=%a atomic_rejection=FAILED\n",
      Scenario->ScenarioId
      );
    Status = EFI_COMPROMISED_DATA;
    goto Exit;
  }

  UT_LOG_INFO (
    "[AUDIT_CONFORMANCE] scenario=%a expected_status=%r actual_status=%r "
    "atomic_rejection=%a result=PASS\n",
    Scenario->ScenarioId,
    Scenario->ExpectedStatus,
    OperationStatus,
    Scenario->ExpectMutation ? "NOT_APPLICABLE" : "VERIFIED"
    );
  Status = EFI_SUCCESS;

Exit:
  FreeVariableValue (&After);
  FreeVariableValue (&Before);
  return Status;
}
