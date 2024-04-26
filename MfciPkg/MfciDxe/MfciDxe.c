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
#include <Library/MfciDeviceIdSupportLib.h>
#include <Library/MfciPolicyParsingLib.h>
#include <Library/MfciRetrievePolicyLib.h>

#include <Protocol/VariablePolicy.h>
#include <Guid/MuVarPolicyFoundationDxe.h>

#include <Library/BaseLib.h>                             // CpuDeadLoop()
#include <Library/DebugLib.h>                            // DEBUG tracing
#include <Library/BaseMemoryLib.h>                       // CopyGuid()
#include <Library/MemoryAllocationLib.h>                 // Memory allocation and freeing
#include <Library/UefiBootServicesTableLib.h>            // gBS
#include <Library/UefiRuntimeServicesTableLib.h>         // gRT
#include <Library/ResetUtilityLib.h>                     // ResetPlatformSpecificGuid()
#include <Library/VariablePolicyHelperLib.h>             // NotifyMfciPolicyChange()
#include <Library/RngLib.h>                              // GetRandomNumber64()

#include "MfciDxe.h"

/**
 * the following helps iterate over the functions and set the corresponding target variable names
 */

// define a structure that pairs up the function pointer with the UEFI variable name
typedef struct {
  MFCI_DEVICE_ID_FN    DeviceIdFn;
  CHAR16               *DeviceIdVarName;
} MFCI_DEVICE_ID_FN_TO_VAR_NAME_MAP;

// populate the array of structures that pair up the functions with variable names
#define MFCI_TARGET_VAR_COUNT  5

STATIC CONST MFCI_DEVICE_ID_FN_TO_VAR_NAME_MAP  gDeviceIdFnToTargetVarNameMap[MFCI_TARGET_VAR_COUNT] = {
  { MfciIdSupportGetManufacturer, MFCI_MANUFACTURER_VARIABLE_NAME },
  { MfciIdSupportGetProductName,  MFCI_PRODUCT_VARIABLE_NAME      },
  { MfciIdSupportGetSerialNumber, MFCI_SERIALNUMBER_VARIABLE_NAME },
  { MfciIdSupportGetOem1,         MFCI_OEM_01_VARIABLE_NAME       },
  { MfciIdSupportGetOem2,         MFCI_OEM_02_VARIABLE_NAME       }
};

MFCI_POLICY_TYPE  mCurrentPolicy;
BOOLEAN           mVarPolicyRegistered;

STATIC
EFI_STATUS
CleanCurrentVariables (
  VOID
  )
{
  EFI_STATUS        Status;
  EFI_STATUS        ReturnStatus   = EFI_SUCCESS;
  UINT64            InvalidNonce   = 0;
  MFCI_POLICY_TYPE  CustomerPolicy = CUSTOMER_STATE;

  Status = gRT->SetVariable (
                  CURRENT_MFCI_NONCE_VARIABLE_NAME,
                  &MFCI_VAR_VENDOR_GUID,
                  MFCI_POLICY_VARIABLE_ATTR,
                  sizeof (InvalidNonce),
                  &InvalidNonce
                  );
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to set %s to InvalidNonce, returned %r\n", __FUNCTION__, CURRENT_MFCI_NONCE_VARIABLE_NAME, Status));
    ReturnStatus = Status;
  }

  Status = gRT->SetVariable (
                  CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME,
                  &MFCI_VAR_VENDOR_GUID,
                  MFCI_POLICY_VARIABLE_ATTR,
                  0,
                  NULL
                  );
  if ((Status != EFI_NOT_FOUND) && (Status != EFI_SUCCESS)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to delete %s, returned %r\n", __FUNCTION__, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, Status));
    ReturnStatus = Status;
  }

  Status = gRT->SetVariable (
                  CURRENT_MFCI_POLICY_VARIABLE_NAME,
                  &MFCI_VAR_VENDOR_GUID,
                  MFCI_POLICY_VARIABLE_ATTR,
                  sizeof (CustomerPolicy),
                  &CustomerPolicy
                  );
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to set %s to CUSTOMER_STATE, returned %r\n", __FUNCTION__, CURRENT_MFCI_POLICY_VARIABLE_NAME, Status));
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
  EFI_STATUS  Status;
  EFI_STATUS  ReturnStatus = EFI_SUCCESS;
  BOOLEAN     RngResult;
  UINT64      TargetNonce;

  TargetNonce = MFCI_POLICY_INVALID_NONCE;
  RngResult   = GetRandomNumber64 (&TargetNonce);
  if (!RngResult) {
    DEBUG ((DEBUG_ERROR, "%a - Generating random number 64 failed.\n", __FUNCTION__));
    ASSERT (FALSE);
    TargetNonce  = MFCI_POLICY_INVALID_NONCE;
    ReturnStatus = EFI_DEVICE_ERROR;
  }

  Status = gRT->SetVariable (
                  NEXT_MFCI_NONCE_VARIABLE_NAME,
                  &MFCI_VAR_VENDOR_GUID,
                  MFCI_POLICY_VARIABLE_ATTR,
                  sizeof (TargetNonce),
                  &TargetNonce
                  );
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to set TargetNonce 0x%lx, returned %r\n", __FUNCTION__, TargetNonce, Status));
    ReturnStatus = Status;
  }

  Status = gRT->SetVariable (
                  NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME,
                  &MFCI_VAR_VENDOR_GUID,
                  MFCI_POLICY_VARIABLE_ATTR,
                  0,
                  NULL
                  );
  if ((Status != EFI_NOT_FOUND) && (Status != EFI_SUCCESS)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to delete %s, returned %r\n", __FUNCTION__, NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME, Status));
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
  EFI_STATUS  Status;

  if (mCurrentPolicy != CUSTOMER_STATE) {
    // Call the callbacks
    NotifyMfciPolicyChange (CUSTOMER_STATE);
  }

  // Delete current blob, current policy, set invalid current nonce
  Status = CleanCurrentVariables ();

  if (mCurrentPolicy != CUSTOMER_STATE) {
    ResetSystemWithSubtype (EfiResetCold, &gMfciPolicyChangeResetGuid);
    // Reset System should not return, dead loop if it does
    CpuDeadLoop ();
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
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a() Entry\n", __FUNCTION__));

  if (mCurrentPolicy != CUSTOMER_STATE) {
    // Call the callbacks
    NotifyMfciPolicyChange (CUSTOMER_STATE);
  }

  // Delete target blob, set new random target nonce
  Status = CleanTargetVariables ();

  // Delete current blob, restore current policy to CUSTOMER_STATE, set invalid current nonce
  Status = CleanCurrentVariables ();

  if (mCurrentPolicy != CUSTOMER_STATE) {
    ResetSystemWithSubtype (EfiResetCold, &gMfciPolicyChangeResetGuid);
    CpuDeadLoop ();
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
  EFI_STATUS       Status;
  EFI_STATUS       ReturnStatus = EFI_SUCCESS;
  POLICY_LOCK_VAR  LockVar;

  DEBUG ((DEBUG_INFO, "MfciDxe: %a() - Enter\n", __FUNCTION__));

  if (mVarPolicyRegistered != TRUE) {
    DEBUG ((DEBUG_ERROR, "MFCI's Variable Policy was not completely registered!  Will still attempt to lock any that were registered...\n"));
    ASSERT (FALSE);
    ReturnStatus = EFI_SECURITY_VIOLATION;
  }

  //
  // Lock all protected variables.
  // Creating this variable will cause the write-protection to be enforced in the policy engine.
  LockVar = MFCI_LOCK_VAR_VALUE;
  Status  = gRT->SetVariable (
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
      Status
      ));
    ASSERT (FALSE);
    ReturnStatus = EFI_SECURITY_VIOLATION;
  } else {
    DEBUG ((DEBUG_VERBOSE, "Successfully set MFCI Policy Lock\n"));
    ReturnStatus = Status;
  }

  return ReturnStatus;
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
  MFCI_POLICY_TYPE  TargetPolicy,
  UINT64            TargetNonce,
  VOID              *TargetBlob,
  UINTN             TargetBlobSize
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "MfciDxe: %a() - Enter\n", __FUNCTION__));

  // Step a: Call the callbacks
  NotifyMfciPolicyChange (TargetPolicy);
  TargetPolicy &= ~MFCI_POLICY_VALUE_ACTIONS_MASK; // clear the action bits

  if (TargetPolicy == CUSTOMER_STATE) {
    goto Done; // no need to transition blobs or nonces, just refresh TargetNonce which happens in Done:
  }

  // Step b: copy target stuff to current stuff
  Status =  gRT->SetVariable (
                   CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME,
                   &MFCI_VAR_VENDOR_GUID,
                   MFCI_POLICY_VARIABLE_ATTR,
                   TargetBlobSize,
                   TargetBlob
                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to set %s, returned %r\n", __FUNCTION__, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, Status));
    goto Done;
  }

  Status =  gRT->SetVariable (
                   CURRENT_MFCI_NONCE_VARIABLE_NAME,
                   &MFCI_VAR_VENDOR_GUID,
                   MFCI_POLICY_VARIABLE_ATTR,
                   sizeof (TargetNonce),
                   &TargetNonce
                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to set %s, returned %r\n", __FUNCTION__, CURRENT_MFCI_NONCE_VARIABLE_NAME, Status));
    goto Done;
  }

  // Step c: set current policy to target policy
  Status =  gRT->SetVariable (
                   CURRENT_MFCI_POLICY_VARIABLE_NAME,
                   &MFCI_VAR_VENDOR_GUID,
                   MFCI_POLICY_VARIABLE_ATTR,
                   sizeof (TargetPolicy),
                   &TargetPolicy
                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to set %s, returned %r\n", __FUNCTION__, CURRENT_MFCI_POLICY_VARIABLE_NAME, Status));
    goto Done;
  }

Done:
  // Step d: Delete target blob, set new random target nonce
  CleanTargetVariables ();

  // Step e: reboot!
  ResetSystemWithSubtype (EfiResetCold, &gMfciPolicyChangeResetGuid);
  CpuDeadLoop ();

  return;
} // InternalTransitionRoutine()

STATIC
EFI_STATUS
RegisterVarPolicies (
  )
{
  EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy = NULL;
  EFI_STATUS                      Status;

  DEBUG ((DEBUG_INFO, "MfciDxe: %a() - Enter\n", __FUNCTION__));

  Status = gBS->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid, NULL, (VOID **)&VariablePolicy);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Locating Variable Policy failed - %r\n", __FUNCTION__, Status));
    goto Done;
  }

  // Register policies to protect the protected state variables
  Status = RegisterVarStateVariablePolicy (
             VariablePolicy,
             &MFCI_VAR_VENDOR_GUID,
             CURRENT_MFCI_POLICY_VARIABLE_NAME,
             sizeof (UINT64),
             sizeof (UINT64),
             MFCI_POLICY_VARIABLE_ATTR,
             (UINT32) ~MFCI_POLICY_VARIABLE_ATTR,
             &gMuVarPolicyWriteOnceStateVarGuid,
             MFCI_LOCK_VAR_NAME,
             MFCI_LOCK_VAR_VALUE
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Registering Variable Policy for Current Policy failed - %r\n", __FUNCTION__, Status));
    goto Done;
  }

  Status = RegisterVarStateVariablePolicy (
             VariablePolicy,
             &MFCI_VAR_VENDOR_GUID,
             NEXT_MFCI_NONCE_VARIABLE_NAME,
             sizeof (UINT64),
             sizeof (UINT64),
             MFCI_POLICY_VARIABLE_ATTR,
             (UINT32) ~MFCI_POLICY_VARIABLE_ATTR,
             &gMuVarPolicyWriteOnceStateVarGuid,
             MFCI_LOCK_VAR_NAME,
             MFCI_LOCK_VAR_VALUE
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Registering Variable Policy for Target Nonce failed - %r\n", __FUNCTION__, Status));
    goto Done;
  }

  Status = RegisterVarStateVariablePolicy (
             VariablePolicy,
             &MFCI_VAR_VENDOR_GUID,
             CURRENT_MFCI_NONCE_VARIABLE_NAME,
             sizeof (UINT64),
             sizeof (UINT64),
             MFCI_POLICY_VARIABLE_ATTR,
             (UINT32) ~MFCI_POLICY_VARIABLE_ATTR,
             &gMuVarPolicyWriteOnceStateVarGuid,
             MFCI_LOCK_VAR_NAME,
             MFCI_LOCK_VAR_VALUE
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Registering Variable Policy for Current Nonce failed - %r\n", __FUNCTION__, Status));
    goto Done;
  }

  // Walk the list of OEM-supplied targeting variables to register variable policy
  // to lock the OEM-supplied targeting variables at End of DXE
  for (UINTN fieldIndex = MFCI_POLICY_TARGET_MANUFACTURER;
       fieldIndex < TARGET_POLICY_COUNT;
       fieldIndex++)
  {
    DEBUG ((DEBUG_VERBOSE, "Registering Variable Policy for %s... \n", gPolicyTargetFieldVarNames[fieldIndex]));
    Status = RegisterVarStateVariablePolicy (
               VariablePolicy,
               &MFCI_VAR_VENDOR_GUID,
               gPolicyTargetFieldVarNames[fieldIndex],
               VARIABLE_POLICY_NO_MIN_SIZE,
               MFCI_POLICY_FIELD_MAX_LEN,
               MFCI_POLICY_TARGETING_VARIABLE_ATTR,
               (UINT32) ~MFCI_POLICY_TARGETING_VARIABLE_ATTR,
               &gMuVarPolicyDxePhaseGuid,
               END_OF_DXE_INDICATOR_VAR_NAME,
               PHASE_INDICATOR_SET
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Registering Variable Policy for Target Variable %s failed - %r\n", __FUNCTION__, gPolicyTargetFieldVarNames[fieldIndex], Status));
      goto Done;
    }
  } // Walk the list of OEM-supplied targeting variables to register variable policy

  // Register NO_LOCK policies for the OS writable mailboxes
  Status = RegisterBasicVariablePolicy (
             VariablePolicy,
             &MFCI_VAR_VENDOR_GUID,
             CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME,
             VARIABLE_POLICY_NO_MIN_SIZE,
             VARIABLE_POLICY_NO_MAX_SIZE,
             MFCI_POLICY_VARIABLE_ATTR,
             (UINT32) ~MFCI_POLICY_VARIABLE_ATTR,
             VARIABLE_POLICY_TYPE_NO_LOCK
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Registering Variable Policy for the Current Policy Blob failed - %r\n", __FUNCTION__, Status));
    goto Done;
  }

  Status = RegisterBasicVariablePolicy (
             VariablePolicy,
             &MFCI_VAR_VENDOR_GUID,
             NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME,
             VARIABLE_POLICY_NO_MIN_SIZE,
             VARIABLE_POLICY_NO_MAX_SIZE,
             MFCI_POLICY_VARIABLE_ATTR,
             (UINT32) ~MFCI_POLICY_VARIABLE_ATTR,
             VARIABLE_POLICY_TYPE_NO_LOCK
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Registering Variable Policy for the Next Policy Blob failed - %r\n", __FUNCTION__, Status));
    goto Done;
  }

  // reaching here means that all variable policy was successfully registered
  mVarPolicyRegistered = TRUE;

Done:
  DEBUG ((DEBUG_VERBOSE, "MfciDxe: %a() - Exit\n", __FUNCTION__));
  return Status;
}

BOOLEAN
CheckTargetVarsExist (
  )
{
  EFI_STATUS  Status;
  UINTN       Size;
  UINT32      VariableAttr;

  for (int i = 0; i < ARRAY_SIZE (gDeviceIdFnToTargetVarNameMap); i++) {
    VariableAttr = 0;
    Size         = 0;
    Status       = gRT->GetVariable (
                          gDeviceIdFnToTargetVarNameMap[i].DeviceIdVarName,
                          &MFCI_VAR_VENDOR_GUID,
                          &VariableAttr,
                          &Size,
                          NULL
                          );
    if (Status != EFI_BUFFER_TOO_SMALL) {
      DEBUG ((DEBUG_VERBOSE, "MFCI targeting variable %s returned %r\n", gDeviceIdFnToTargetVarNameMap[i].DeviceIdVarName, Status));
      return FALSE;
    }
  }

  return TRUE;
}

EFI_STATUS
PopulateTargetVarsFromLib (
  )
{
  EFI_STATUS  Status;
  CHAR16      *TargetString;
  UINTN       TargetStringSize;

  DEBUG ((DEBUG_INFO, "MfciDxe: %a() - Enter\n", __FUNCTION__));

  for (int i = 0; i < ARRAY_SIZE (gDeviceIdFnToTargetVarNameMap); i++) {
    DEBUG ((DEBUG_VERBOSE, "Calling MfciDeviceIdSupportLib to populate MFCI target variable: %s\n", gDeviceIdFnToTargetVarNameMap[i].DeviceIdVarName));

    TargetStringSize = 0;
    TargetString     = NULL;

    // invoke the target DeviceId function corresponding to the current index
    Status = gDeviceIdFnToTargetVarNameMap[i].DeviceIdFn (&TargetString, &TargetStringSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "MfciDeviceIdSupportLib function index %d returned %r\n", i, Status));
      if (TargetString != NULL) {
        FreePool (TargetString);
        TargetString = NULL;
      }

      break;
    }

    // set the targeting variable name corresponding to the current index
    Status = gRT->SetVariable (
                    gDeviceIdFnToTargetVarNameMap[i].DeviceIdVarName,
                    &MFCI_VAR_VENDOR_GUID,
                    MFCI_POLICY_TARGETING_VARIABLE_ATTR,
                    TargetStringSize,
                    TargetString
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to set MFCI targeting variable %s, returned %r\n", gDeviceIdFnToTargetVarNameMap[i].DeviceIdVarName, Status));
      FreePool (TargetString);
      TargetString = NULL;
      break;
    }

    FreePool (TargetString);
  }

  return Status;
}

/**
 * Executes after variable policy protocol becomes available, uses it to lock variables
 *
 * @param Event
 *   Unused
 * @param Context
 *   Unused
 */
VOID
EFIAPI
VarPolicyCallback (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  RegisterVarPolicies ();
}

/**
 * Validate blob on each certificate from preset XDR buffer.
 *
 * @param SignedPolicy        Pointer to hold the policy buffer to be validated.
 * @param SignedPolicySize    Size of SignedPolicy, in bytes.
 * @param Certificates        Pointer to hold the XDR formatted buffer of certificates.
 * @param CertificatesSize    Size of Certificates, in bytes.
 *
 * @retval EFI_SUCCESS        The one certificate from Certificate is valid for input policy validation.
 * @retval EFI_ABORTED        SignedPolicy is null data or at least one certificate from incoming Certificates is
 *                            malformatted.
 * @retval Others             Other errors from the underlying ValidateBlob function.
 */
EFI_STATUS
ValidateBlobWithXdrCertificates (
  IN CONST UINT8  *SignedPolicy,
  IN UINTN        SignedPolicySize,
  IN CONST UINT8  *Certificates,
  IN UINTN        CertificatesSize
  )
{
  EFI_STATUS   Status;
  CONST UINT8  *PublicKeyDataXdr;
  CONST UINT8  *PublicKeyDataCurrent;
  CONST UINT8  *PublicKeyDataXdrEnd;
  CONST UINT8  *PublicKeyData;
  UINTN        PublicKeyDataLength;
  CHAR8        *RequiredEKUs;
  UINTN        Index;

  if ((SignedPolicy == NULL) || (SignedPolicySize == 0)) {
    DEBUG ((DEBUG_ERROR, "Incoming signed policy buffer is invalid, aborting validation!\n"));
    Status = EFI_ABORTED;
    goto Exit;
  }

  // Initialize configuration from PCDs, consider moving elsewhere and only initializing if there is a blob to validate
  RequiredEKUs = (CHAR8 *)FixedPcdGetPtr (PcdMfciPkcs7RequiredLeafEKU);

  // below is inspired/borrowed from FmpDxe.c
  PublicKeyDataXdr    = Certificates;
  PublicKeyDataXdrEnd = PublicKeyDataXdr + CertificatesSize;

  if ((PublicKeyDataXdr == NULL) || ((PublicKeyDataXdr + sizeof (UINT32)) > PublicKeyDataXdrEnd)) {
    DEBUG ((DEBUG_ERROR, "Pcd PcdMfciPkcs7CertBufferXdr NULL or invalid size\n"));
    Status = EFI_ABORTED;
    goto Exit;
  }

  PublicKeyDataCurrent = PublicKeyDataXdr;
  //
  // Try each key from PcdFmpDevicePkcs7CertBufferXdr
  //
  for (Index = 1; PublicKeyDataCurrent < PublicKeyDataXdrEnd; Index++) {
    DEBUG ((
      DEBUG_INFO,
      "%a: Certificate #%d [%p..%p].\n",
      __FUNCTION__,
      Index,
      PublicKeyDataCurrent,
      PublicKeyDataXdrEnd
      ));

    if ((PublicKeyDataCurrent + sizeof (UINT32)) > PublicKeyDataXdrEnd) {
      //
      // Key data extends beyond end of PCD
      //
      DEBUG ((DEBUG_ERROR, "%a: Certificate size extends beyond end of PCD, skipping it.\n", __FUNCTION__));
      Status = EFI_ABORTED;
      goto Exit;
    }

    // Read key length stored in big-endian format
    //
    PublicKeyDataLength = SwapBytes32 (*(UINT32 *)(PublicKeyDataCurrent));
    //
    // Point to the start of the key data
    //
    PublicKeyData = PublicKeyDataCurrent + sizeof (UINT32);

    // Length + ALIGN_VALUE(Length, 4) for 4-byte alignment (XDR standard).
    if ((PublicKeyData + ALIGN_VALUE (PublicKeyDataLength, 4)) > PublicKeyDataXdrEnd) {
      DEBUG ((
        DEBUG_ERROR,
        "%a - PcdMfciPkcs7CertBufferXdr size incorrect: PublicKeyData(0x%x) PublicKeyDataLength(0x%x) PublicKeyDataXdrEnd(0x%x)\n",
        __FUNCTION__,
        PublicKeyData,
        PublicKeyDataLength,
        PublicKeyDataXdrEnd
        ));
      Status = EFI_ABORTED;
      goto Exit;
    }

    Status = ValidateBlob (SignedPolicy, SignedPolicySize, PublicKeyData, PublicKeyDataLength, RequiredEKUs);
    if (!EFI_ERROR (Status)) {
      break;
    }

    PublicKeyDataCurrent = PublicKeyData + PublicKeyDataLength;
    PublicKeyDataCurrent = (UINT8 *)ALIGN_POINTER (PublicKeyDataCurrent, sizeof (UINT32));
  }

  // above is inspired/borrowed from FmpDxe.c
Exit:
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
VOID
EFIAPI
VerifyPolicyAndChange (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  BOOLEAN           RngResult;
  UINT32            VariableAttr = 0;
  UINTN             DataSize;
  VOID              *CurrentBlob = NULL;
  UINTN             CurrentBlobSize;
  UINT64            CurrentNonce;
  VOID              *TargetBlob = NULL;
  UINTN             TargetBlobSize;
  UINT64            TargetNonce;
  MFCI_POLICY_TYPE  BlobPolicy;

  DEBUG ((DEBUG_INFO, "MfciDxe: %a() - Enter\n", __FUNCTION__));

  if (!CheckTargetVarsExist ()) {
    Status = PopulateTargetVarsFromLib ();
    if (EFI_ERROR (Status)) {
      if (Status == EFI_UNSUPPORTED) {
        DEBUG ((DEBUG_ERROR, "MfciDeviceIdSupportLib returned EFI_UNSUPPORTED. Did you forget to either create the MFCI targeting variables, or implement MfciDeviceIdSupportLib?\n"));
      }

      Status = EFI_ABORTED;
      goto Exit;
    }
  }

  // Step 1: Check target nonce exist
  TargetNonce = MFCI_POLICY_INVALID_NONCE;
  DataSize    = sizeof (TargetNonce);
  Status      = gRT->GetVariable (
                       NEXT_MFCI_NONCE_VARIABLE_NAME,
                       &MFCI_VAR_VENDOR_GUID,
                       &VariableAttr,
                       &DataSize,
                       &TargetNonce
                       );
  if (EFI_ERROR (Status) ||
      (DataSize != sizeof (TargetNonce)) ||
      (VariableAttr != MFCI_POLICY_VARIABLE_ATTR) ||
      (TargetNonce == MFCI_POLICY_INVALID_NONCE))
  {
    DEBUG ((DEBUG_INFO, "%a - Refreshing Target Nonce - DataSize(%d) VariableAttr(%x) TargetNonce(0x%lx) Status(%r)\n", __FUNCTION__, DataSize, VariableAttr, TargetNonce, Status));

    // Create a new one if we do not like it..
    TargetNonce = MFCI_POLICY_INVALID_NONCE;
    RngResult   = GetRandomNumber64 (&TargetNonce);
    if (!RngResult) {
      DEBUG ((DEBUG_ERROR, "%a - Generating random number 64 failed.\n", __FUNCTION__));
      ASSERT (FALSE);
      TargetNonce = MFCI_POLICY_INVALID_NONCE;
    }

    Status = gRT->SetVariable (
                    NEXT_MFCI_NONCE_VARIABLE_NAME,
                    &MFCI_VAR_VENDOR_GUID,
                    MFCI_POLICY_VARIABLE_ATTR,
                    sizeof (TargetNonce),
                    &TargetNonce
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Set TARGET nonce failed! - %r\n", __FUNCTION__, Status));
      ASSERT (FALSE);
      goto Exit;
    }
  }

  // Step 2: Check current policy related variables
  // Step 2.1: grab current blob
  DEBUG ((DEBUG_INFO, "%a - Step 2: Check current policy related variables.\n", __FUNCTION__));
  // Check for presence of current MFCI policy blob.  It is either:
  // i. not found
  // ii. other error
  // iii. found with correct attributes
  CurrentBlobSize = 0;
  Status          = gRT->GetVariable (
                           CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME,
                           &MFCI_VAR_VENDOR_GUID,
                           &VariableAttr,
                           &CurrentBlobSize,
                           CurrentBlob
                           );
  // i. not found
  if (Status == EFI_NOT_FOUND) {
    DEBUG ((DEBUG_INFO, "%a - Get current MFCI Policy blob - %r\n", __FUNCTION__, Status));

    // If there is no current blob found, make sure all current stuff looks good
    Status = InternalCleanupCurrentPolicy ();

    // If we return check result and decide whether we should go for target side
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Clear other current variables returned - %r\n", __FUNCTION__, Status));
      goto Exit;
    } else {
      DEBUG ((DEBUG_INFO, "%a - Clear other current variables returned, proceeding to TARGET step.\n", __FUNCTION__));
      goto VerifyTarget;
    }
  }
  // ii. other error
  else if ((Status != EFI_BUFFER_TOO_SMALL) ||
           (VariableAttr != MFCI_POLICY_VARIABLE_ATTR))
  {
    // Something is wrong, bail here..
    DEBUG ((DEBUG_ERROR, "%a - Initial get current MFCI Policy blob failed - %r with attribute %08x\n", __FUNCTION__, Status, VariableAttr));
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  // iii. found with correct attributes

  CurrentBlob = AllocatePool (CurrentBlobSize);
  if (CurrentBlob == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Allocating memory for current MFCI Policy blob of size %d failed.\n", __FUNCTION__, CurrentBlobSize));
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  Status = gRT->GetVariable (
                  CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME,
                  &MFCI_VAR_VENDOR_GUID,
                  &VariableAttr,
                  &CurrentBlobSize,
                  CurrentBlob
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "%a - Second get current MFCI Policy blob failed - %r\n", __FUNCTION__, Status));

    // If there is error reading current blob, make sure clearing all current staff looks good
    Status = InternalCleanupCurrentPolicy ();

    // If we return check result and decide whether we should go for target side
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Clear ALL current variables returned - %r\n", __FUNCTION__, Status));
      goto Exit;
    } else {
      DEBUG ((DEBUG_INFO, "%a - Clear ALL current variables returned, proceeding to TARGET step.\n", __FUNCTION__));
      goto VerifyTarget;
    }
  }

  // Step 2.2: grab current nonce
  DataSize = sizeof (CurrentNonce);
  Status   = gRT->GetVariable (
                    CURRENT_MFCI_NONCE_VARIABLE_NAME,
                    &MFCI_VAR_VENDOR_GUID,
                    &VariableAttr,
                    &DataSize,
                    &CurrentNonce
                    );
  if (EFI_ERROR (Status) ||
      (DataSize != sizeof (CurrentNonce)) ||
      (VariableAttr != MFCI_POLICY_VARIABLE_ATTR))
  {
    // Something we do not like about this.. bail here
    DEBUG ((DEBUG_ERROR, "%a - Reading current nonce failed - %r with size: %d and attribute: 0x%08x.\n", __FUNCTION__, Status, DataSize, VariableAttr));
    goto Exit;
  }

  // Step 2.3: validate current blob signature

  Status = ValidateBlobWithXdrCertificates (CurrentBlob, CurrentBlobSize, FixedPcdGetPtr (PcdMfciPkcs7CertBufferXdr), FixedPcdGetSize (PcdMfciPkcs7CertBufferXdr));
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - validate current blob failed - %r.\n", __FUNCTION__, Status));

    // If there is no current blob found, make sure all current staff looks good
    Status = InternalCleanupCurrentPolicy ();

    // If we return check result and decide whether we should go for target side
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Clean invalid current policy failed - %r.\n", __FUNCTION__, Status));
      goto Exit;
    } else {
      DEBUG ((DEBUG_INFO, "%a - Clean invalid current policy returned, proceeding to TARGET step.\n", __FUNCTION__));
      goto VerifyTarget;
    }
  }

  // Step 2.4: verify targeting is for this machine
  Status = VerifyTargeting (
             CurrentBlob,
             CurrentBlobSize,
             CurrentNonce,
             &BlobPolicy
             );
  if (!EFI_ERROR (Status)) {
    BlobPolicy &= ~MFCI_POLICY_VALUE_ACTIONS_MASK; // clear the action bits as they would have been processed upon installation
  }

  if (EFI_ERROR (Status) ||
      (BlobPolicy != mCurrentPolicy))
  {
    // TODO: Telemetry here
    DEBUG ((DEBUG_ERROR, "%a Verify targeting return error - %r. mCurrentPolicy: 0x%16x, BlobPolicy: 0x%16x.\n", __FUNCTION__, Status, mCurrentPolicy, BlobPolicy));

    // If targeting is incorrect, or current policy and extracted has mismatch, fall back
    Status = InternalCleanupCurrentPolicy ();

    // If we return check result and decide whether we should go for target side
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Clean invalid targeting current policy failed - %r.\n", __FUNCTION__, Status));
      goto Exit;
    } else {
      DEBUG ((DEBUG_INFO, "%a - Clean invalid targeting current policy returned, proceeding to TARGET step.\n", __FUNCTION__));
      goto VerifyTarget;
    }
  }

VerifyTarget:

  // Step 3: If we got here, check target policy related variables
  DEBUG ((DEBUG_ERROR, "%a - Verify targeting step!\n", __FUNCTION__));

  // Step 3.1: grab target blob
  TargetBlobSize = 0;
  Status         = gRT->GetVariable (
                          NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME,
                          &MFCI_VAR_VENDOR_GUID,
                          &VariableAttr,
                          &TargetBlobSize,
                          TargetBlob
                          );
  if (Status == EFI_NOT_FOUND) {
    // If there is no target blob found, we are done!!!
    DEBUG ((DEBUG_INFO, "%a - No target blob found, bail here.\n", __FUNCTION__));
    Status = EFI_SUCCESS;
    goto Exit;
  } else if ((Status != EFI_BUFFER_TOO_SMALL) ||
             (VariableAttr != MFCI_POLICY_VARIABLE_ATTR))
  {
    // Something is wrong, bail here..
    DEBUG ((DEBUG_ERROR, "%a - Failed to read target blob - %r with attribute 0x%08x.\n", __FUNCTION__, Status, VariableAttr));
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  TargetBlob = AllocatePool (TargetBlobSize);
  if (TargetBlob == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Allocating memory for target MFCI Policy blob of size %d failed.\n", __FUNCTION__, TargetBlobSize));
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  Status = gRT->GetVariable (
                  NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME,
                  &MFCI_VAR_VENDOR_GUID,
                  &VariableAttr,
                  &TargetBlobSize,
                  TargetBlob
                  );
  if (EFI_ERROR (Status)) {
    // There is something wrong here...
    // Try to tear down everything and bail
    DEBUG ((DEBUG_ERROR, "%a - Failed to read target blob - %r.\n", __FUNCTION__, Status));

    Status = InternalCleanupTargetPolicy ();

    DEBUG ((DEBUG_WARN, "%a - Clean up bad target variable returned - %r.\n", __FUNCTION__, Status));
    goto Exit;
  }

  // Step 3.2: grab target nonce (which is TargetNonce from step 1)
  // Do nothing.

  // Step 3.3: validate target blob signature
  Status = ValidateBlobWithXdrCertificates (TargetBlob, TargetBlobSize, FixedPcdGetPtr (PcdMfciPkcs7CertBufferXdr), FixedPcdGetSize (PcdMfciPkcs7CertBufferXdr));
  if (EFI_ERROR (Status)) {
    // In effort of being fail safe, we let it fail here
    DEBUG ((DEBUG_ERROR, "%a - Target blob validation failed - %r.\n", __FUNCTION__, Status));

    Status = InternalCleanupTargetPolicy ();

    DEBUG ((DEBUG_WARN, "%a - Clean up invalid target variable returned - %r.\n", __FUNCTION__, Status));
    goto Exit;
  }

  // Step 3.4: verify targeting is for this machine
  Status = VerifyTargeting (
             TargetBlob,
             TargetBlobSize,
             TargetNonce,
             &BlobPolicy
             );

  if (EFI_ERROR (Status)) {
    // If target is wrong, we fail, back to safe zone
    DEBUG ((DEBUG_ERROR, "%a - Target blob validation failed - %r.\n", __FUNCTION__, Status));

    Status = InternalCleanupTargetPolicy ();

    DEBUG ((DEBUG_WARN, "%a - Clean up mis-targeted target variable returned - %r.\n", __FUNCTION__, Status));
    goto Exit;
  }

  // Step 4: If we are still here, probably it is time to do transition
  // This routine will not return
  InternalTransitionRoutine (
    BlobPolicy,
    TargetNonce,
    TargetBlob,
    TargetBlobSize
    );

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

  EFI_STATUS  Status2 = LockPolicyVariables ();

  if (EFI_ERROR (Status) || EFI_ERROR (Status2)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a !!! An error occurred while processing MFCI Policy - Status(%r), Status2(%r)\n",
      __FUNCTION__,
      Status,
      Status2
      ));

    Status = InternalCleanupCurrentPolicy ();
    // TODO: Log telemetry for any errors that occur.
    // If we return check result and decide whether we should go for target side
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Failed to clean targeting current policy on failed policy processing - %r!!!\n", __FUNCTION__, Status));
    } else {
      DEBUG ((DEBUG_INFO, "%a - Clean targeting current policy succeeded, returning.\n", __FUNCTION__));
    }
  }

  DEBUG ((DEBUG_VERBOSE, "MfciDxe: %a() - Exit\n", __FUNCTION__));
}

/**

Routine Description:

  Driver Entrypoint that...
  Reads data for the early boot policy from MfciRetrievePolicyLib
  Installs the protocol for others to get the current policy & register change notifications
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
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   VarPolicyEvent;
  EFI_EVENT   MfciPolicyCheckEvent = NULL;
  VOID        *NotUsed;

  mCurrentPolicy       = CUSTOMER_STATE; // safety net
  mVarPolicyRegistered = FALSE;

  DEBUG ((DEBUG_INFO, "MfciDxe: %a() - Enter\n", __FUNCTION__));

  // First initialize the variable policies & prepare locks
  // NOTE:  we _always_ lock the variables to prevent tampering by an attacker.
  Status = gBS->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid, NULL, &NotUsed);
  if (EFI_ERROR (Status)) {
    // The DepEx should have ensured that Variable Policy was already available.  If we fail to locate the protocol,
    // ASSERT on debug builds, and for retail register a notification in hopes the system will recovery (defense in depth)
    DEBUG ((DEBUG_ERROR, "%a() - Failed to locate VariablePolicy protocol with status %r, will register protocol notification\n", __FUNCTION__, Status));
    ASSERT (FALSE);

    Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL, TPL_CALLBACK, VarPolicyCallback, NULL, &VarPolicyEvent);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a() - CreateEvent failed returning %r\n", __FUNCTION__, Status));
      goto Error;
    }

    Status = gBS->RegisterProtocolNotify (&gEdkiiVariablePolicyProtocolGuid, VarPolicyEvent, (VOID **)&NotUsed);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a() - RegisterProtocolNotify failed returning %r\n", __FUNCTION__, Status));
      goto Error;
    }
  } else {
    Status = RegisterVarPolicies ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a() - RegisterVarPolicies failed returning %r\n", __FUNCTION__, Status));
      goto Error; // attempt to lock anything that might have registered successfully...
    }
  }

  /**
    During earlier phases of boot, the platform uses a cached copy of the policy
    because we prefer to limit the amount of crypto and parsing in the early TCB.
    MfciRetrievePolicy() is the abstraction that retrieves the cached policy that
    was used during the earlier phases. This is used to determine if there is a
    state mis-match with the current policy or if the incoming new policy is the
    same or different from the current one.
  **/
  Status = MfciRetrievePolicy (&mCurrentPolicy);

  /**
    On first boot after flashing, the cached copy does not exist yet.  We handle
    this, or any other error receiving the policy, as if the system was in
    CUSTOMER_STATE.  When events are called back, the variables should be
    properly initialized and resynchronized.
  **/
  if (EFI_ERROR (Status)) {
    DEBUG ((
      ((Status == EFI_NOT_FOUND) ? DEBUG_INFO : DEBUG_ERROR),
      "%a() - MfciRetrievePolicy failed returning %r\n",
      __FUNCTION__,
      Status
      ));
    // Continue in CUSTOMER_STATE, may be first boot with variable not initialized, clear error status
    mCurrentPolicy = CUSTOMER_STATE;
    Status         = EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "%a() - MFCI Policy after retrieve 0x%lx\n", __FUNCTION__, mCurrentPolicy));

  Status = InitPublicInterface ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a() - InitPublicInterface failed returning %r\n", __FUNCTION__, Status));
    goto Error;
  }

  Status = InitSecureBootListener ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a() - Initializing Secure Boot Callback failed! %r\n", __FUNCTION__, Status));
    goto Error;
  }

  Status = InitTpmListener ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a() - Initializing Tpm Callback failed! %r\n", __FUNCTION__, Status));
    goto Error;
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
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Registering Start of BDS failed!!! %r\n", __FUNCTION__, Status));
    goto Error;
  }

  goto Exit;

Error:
  LockPolicyVariables (); // ignore this status, let existing failure status flow through

Exit:

  DEBUG ((DEBUG_VERBOSE, "MfciDxe: %a() - Exit\n", __FUNCTION__));
  return Status;
}// MfciDxeEntry()
