/** @file
  This module handles re-authentication of existing MFCI
  Policies and ingestion of new policies.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <MfciPolicyType.h>
#include <MfciVariables.h>
#include <MfciPolicyFields.h>
#include <Protocol/MfciProtocol.h>

#include <Protocol/VariablePolicy.h>
#include <Guid/MuVarPolicyFoundationDxe.h>

#include <Library/BaseLib.h>                             // CpuDeadLoop()
#include <Library/HobLib.h>                              // GetFirstGuidHob()
#include <Library/DebugLib.h>                            // DEBUG tracing
#include <Library/BaseMemoryLib.h>                       // CopyGuid()
#include <Library/MemoryAllocationLib.h>                 // Memory allocation and freeing
#include <Library/UefiBootServicesTableLib.h>            // gBS
#include <Library/UefiRuntimeServicesTableLib.h>         // gRT
#include <Library/ResetUtilityLib.h>                     // ResetPlatformSpecificGuid()
#include <Library/MuVariablePolicyHelperLib.h>           // NotifyMfciPolicyChange()
#include <Library/RngLib.h>                              // GetRandomNumber64()
#include <Library/MfciPolicyParsingLib.h>                // ValidateBlob()

#include "MfciDxe.h"

MFCI_POLICY_TYPE                  mCurrentPolicy;

STATIC
EFI_STATUS
CleanCurrentVariables (
  VOID
)
{
  EFI_STATUS Status;
  EFI_STATUS ReturnStatus = EFI_SUCCESS;
  UINT64     InvalidNonce = 0;

  Status = gRT->SetVariable (CURRENT_MFCI_NONCE_VARIABLE_NAME,
                             &MFCI_VAR_VENDOR_GUID,
                             MFCI_POLICY_VARIABLE_ATTR,
                             sizeof(InvalidNonce),
                             &InvalidNonce);
  if (Status != EFI_SUCCESS) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to set %s, returned %r\n", __FUNCTION__, CURRENT_MFCI_NONCE_VARIABLE_NAME, Status));
    ReturnStatus = Status;
  }

  Status = gRT->SetVariable (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME,
                             &MFCI_VAR_VENDOR_GUID,
                             MFCI_POLICY_VARIABLE_ATTR,
                             0,
                             NULL);
  if ((Status != EFI_NOT_FOUND) && (Status != EFI_SUCCESS)) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to delete %s, returned %r\n", __FUNCTION__, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, Status));
    ReturnStatus = Status;
  }

  Status = gRT->SetVariable (CURRENT_MFCI_POLICY_VARIABLE_NAME,
                             &MFCI_VAR_VENDOR_GUID,
                             MFCI_POLICY_VARIABLE_ATTR,
                             0,
                             NULL);
  if ((Status != EFI_NOT_FOUND) && (Status != EFI_SUCCESS)) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to delete %s, returned %r\n", __FUNCTION__, CURRENT_MFCI_POLICY_VARIABLE_NAME, Status));
    ReturnStatus = Status;
  }

  return ReturnStatus;
}

STATIC
EFI_STATUS
CleanTargetVariables (
  VOID
)
{
  EFI_STATUS Status;
  EFI_STATUS ReturnStatus = EFI_SUCCESS;
  BOOLEAN RngResult;
  UINT64 TargetNonce;

  TargetNonce = MFCI_POLICY_INVALID_NONCE;
  RngResult = GetRandomNumber64 (&TargetNonce);
  if (!RngResult) {
    DEBUG(( DEBUG_ERROR, "%a - Generating random number 64 failed.\n", __FUNCTION__ ));
    ASSERT(FALSE);
    TargetNonce = MFCI_POLICY_INVALID_NONCE;
    ReturnStatus = EFI_DEVICE_ERROR;
  }

  Status = gRT->SetVariable (NEXT_MFCI_NONCE_VARIABLE_NAME,
                             &MFCI_VAR_VENDOR_GUID,
                             MFCI_POLICY_VARIABLE_ATTR,
                             sizeof(TargetNonce),
                             &TargetNonce);
  if (Status != EFI_SUCCESS) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to set TargetNonce %d, returned %r\n", __FUNCTION__, TargetNonce, Status));
    ReturnStatus = Status;
  }

  Status = gRT->SetVariable (NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME,
                             &MFCI_VAR_VENDOR_GUID,
                             MFCI_POLICY_VARIABLE_ATTR,
                             0,
                             NULL);
  if ((Status != EFI_NOT_FOUND) && (Status != EFI_SUCCESS)) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to delete %s, returned %r\n", __FUNCTION__, NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME, Status));
    ReturnStatus = Status;
  }

  return ReturnStatus;
}

STATIC
EFI_STATUS
InternalCleanupCurrentPolicy (
  VOID
)
{
  EFI_STATUS Status;

  if (mCurrentPolicy != CUSTOMER_STATE) {
    // Call the callbacks
    NotifyMfciPolicyChange (CUSTOMER_STATE);
  }

  // Delete current blob, current policy, set invalid current nonce
  Status = CleanCurrentVariables ();

  if (mCurrentPolicy != CUSTOMER_STATE) {
    ResetSystemWithSubtype(EfiResetCold, &gMfciPolicyChangeResetGuid);
    // Reset System should not return, deadloop if it does
    CpuDeadLoop();
  }

  // Otherwise, someone else might be interested in it..
  return Status;
}

STATIC
EFI_STATUS
InternalCleanupTargetPolicy (
  VOID
)
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_INFO, "%a() Entry\n", __FUNCTION__));

  if (mCurrentPolicy != CUSTOMER_STATE) {
    // Call the callbacks
    NotifyMfciPolicyChange (CUSTOMER_STATE);
  }

  // Delete target blob, set new random target nonce
  Status = CleanTargetVariables ();

  // Delete current blob, current policy, set invalid current nonce
  Status = CleanCurrentVariables ();

  if (mCurrentPolicy != CUSTOMER_STATE) {
    ResetSystemWithSubtype(EfiResetCold, &gMfciPolicyChangeResetGuid);
    CpuDeadLoop();
  }

  // Otherwise, someone else might be interested in it..
  return Status;
}


/**
  A helper function to lock all protected variables that control
  MFCI Policy

  @retval     EFI_SUCCESS             Locked all variables.
  @retval     EFI_SECURITY_VIOLATION  Failed to lock one of the required variables.

**/
STATIC
EFI_STATUS
LockPolicyVariables (
  VOID
  )
{
  EFI_STATUS                    Status;
  POLICY_LOCK_VAR               LockVar;

  DEBUG(( DEBUG_INFO, "MfciDxe: %a() - Enter\n", __FUNCTION__ ));

  //
  // Lock any protected variables.
  // Creating this variable will cause the write-protection to be enforced in the policy engine.
  LockVar = MFCI_LOCK_VAR_VALUE;
  Status = gRT->SetVariable (
                  MFCI_LOCK_VAR_NAME,
                  &gMuVarPolicyWriteOnceStateVarGuid,
                  WRITE_ONCE_STATE_VAR_ATTR,
                  sizeof (LockVar),
                  &LockVar
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a] - Failed to lock MFCI Policy variables! %r\n",
      __FUNCTION__,
      Status));
    ASSERT (FALSE);
    Status = EFI_SECURITY_VIOLATION;
  }
  else {
    DEBUG((DEBUG_VERBOSE, "Successfully set MFCI Policy Lock"));
  }

  return Status;
} // LockPolicyVariables()


/*
  InternalTransitionRoutine()

  Perform the policy state transition.  This entails:

    a. Notify registered policy change callbacks

       NOTE: It is the responsibility of the callbacks to perform all "actions"
       specified in the TargetPolicy, either synchronously here,
       or asynchronously (e.g. pended to the next boot using unspecified mechanism)

       NOTE: It is possible to have spurious notifications if there are errors
       during state transition

    b. Set "Current" blob & nonce variables to the new Target values
       Note that these are re-authenticated every boot in DXE

    c. Set "Current" state variable to the new Target value
       Note that this is implicitly trusted in PEI

    d. Clean "Target" state variables including rolling the TargetNonce

    e. Reset so that PEI can boot in new "current" state
       ResetType: EfiResetCold
       SubType: gMfciPolicyChangeResetGuid

  @param[in]  TargetPolicy    New policy to be installed
  @param[in]  TargetNonce     New nonce to be installed (as current)
  @param[in]  TargetBlob      Pointer to the new policy blob to be installed
  @param[in]  TargetBlobSize  Size of TargetBlob in bytes
*/
STATIC
VOID
InternalTransitionRoutine (
  MFCI_POLICY_TYPE                TargetPolicy,
  UINT64                          TargetNonce,
  VOID                            *TargetBlob,
  UINTN                           TargetBlobSize
)
{
  EFI_STATUS Status;

  DEBUG(( DEBUG_INFO, "MfciDxe: %a() - Enter\n", __FUNCTION__ ));

  // Step a: Call the callbacks
  NotifyMfciPolicyChange (TargetPolicy);
  TargetPolicy &= ~MFCI_POLICY_VALUE_ACTIONS_MASK; // clear the action bits

  if (TargetPolicy == CUSTOMER_STATE) {
    goto Done; // no need to transition blobs or nonces, just refresh TargetNonce which happens in Done:
  }

  // Step b: copy target stuff to current stuff
  Status =  gRT->SetVariable (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME,
                              &MFCI_VAR_VENDOR_GUID,
                              MFCI_POLICY_VARIABLE_ATTR,
                              TargetBlobSize,
                              TargetBlob);
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to set %s, returned %r\n", __FUNCTION__, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, Status));
    goto Done;
  }

  Status =  gRT->SetVariable (CURRENT_MFCI_NONCE_VARIABLE_NAME,
                              &MFCI_VAR_VENDOR_GUID,
                              MFCI_POLICY_VARIABLE_ATTR,
                              sizeof(TargetNonce),
                              &TargetNonce);
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to set %s, returned %r\n", __FUNCTION__, CURRENT_MFCI_NONCE_VARIABLE_NAME, Status));
    goto Done;
  }

  // Step c: set current policy to target policy
  Status =  gRT->SetVariable (CURRENT_MFCI_POLICY_VARIABLE_NAME,
                              &MFCI_VAR_VENDOR_GUID,
                              MFCI_POLICY_VARIABLE_ATTR,
                              sizeof(TargetPolicy),
                              &TargetPolicy);
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to set %s, returned %r\n", __FUNCTION__, CURRENT_MFCI_POLICY_VARIABLE_NAME, Status));
    goto Done;
  }

Done:
  // Step d: Delete target blob, set new random target nonce
  CleanTargetVariables();

  // Step e: reboot!
  ResetSystemWithSubtype(EfiResetCold, &gMfciPolicyChangeResetGuid);
  CpuDeadLoop();

  return;
} // InternalTransitionRoutine()

STATIC
EFI_STATUS
VarPolicyCallback (
  IN  EFI_EVENT             Event,
  IN  VOID                  *Context
  )
{
  VARIABLE_POLICY_PROTOCOL            *VariablePolicy = NULL;
  EFI_STATUS                          Status;

  DEBUG(( DEBUG_INFO, "MfciDxe: %a() - Enter\n", __FUNCTION__ ));

  Status = gBS->LocateProtocol( &gVariablePolicyProtocolGuid, NULL, &VariablePolicy );
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Locating Variable Policy failed - %r\n", __FUNCTION__, Status ));
    goto Done;
  }

  // Register policies to protect the protected state variables
  Status = RegisterVarStateVariablePolicy (VariablePolicy,
                                        &MFCI_VAR_VENDOR_GUID,
                                        CURRENT_MFCI_POLICY_VARIABLE_NAME,
                                        sizeof(UINT64),
                                        sizeof(UINT64),
                                        MFCI_POLICY_VARIABLE_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        &gMuVarPolicyWriteOnceStateVarGuid,
                                        MFCI_LOCK_VAR_NAME,
                                        MFCI_LOCK_VAR_VALUE);
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Registering Variable Policy for Current Policy failed - %r\n", __FUNCTION__, Status ));
    goto Done;
  }

  Status = RegisterVarStateVariablePolicy (VariablePolicy,
                                        &MFCI_VAR_VENDOR_GUID,
                                        NEXT_MFCI_NONCE_VARIABLE_NAME,
                                        sizeof(UINT64),
                                        sizeof(UINT64),
                                        MFCI_POLICY_VARIABLE_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        &gMuVarPolicyWriteOnceStateVarGuid,
                                        MFCI_LOCK_VAR_NAME,
                                        MFCI_LOCK_VAR_VALUE);
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Registering Variable Policy for Target Nonce failed - %r\n", __FUNCTION__, Status ));
    goto Done;
  }

  Status = RegisterVarStateVariablePolicy (VariablePolicy,
                                        &MFCI_VAR_VENDOR_GUID,
                                        CURRENT_MFCI_NONCE_VARIABLE_NAME,
                                        sizeof(UINT64),
                                        sizeof(UINT64),
                                        MFCI_POLICY_VARIABLE_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        &gMuVarPolicyWriteOnceStateVarGuid,
                                        MFCI_LOCK_VAR_NAME,
                                        MFCI_LOCK_VAR_VALUE);
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Registering Variable Policy for Current Nonce failed - %r\n", __FUNCTION__, Status ));
    goto Done;
  }


  // Walk the list of OEM-supplied targeting variables
  for (UINTN fieldIndex = MFCI_POLICY_TARGET_MANUFACTURER;
       fieldIndex < TARGET_POLICY_COUNT;
       fieldIndex++)
  {
    // Register variable policy to lock the OEM-supplied targeting variables at End of DXE
    DEBUG(( DEBUG_VERBOSE, "Registering Variable Policy for %s... \n", gPolicyTargetFieldVarNames[fieldIndex]));
    Status = RegisterVarStateVariablePolicy (VariablePolicy,
                                        &MFCI_VAR_VENDOR_GUID,
                                        gPolicyTargetFieldVarNames[fieldIndex],
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        MFCI_POLICY_FIELD_MAX_LEN,
                                        MFCI_POLICY_TARGETING_VARIABLE_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        &gMuVarPolicyDxePhaseGuid,
                                        END_OF_DXE_INDICATOR_VAR_NAME,
                                        PHASE_INDICATOR_SET);
    if (EFI_ERROR(Status)) {
      DEBUG(( DEBUG_ERROR, "%a - Registering Variable Policy for Target Variable %s failed - %r\n", __FUNCTION__, gPolicyTargetFieldVarNames[fieldIndex], Status ));
      goto Done;
    }
  }

Done:
  DEBUG(( DEBUG_VERBOSE, "MfciDxe: %a() - Exit\n", __FUNCTION__ ));
  return Status;
}


/**
 * Executes after DXE modules get opportunity to publish the OEM, model, SN, ... variables that
 * are used for per-device targeting of policies.
 * Always re-authenticate any policy that is currently installed.  Then check if a new policy
 * is pending installation, and if so authenticate and install it.  If policy changes, notify
 * registered callbacks, clear "action" bits (leaving only the "state" bits), update variables,
 * and reset the system.  Always check sanity of variables, re-initialize them if missing or state
 * is torn (due to an error during processing on prior boot).  If not changing policy, lock
 * protected variables (nonces and the bare policy variable that is used by PEI because we don't
 * want to perform crypto there, then continue boot).
 *
 * @param Event
 *   Unused
 * @param Context
 *   Unused
 */
STATIC
VOID
VerifyPolicyAndChange (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  EFI_STATUS                        Status;

  BOOLEAN                           RngResult;
  UINT32                            VariableAttr = 0;
  UINTN                             DataSize;
  VOID                              *CurrentBlob = NULL;
  UINTN                             CurrentBlobSize;
  UINT64                            CurrentNonce;
  VOID                              *TargetBlob = NULL;
  UINTN                             TargetBlobSize;
  UINT64                            TargetNonce;
  MFCI_POLICY_TYPE                  BlobPolicy;
  UINT8                             *PublicKeyDataXdr;
  UINT8                             *PublicKeyDataXdrEnd;
  UINT8                             *PublicKeyData;
  UINTN                             PublicKeyDataLength;
  CHAR8                             *RequiredEKUs;

  DEBUG(( DEBUG_INFO, "MfciDxe: %a() - Enter\n", __FUNCTION__ ));

  // Initialize configuration from PCDs, consider moving elsewhere and only initializing if there is a blob to validate
  RequiredEKUs = (CHAR8*)FixedPcdGetPtr(PcdMfciPkcs7RequiredLeafEKU);

  // below is inspired/borrowed from FmpDxe.c
  PublicKeyDataXdr = FixedPcdGetPtr(PcdMfciPkcs7CertBufferXdr);
  PublicKeyDataXdrEnd = PublicKeyDataXdr + FixedPcdGetSize(PcdMfciPkcs7CertBufferXdr);

  if (PublicKeyDataXdr == NULL || ((PublicKeyDataXdr + sizeof (UINT32)) > PublicKeyDataXdrEnd) ) {
    DEBUG ((DEBUG_ERROR, "Pcd PcdMfciPkcs7CertBufferXdr NULL or invalid size\n"));
    Status = EFI_ABORTED;
    goto Exit;
  }

  // Read key length stored in big-endian format
  //
  PublicKeyDataLength = SwapBytes32 (*(UINT32 *)(PublicKeyDataXdr));
  //
  // Point to the start of the key data
  //
  PublicKeyData = PublicKeyDataXdr + sizeof (UINT32);

  // Only 1 certificate is supported
  // Length + sizeof(CHAR8) because there is a terminating NULL byte
  if (PublicKeyData + PublicKeyDataLength + sizeof(CHAR8) != PublicKeyDataXdrEnd) {
    DEBUG ((DEBUG_ERROR, "PcdMfciPkcs7CertBufferXdr size mismatch: PublicKeyData(0x%x) PublicKeyDataLength(0x%x) PublicKeyDataXdrEnd(0x%x)", PublicKeyData, PublicKeyDataLength, PublicKeyDataXdrEnd));
    Status = EFI_ABORTED;
    goto Exit;
  }
  // above is inspired/borrowed from FmpDxe.c

  // Step 1: Check target nonce exist
  TargetNonce = MFCI_POLICY_INVALID_NONCE;
  DataSize = sizeof (TargetNonce);
  Status = gRT->GetVariable (NEXT_MFCI_NONCE_VARIABLE_NAME,
                             &MFCI_VAR_VENDOR_GUID,
                             &VariableAttr,
                             &DataSize,
                             &TargetNonce);
  if (EFI_ERROR(Status) ||
      DataSize != sizeof (TargetNonce) ||
      VariableAttr != MFCI_POLICY_VARIABLE_ATTR ||
      TargetNonce == MFCI_POLICY_INVALID_NONCE) {

    DEBUG(( DEBUG_INFO, "%a - Refreshing Target Nonce - DataSize(%d) VariableAttr(%x) TargetNonce(%d) Status(%r)\n", __FUNCTION__, DataSize, VariableAttr, TargetNonce, Status ));

    // Create a new one if we do not like it..
    TargetNonce = MFCI_POLICY_INVALID_NONCE;
    RngResult = GetRandomNumber64 (&TargetNonce);
    if (!RngResult) {
      DEBUG(( DEBUG_ERROR, "%a - Generating random number 64 failed.\n", __FUNCTION__ ));
      ASSERT(FALSE);
      TargetNonce = MFCI_POLICY_INVALID_NONCE;
    }

    Status = gRT->SetVariable (NEXT_MFCI_NONCE_VARIABLE_NAME,
                               &MFCI_VAR_VENDOR_GUID,
                               MFCI_POLICY_VARIABLE_ATTR,
                               sizeof (TargetNonce),
                               &TargetNonce);
    if (EFI_ERROR(Status)) {
      DEBUG(( DEBUG_ERROR, "%a - Set TARGET nonce failed! - %r\n", __FUNCTION__, Status ));
      ASSERT(FALSE);
      goto Exit;
    }
  }

  // Step 2: Check current policy related variables
  // Step 2.1: grab current blob
  DEBUG(( DEBUG_INFO, "%a - Step 2: Check current policy related variables.\n", __FUNCTION__ ));
  // Check for presence of current MFCI policy blob.  It is either:
  // i. not found
  // ii. other error
  // iii. found with correct attributes
  CurrentBlobSize = 0;
  Status = gRT->GetVariable (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME,
                             &MFCI_VAR_VENDOR_GUID,
                             &VariableAttr,
                             &CurrentBlobSize,
                             CurrentBlob);
  // i. not found
  if (Status == EFI_NOT_FOUND) {
    DEBUG(( DEBUG_INFO, "%a - Get current MFCI Policy blob - %r\n", __FUNCTION__, Status ));

    // If there is no current blob found, make sure all current stuff looks good
    Status = InternalCleanupCurrentPolicy();

    // If we return check result and decide whether we should go for target side
    if (EFI_ERROR(Status)) {
      DEBUG(( DEBUG_ERROR, "%a - Clear other current variables returned - %r\n", __FUNCTION__, Status ));
      goto Exit;
    }
    else {
      DEBUG(( DEBUG_INFO, "%a - Clear other current variables returned, proceeding to TARGET step.\n", __FUNCTION__ ));
      goto VerifyTarget;
    }
  }
  // ii. other error
  else if ((Status != EFI_BUFFER_TOO_SMALL) ||
           (VariableAttr != MFCI_POLICY_VARIABLE_ATTR)) {
    // Something is wrong, bail here..
    DEBUG(( DEBUG_ERROR, "%a - Initial get current MFCI Policy blob failed - %r with attribute %08x\n", __FUNCTION__, Status, VariableAttr ));
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  // iii. found with correct attributes

  CurrentBlob = AllocatePool (CurrentBlobSize);
  if (CurrentBlob == NULL) {
    DEBUG(( DEBUG_ERROR, "%a - Allocating memory for current MFCI Policy blob of size %d failed.\n", __FUNCTION__, CurrentBlobSize ));
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  Status = gRT->GetVariable (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME,
                             &MFCI_VAR_VENDOR_GUID,
                             &VariableAttr,
                             &CurrentBlobSize,
                             CurrentBlob);
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_WARN, "%a - Second get current MFCI Policy blob failed - %r\n", __FUNCTION__, Status ));

    // If there is error reading current blob, make sure clearing all current staff looks good
    Status = InternalCleanupCurrentPolicy();

    // If we return check result and decide whether we should go for target side
    if (EFI_ERROR(Status)) {
      DEBUG(( DEBUG_ERROR, "%a - Clear ALL current variables returned - %r\n", __FUNCTION__, Status ));
      goto Exit;
    }
    else {
      DEBUG(( DEBUG_INFO, "%a - Clear ALL current variables returned, proceeding to TARGET step.\n", __FUNCTION__ ));
      goto VerifyTarget;
    }
  }

  // Step 2.2: grab current nonce
  DataSize = sizeof (CurrentNonce);
  Status = gRT->GetVariable (CURRENT_MFCI_NONCE_VARIABLE_NAME,
                             &MFCI_VAR_VENDOR_GUID,
                             &VariableAttr,
                             &DataSize,
                             &CurrentNonce);
  if (EFI_ERROR(Status) ||
      DataSize != sizeof (CurrentNonce) ||
      VariableAttr != MFCI_POLICY_VARIABLE_ATTR) {
    // Something we do not like about this.. bail here
    DEBUG(( DEBUG_ERROR, "%a - Reading current nonce failed - %r with size: %d and attribute: 0x%08x.\n", __FUNCTION__, Status, DataSize, VariableAttr ));
    goto Exit;
  }

  // Step 2.3: validate current blob signature

  Status = ValidateBlob (CurrentBlob, CurrentBlobSize, PublicKeyData, PublicKeyDataLength, RequiredEKUs);
  if (EFI_ERROR(Status)) {

    DEBUG(( DEBUG_ERROR, "%a - validate current blob failed - %r.\n", __FUNCTION__, Status ));

    // If there is no current blob found, make sure all current staff looks good
    Status = InternalCleanupCurrentPolicy();

    // If we return check result and decide whether we should go for target side
    if (EFI_ERROR(Status)) {
      DEBUG(( DEBUG_ERROR, "%a - Clean invalid current policy failed - %r.\n", __FUNCTION__, Status ));
      goto Exit;
    }
    else {
      DEBUG(( DEBUG_INFO, "%a - Clean invalid current policy returned, proceeding to TARGET step.\n", __FUNCTION__ ));
      goto VerifyTarget;
    }
  }

  // Step 2.4: verify targetting is for this machine
  Status = VerifyTargeting (CurrentBlob,
                             CurrentBlobSize,
                             CurrentNonce,
                             &BlobPolicy);

  BlobPolicy &= ~MFCI_POLICY_VALUE_ACTIONS_MASK; // clear the action bits as they would have been processed upon installation

  if (EFI_ERROR(Status) ||
      (BlobPolicy != mCurrentPolicy)) {
    // TODO: Telemetry here
    DEBUG(( DEBUG_ERROR, "%a Verify targeting return error - %r. mCurrentPolicy: 0x%16x, BlobPolicy: 0x%16x.\n", __FUNCTION__, Status, mCurrentPolicy, BlobPolicy ));

    // If targetting is incorrect, or current policy and extracted has mismatch, fall back
    Status = InternalCleanupCurrentPolicy();

    // If we return check result and decide whether we should go for target side
    if (EFI_ERROR(Status)) {
      DEBUG(( DEBUG_ERROR, "%a - Clean invalid targeting current policy failed - %r.\n", __FUNCTION__, Status ));
      goto Exit;
    }
    else {
      DEBUG(( DEBUG_INFO, "%a - Clean invalid targeting current policy returned, proceeding to TARGET step.\n", __FUNCTION__ ));
      goto VerifyTarget;
    }
  }

VerifyTarget:

  // Step 3: If we got here, check target policy related variables
  DEBUG(( DEBUG_ERROR, "%a - Verify targeting step!\n", __FUNCTION__ ));

  // Step 3.1: grab target blob
  TargetBlobSize = 0;
  Status = gRT->GetVariable (NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME,
                             &MFCI_VAR_VENDOR_GUID,
                             &VariableAttr,
                             &TargetBlobSize,
                             TargetBlob);
  if (Status == EFI_NOT_FOUND) {
    // If there is no target blob found, we are done!!!
    DEBUG(( DEBUG_INFO, "%a - No target blob found, bail here.\n", __FUNCTION__ ));
    Status = EFI_SUCCESS;
    goto Exit;
  }
  else if ((Status != EFI_BUFFER_TOO_SMALL) ||
           (VariableAttr != MFCI_POLICY_VARIABLE_ATTR)) {
    // Something is wrong, bail here..
    DEBUG(( DEBUG_ERROR, "%a - Failed to read target blob - %r with attribute 0x%08x.\n", __FUNCTION__, Status, VariableAttr ));
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  TargetBlob = AllocatePool (TargetBlobSize);
  DEBUG(( DEBUG_VERBOSE, "\n%a - %d TargetBlobSize(%d) TargetBlob(%p)\n", __FUNCTION__, __LINE__, TargetBlobSize, TargetBlob ));
  if (TargetBlob == NULL) {
    DEBUG(( DEBUG_ERROR, "%a - Allocating memory for target MFCI Policy blob of size %d failed.\n", __FUNCTION__, TargetBlobSize ));
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  Status = gRT->GetVariable (NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME,
                             &MFCI_VAR_VENDOR_GUID,
                             &VariableAttr,
                             &TargetBlobSize,
                             TargetBlob);
  if (EFI_ERROR(Status)) {
    // There is something wrong here...
    // Try to tear down everything and bail
    DEBUG(( DEBUG_ERROR, "%a - Failed to read target blob - %r.\n", __FUNCTION__, Status ));

    Status = InternalCleanupTargetPolicy();

    DEBUG(( DEBUG_WARN, "%a - Clean up bad target variable returned - %r.\n", __FUNCTION__, Status ));
    goto Exit;
  }

  // Step 3.2: grab target nonce (which is TargetNonce from step 1)
  // Do nothing.

  // Step 3.3: validate target blob signature
  Status = ValidateBlob (TargetBlob, TargetBlobSize, PublicKeyData, PublicKeyDataLength, RequiredEKUs);
  if (EFI_ERROR(Status)) {
    // In effort of being fail safe, we let it fail here
    DEBUG(( DEBUG_ERROR, "%a - Target blob validation failed - %r.\n", __FUNCTION__, Status ));

    Status = InternalCleanupTargetPolicy();

    DEBUG(( DEBUG_WARN, "%a - Clean up invalid target variable returned - %r.\n", __FUNCTION__, Status ));
    goto Exit;
  }

  DEBUG(( DEBUG_INFO, "\n%a - %d\n", __FUNCTION__, __LINE__ ));
  // Step 3.4: verify targetting is for this machine
    Status = VerifyTargeting (TargetBlob,
                               TargetBlobSize,
                               TargetNonce,
                               &BlobPolicy);

  DEBUG(( DEBUG_INFO, "\n%a - %d\n", __FUNCTION__, __LINE__ ));
  if (EFI_ERROR(Status)) {
    // If target is wrong, we fail, back to safe zone
    DEBUG(( DEBUG_ERROR, "%a - Target blob validation failed - %r.\n", __FUNCTION__, Status ));

    Status = InternalCleanupTargetPolicy();

    DEBUG(( DEBUG_WARN, "%a - Clean up mistargeted target variable returned - %r.\n", __FUNCTION__, Status ));
    goto Exit;
  }

  DEBUG(( DEBUG_INFO, "\n%a - %d\n", __FUNCTION__, __LINE__ ));
  // Step 4: If we are still here, probably it is time to do transition
  // This routine will not return
  InternalTransitionRoutine (BlobPolicy,
                             TargetNonce,
                             TargetBlob,
                             TargetBlobSize);

  // Should not be here
  ASSERT (FALSE);

Exit:

  // Prevent any memory leakage
  if (CurrentBlob != NULL) {
    FreePool (CurrentBlob);
    CurrentBlob = NULL;
  }
  if (TargetBlob != NULL) {
    FreePool (TargetBlob);
    TargetBlob = NULL;
  }

  EFI_STATUS Status2 = LockPolicyVariables ();

  if (EFI_ERROR( Status ) || EFI_ERROR( Status2 )) {
    DEBUG(( DEBUG_ERROR,
            "%a - An error occurred while processing MFCI Policy - %r, %r!!\n",
            __FUNCTION__, Status, Status2 ));

    // TODO uncomment the below after debugging is complete
    // InternalCleanupCurrentPolicy ();
    // TODO: Log telemetry for any errors that occur.
  }

  DEBUG(( DEBUG_VERBOSE, "MfciDxe: %a() - Exit\n", __FUNCTION__ ));
}


/**

Routine Description:

  Driver Entrypoint that...
  Reads data for the policy flavor for the current boot from a HOB
  Installs the protocol for getting current policy & policy change notifications
  Registers variable policy to lock protected variables
  Registers a start of BDS callback that verifies policies & processes changes

Arguments:

  @param[in]  ImageHandle -- Handle to this image.
  @param[in]  SystemTable -- Pointer to the system table.

  @retval EFI_STATUS
**/
EFI_STATUS
EFIAPI
MfciDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE* SystemTable
  )
{
  EFI_STATUS                        Status;
  EFI_EVENT                         VarPolicyEvent;
  EFI_EVENT                         MfciPolicyCheckEvent = NULL;
  MFCI_POLICY_TYPE                  *PolicyPtr;
  UINTN                             EntrySize;
  VOID                              *GuidHob;
  VOID                              *NotUsed;

  DEBUG(( DEBUG_INFO, "MfciDxe: %a() - Enter\n", __FUNCTION__ ));

  GuidHob = GetFirstGuidHob(&gMfciHobGuid);
  if (GuidHob == NULL) {
    DEBUG(( DEBUG_ERROR, "%a() - MFCI Policy HOB not found!\n", __FUNCTION__ ));
    ASSERT(FALSE);
    goto Exit;
  }

  PolicyPtr = GET_GUID_HOB_DATA(GuidHob);
  EntrySize = GET_GUID_HOB_DATA_SIZE(GuidHob);

  if ((PolicyPtr == NULL) || (EntrySize != sizeof(MFCI_POLICY_TYPE))) {
    DEBUG(( DEBUG_ERROR, "%a() - MFCI Policy HOB malformed, PolicyPtr(%p) , EntrySize(%x)\n", __FUNCTION__, PolicyPtr, EntrySize ));
    goto Exit;
  }

  mCurrentPolicy = *PolicyPtr;

  Status = InitPublicInterface ();
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - InitPublicInterface failed returning %r\n", __FUNCTION__, Status ));
    goto Exit;
  }

  //
  // Before working on the variables themselves, we should make sure that the protection policies are put in place.
  Status = gBS->LocateProtocol( &gVariablePolicyProtocolGuid, NULL, &NotUsed );
  if (EFI_ERROR( Status )) {
    DEBUG(( DEBUG_INFO, "%a - Failed to locate VariablePolicy protocol with status %r, will register protocol notification\n", __FUNCTION__, Status ));

    Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL, TPL_CALLBACK, VarPolicyCallback, NULL, &VarPolicyEvent);
    if (EFI_ERROR(Status)) {
      DEBUG(( DEBUG_ERROR, "%a - CreateEvent failed returning %r\n", __FUNCTION__, Status ));
      goto Exit;
    }

    Status = gBS->RegisterProtocolNotify (&gVariablePolicyProtocolGuid, VarPolicyEvent, (VOID **) &NotUsed);
    if (EFI_ERROR(Status)) {
      DEBUG(( DEBUG_ERROR, "%a - RegisterProtocolNotify failed returning %r\n", __FUNCTION__, Status ));
      goto Exit;
    }
  }
  else {
    Status = VarPolicyCallback (VarPolicyEvent, NULL);
    if (EFI_ERROR(Status)) {
      DEBUG(( DEBUG_ERROR, "%a - VarPolicyCallback failed returning %r\n", __FUNCTION__, Status ));
      goto Exit;
    }
  }

  Status = InitSecureBootListener();
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Initializing Secure Boot Callback failed! %r\n", __FUNCTION__, Status ));
    goto Exit;
  }

  Status = InitTpmListener();
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Initializing Tpm Callback failed! %r\n", __FUNCTION__, Status ));
    goto Exit;
  }

  //
  // This StartOfBds event is before EndOfDxe.
  // This allows us to notify all consumers *before* any of the security
  // locks fall into place.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  VerifyPolicyAndChange,
                  ImageHandle,
                  &gMsStartOfBdsNotifyGuid,
                  &MfciPolicyCheckEvent
                  );
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Registering Start of BDS failed!!! %r\n", __FUNCTION__, Status ));
    goto Exit;
  }

Exit:

  DEBUG(( DEBUG_VERBOSE, "MfciDxe: %a() - Exit\n", __FUNCTION__ ));
  return Status;
}// MfciDxeEntry()
