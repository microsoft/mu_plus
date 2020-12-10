/** @file
  MFCI Receive Policy Library for platforms where it comes directly from the variable store
  because no PEI phase exists to consume it and publish a HOB

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiRuntimeServicesTableLib.h>         // gRT
#include <Library/DebugLib.h>                            // DEBUG tracing
#include <MfciVariables.h>

#include <Library/MfciRetrievePolicyLib.h>

/**

Routine Description:

    During earlier phases of boot, the platform uses a cached copy of the policy
    because we prefer to limit the amount of crypto and parsing in the early TCB.
    MfciRetrievePolicy() is the abstraction that retrieves the cached policy that
    was used during the earlier phases.  Note that on first boot after flashing,
    the cached copy does not exist yet.
    The caller must be prepared to gracefully handle a return status of EFI_NOT_FOUND
    in particular for 1st boot scenarios.

Arguments:

  @param[out]  MfciPolicyValue -- the MFCI policy in force during early phases of boot
                                  MfciPolicyValue is not written if any error occurs

  @retval     EFI_SUCCESS             Successfully retrieved the early boot MFCI policy
  @retval     EFI_NOT_FOUND           Could not locate the early boot policy.  This is
                                      expected on the first boot after a clean flash.
  @retval     EFI_SECURITY_VIOLATION  The policy value was corrupt.  The library will
                                      attempt to clean up NV storage.
  @retval     EFI_INVALID_PARAMETER   Nuff said.
  @retval     EFI_UNSUPPORTED         Likely using the NULL library instance
  @retval     Others                  Unable to get HOB, variable, or other... ?

**/
EFI_STATUS
EFIAPI
MfciRetrievePolicy (
    OUT  MFCI_POLICY_TYPE  *MfciPolicyValue
 )
{
  EFI_STATUS        Status;
  MFCI_POLICY_TYPE  Policy;
  UINTN             DataSize = sizeof(Policy);
  UINT32            VariableAttr;

  if ( MfciPolicyValue == NULL ) {
    ASSERT(MfciPolicyValue != NULL);
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  Status = gRT->GetVariable (CURRENT_MFCI_POLICY_VARIABLE_NAME,
                             &MFCI_VAR_VENDOR_GUID,
                             &VariableAttr,
                             &DataSize,
                             &Policy);
  if (EFI_ERROR(Status)) {
    if (Status != EFI_NOT_FOUND) {
      DEBUG(( DEBUG_ERROR, "%a - Failure reading Current Policy - Status(%r)\n", __FUNCTION__, Status ));
    }
    goto Exit;
  }
  else if (DataSize != sizeof (Policy) || VariableAttr != MFCI_POLICY_VARIABLE_ATTR) {
    DEBUG(( DEBUG_ERROR, "%a - Invalid current policy size or attributes - DataSize(%d) VariableAttr(0x%x)\n"\
      "Will attempt to delete invalid current policy\n", __FUNCTION__, DataSize, VariableAttr ));
    Status = gRT->SetVariable (CURRENT_MFCI_POLICY_VARIABLE_NAME,
                             &MFCI_VAR_VENDOR_GUID,
                             MFCI_POLICY_VARIABLE_ATTR,
                             0,
                             NULL);
    if (Status != EFI_SUCCESS) {
      DEBUG(( DEBUG_ERROR, "%a - Failed to delete %s, returned %r\n", __FUNCTION__, CURRENT_MFCI_POLICY_VARIABLE_NAME, Status));
    }
    Status = EFI_SECURITY_VIOLATION;
    goto Exit;
  }

  *MfciPolicyValue = Policy;
  DEBUG(( DEBUG_INFO, "%a() - MFCI Policy From Variable 0x%lx\n", __FUNCTION__, *MfciPolicyValue ));

Exit:

  return Status;
}
