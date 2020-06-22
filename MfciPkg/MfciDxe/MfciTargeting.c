/** @file
  Verifies that the targeting information in a MFCI Policy
  matches the platform targeting 

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <MfciPolicyType.h>
#include <MfciPolicyFields.h>

#include <Library/BaseLib.h>                             // CpuDeadLoop()
#include <Library/DebugLib.h>                            // DEBUG tracing
#include <Library/BaseMemoryLib.h>                       // CopyGuid()
#include <Library/MemoryAllocationLib.h>                 // Memory allocation and freeing
#include <Library/UefiRuntimeServicesTableLib.h>         // gRT
#include <Library/ResetUtilityLib.h>                     // ResetPlatformSpecificGuid()
#include <Library/MfciPolicyParsingLib.h>                  // Extracting and validating policy blobs

#include "MfciDxe.h"
#include "MfciVariables.h"


STATIC
EFI_STATUS
GetOemField (
  CONST CHAR16            *MfciPolicyFieldName,
  VOID                    *Data,
  UINTN                   *DataSize
)
{
  EFI_STATUS Status;

  if ((DataSize == NULL) || (Data == NULL) || (MfciPolicyFieldName == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  if (*DataSize == 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = gRT->GetVariable((CHAR16*)MfciPolicyFieldName,
                            &MFCI_VAR_VENDOR_GUID,
                            NULL,   // We do not check variable attributes
                            DataSize,
                            Data);

  return Status;
}

STATIC
EFI_STATUS
VerifyStringFieldHelper (
  VOID                    *PolicyBlob,
  UINTN                   PolicyBlobSize,
  MFCI_POLICY_FIELD       TargetField
)
{
  EFI_STATUS  Status;

  CHAR16      *MfciPolData = NULL;
  CHAR16      ThisMfciPolData[MFCI_POLICY_FIELD_MAX_LEN];
  UINTN       DataSize = MFCI_POLICY_FIELD_MAX_LEN * sizeof(CHAR16);

  if ((PolicyBlob == NULL) ||
      (PolicyBlobSize == 0) ||
      (TargetField >= MFCI_POLICY_FIELD_COUNT)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = ExtractChar16 (PolicyBlob, PolicyBlobSize, gPolicyBlobFieldName[TargetField], &MfciPolData);
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Extracting String Field '%s' from Blob failed - %r.\n", __FUNCTION__, gPolicyBlobFieldName[TargetField], Status ));
    goto Done;
  }

  Status = GetOemField (gPolicyTargetFieldVarNames[TargetField], ThisMfciPolData, &DataSize);
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to read UEFI variable %s with return status %r\n", __FUNCTION__, gPolicyTargetFieldVarNames[TargetField], Status ));
    goto Done;
  }
  if (DataSize < (MFCI_POLICY_FIELD_MAX_LEN * sizeof(CHAR16))) { ThisMfciPolData[DataSize/sizeof(CHAR16)] = L'\0'; } // ensure NULL termination

  if (StrnCmp (ThisMfciPolData, MfciPolData, MFCI_POLICY_FIELD_MAX_LEN) != 0) {
    // String comparison failed
    DEBUG(( DEBUG_ERROR, "%a - Target Field '%s' policy target '%s' does not match system value '%s'\n", __FUNCTION__, gPolicyBlobFieldName[TargetField], MfciPolData, ThisMfciPolData ));
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

Done:
  if (MfciPolData != NULL) {
    FreePool (MfciPolData);
    MfciPolData = NULL;
  }
  return Status;
}

EFI_STATUS
EFIAPI
VerifyTargeting (
  VOID                              *PolicyBlob,
  UINTN                             PolicyBlobSize,
  UINT64                            ExpectedNonce,
  MFCI_POLICY_TYPE                  *ExtractedPolicy
)
{
  EFI_STATUS                        Status;
  UINT64                            BlobNonce;

  DEBUG(( DEBUG_INFO, "MfciDxe: %a() - Enter\n", __FUNCTION__ ));

  if ((PolicyBlob == NULL) ||
      (ExtractedPolicy == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  Status = EFI_SUCCESS;

  // Steps 1 - 5: Verify Manufacturer, product name, serial number, OEM_01, & OEM_02
  for (UINTN fieldIndex = MFCI_POLICY_TARGET_MANUFACTURER;
       fieldIndex < MFCI_POLICY_TARGET_NONCE;
       fieldIndex++)
  {
    Status = VerifyStringFieldHelper (PolicyBlob, PolicyBlobSize, fieldIndex);
    if (EFI_ERROR(Status)) { goto Done; }  // helper function above takes care of debug logging
  }

  // Step 6: Verify nonce
  Status = ExtractUint64 (PolicyBlob, PolicyBlobSize, gPolicyBlobFieldName[MFCI_POLICY_TARGET_NONCE], &BlobNonce);
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to extract nonce from policy blob with return status %r\n", __FUNCTION__, gPolicyBlobFieldName[MFCI_POLICY_TARGET_NONCE], Status ));
    goto Done;
  }
  if (BlobNonce != ExpectedNonce) {
    DEBUG(( DEBUG_ERROR, "%a - Blob nonce does not match platform's nonce, the blob is not fresh.\n", __FUNCTION__));
    Status = EFI_SECURITY_VIOLATION;
    goto Done;
  }

  // Step 7: Extract policy
  Status = ExtractUint64 (PolicyBlob, PolicyBlobSize, gPolicyBlobFieldName[MFCI_POLICY_FIELD_UEFI_POLICY], ExtractedPolicy);
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to extract the MFCI Policy from the binary blob with return status %r\n", __FUNCTION__, Status ));
    goto Done;
  }

Done:
  return Status;
}
