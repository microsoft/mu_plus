/** @file
  MFCI Receive Policy Library for platforms where PEI passes it by HOB to DXE

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/HobLib.h>                              // GetFirstGuidHob()
#include <Library/DebugLib.h>                            // DEBUG tracing

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
  VOID              *GuidHob;
  MFCI_POLICY_TYPE  *PolicyPtr;
  UINTN             EntrySize;

  if ( MfciPolicyValue == NULL ) {
    DEBUG ((DEBUG_ERROR, "MfciPolicyValue must be non-NULL\n"));
    ASSERT(MfciPolicyValue != NULL);
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  GuidHob = GetFirstGuidHob(&gMfciHobGuid);
  if (GuidHob == NULL) {
    DEBUG(( DEBUG_ERROR, "%a() - MFCI Policy HOB not found!\n", __FUNCTION__ ));
    ASSERT(GuidHob != NULL);
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  PolicyPtr = GET_GUID_HOB_DATA(GuidHob);
  EntrySize = GET_GUID_HOB_DATA_SIZE(GuidHob);
  if ((PolicyPtr == NULL) || (EntrySize != sizeof(MFCI_POLICY_TYPE))) {
    DEBUG(( DEBUG_ERROR, "%a() - MFCI Policy HOB malformed, PolicyPtr(%p) , EntrySize(%x)\n", __FUNCTION__, PolicyPtr, EntrySize ));
    ASSERT(PolicyPtr != NULL);
    ASSERT(EntrySize == sizeof(MFCI_POLICY_TYPE));
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  *MfciPolicyValue = *PolicyPtr;
  Status = EFI_SUCCESS;
  DEBUG(( DEBUG_INFO, "%a() - MFCI Policy from HOB 0x%lx\n", __FUNCTION__, *MfciPolicyValue ));

Exit:

  return Status;
}
