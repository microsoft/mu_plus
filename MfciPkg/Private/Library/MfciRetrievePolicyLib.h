/** @file
  Declares the interface to the MFCI policy DXE receiver library

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MFCI_RETRIEVE_POLICY_LIB_H__
#define __MFCI_RETRIEVE_POLICY_LIB_H__

#include <MfciPolicyType.h>


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
 );

#endif // __MFCI_RETRIEVE_POLICY_LIB_H__
