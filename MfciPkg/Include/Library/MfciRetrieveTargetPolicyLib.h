/** @file
  Declares the interface to query the target policy consumed
  by MFCI policy modules.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MFCI_RETRIEVE_TARGET_POLICY_LIB_H_
#define MFCI_RETRIEVE_TARGET_POLICY_LIB_H_

#include <MfciPolicyType.h>

/**

Routine Description:

    MfciRetrieveTargetPolicy() is the abstraction that retrieves the active policy that
    is recognized by system root of trust (RoT). The routine should handle necessary
    translations to conform to MFCI_POLICY_TYPE bit definitions from the RoT states.
    Note that the failure of retrieving target policy will default the system policy
    to CUSTOMER_STATE, and potentially a state transitioning.

    The caller must be prepared to gracefully handle a return status of EFI_NOT_FOUND
    in particular for 1st boot scenarios.

Arguments:

  @param[out]  MfciPolicyValue -- the MFCI policy in force as recognized by the system
                                  root of trust.

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
MfciRetrieveTargetPolicy (
  OUT  MFCI_POLICY_TYPE  *MfciPolicyValue
  );

#endif // MFCI_RETRIEVE_TARGET_POLICY_LIB_H_
