/** @file
  This module handles distribution of previously existing/cached
  MFCI Policies and ingestion of policy updates from system's
  root of trust state.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>

#include <MfciPolicyType.h>
#include <MfciVariables.h>
#include <Protocol/MfciProtocol.h>

#include <Protocol/VariablePolicy.h>
#include <Guid/MuVarPolicyFoundationDxe.h>

#include <Library/BaseLib.h>                        // CpuDeadLoop()
#include <Library/DebugLib.h>                       // DEBUG tracing
#include <Library/UefiBootServicesTableLib.h>       // gBS
#include <Library/UefiRuntimeServicesTableLib.h>    // gRT
#include <Library/ResetUtilityLib.h>                // ResetPlatformSpecificGuid()
#include <Library/VariablePolicyHelperLib.h>        // NotifyMfciPolicyChange()
#include <Library/MfciRetrievePolicyLib.h>          // MfciRetrievePolicy()
#include <Library/MfciRetrieveTargetPolicyLib.h>    // MfciRetrieveTargetPolicy()
#include <Library/MuTelemetryHelperLib.h>           // LogTelemetry()

#include "MfciDxe.h"

MFCI_POLICY_TYPE  mCurrentPolicy;
BOOLEAN           mVarPolicyRegistered;

/**
  A helper function to cache the new MFCI policy into variable storage.

  @param[in]  NewPolicy     New policy to be cached in the variable storage.

  @retval     EFI_SUCCESS   CURRENT_MFCI_POLICY_VARIABLE_NAME is updated with new policy.
  @retval     Others        Failures returned from SetVariable

**/
STATIC
EFI_STATUS
RecordNewPolicy (
  IN MFCI_POLICY_TYPE  NewPolicy
  )
{
  EFI_STATUS  Status;

  Status = gRT->SetVariable (
                  CURRENT_MFCI_POLICY_VARIABLE_NAME,
                  &MFCI_VAR_VENDOR_GUID,
                  MFCI_POLICY_VARIABLE_ATTR,
                  sizeof (NewPolicy),
                  &NewPolicy
                  );
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to set %s to NewPolicy - %r\n", __FUNCTION__, CURRENT_MFCI_POLICY_VARIABLE_NAME, Status));
  }

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
}

/**
  A helper function to register variable policy on CURRENT_MFCI_POLICY_VARIABLE_NAME.

  This variable policy is set to take effect when MFCI_LOCK_VAR_NAME is set to
  MFCI_LOCK_VAR_VALUE.

  @retval     EFI_SUCCESS             Locked all variables.
  @retval     EFI_SECURITY_VIOLATION  Failed to lock one of the required variables.

**/
STATIC
EFI_STATUS
RegisterVarPolicies (
  VOID
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

  // reaching here means that all variable policy was successfully registered
  mVarPolicyRegistered = TRUE;

Done:
  DEBUG ((DEBUG_VERBOSE, "MfciDxe: %a() - Exit\n", __FUNCTION__));
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
  EFI_STATUS        Status;
  MFCI_POLICY_TYPE  NewMfciType = CUSTOMER_STATE;

  // Step 1: Fetch the current policy through abstracted interface
  Status = MfciRetrieveTargetPolicy (&NewMfciType);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a !!! An error occurred while trying to read new target policy value - Status(%r). Default to secure mode\n", __FUNCTION__, Status));
    NewMfciType = CUSTOMER_STATE;
  }

  DEBUG ((DEBUG_INFO, "%a New target policy value is 0x%lx (current MFCI policy is 0x%lx).\n", __FUNCTION__, NewMfciType, mCurrentPolicy));

  // Step 2: Check the difference
  if (NewMfciType == mCurrentPolicy) {
    DEBUG ((DEBUG_INFO, "%a Current MFCI type matches the cached value, skipping notification!\n", __FUNCTION__));
    goto Exit;
  }

  // Step 3: Notify the change if any, the result is not inspected...
  NotifyMfciPolicyChange (NewMfciType);

  // Step 4: Regardless of the result, set the new state to variable storage
  Status = RecordNewPolicy (NewMfciType);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a !!! An error occurred while updating variables to current value - Status(%r)\n", __FUNCTION__, Status));
  }

  // Step 5: reboot!
  ResetSystemWithSubtype (EfiResetCold, &gMfciPolicyChangeResetGuid);
  CpuDeadLoop ();

Exit:
  // Non-change: trigger the variable policies
  Status = LockPolicyVariables ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a !!! An error occurred while locking capabilities - Status(%r)\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);

    // Log telemetry and reboot here
    LogTelemetry (TRUE, NULL, EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_EC_ILLEGAL_SOFTWARE_STATE, NULL, NULL, Status, NewMfciType);
    ResetSystemWithSubtype (EfiResetCold, &gMfciPolicyChangeResetGuid);
    CpuDeadLoop ();
  }
}

/**

Routine Description:

  Driver Entrypoint that...
  Reads data for the early boot policy from HOB
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
MfciDxeRootOfTrustEntry   (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  Status2;
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
  Status2 = LockPolicyVariables (); // use a new status, let existing failure status flow through
  if (EFI_ERROR (Status2) || EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a !!! An error occurred when initializing MFCI framework - Status2(%r)\n", __FUNCTION__, Status2));
    ASSERT_EFI_ERROR (Status);
    ASSERT_EFI_ERROR (Status2);

    // Log telemetry and reboot here
    LogTelemetry (TRUE, NULL, EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_EC_START_ERROR, NULL, NULL, Status2, Status);
    ResetSystemWithSubtype (EfiResetCold, &gMfciPolicyChangeResetGuid);
    CpuDeadLoop ();
  }

Exit:

  DEBUG ((DEBUG_VERBOSE, "MfciDxe: %a() - Exit\n", __FUNCTION__));
  return Status;
}
