/** @file
  This driver provides the in-effect MFCI Policy to PEI

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MFCI_POLICY_PPI_H__
#define __MFCI_POLICY_PPI_H__

#include <MfciPolicyType.h>

typedef struct _MFCI_POLICY_PPI  MFCI_POLICY_PPI;

/**
  GetMfciPolicy()

  This function returns the MFCI Policy in effect for the current boot

  @param[in] This                - Current MFCI policy ppi installed.

  @retval Other                  - Bits definition can be found in MfciPolicyType.h.

**/
typedef
MFCI_POLICY_TYPE
(EFIAPI *GET_MFCI_POLICY) (
IN CONST MFCI_POLICY_PPI   *This
);


struct _MFCI_POLICY_PPI
{
  GET_MFCI_POLICY GetMfciPolicy;
};

extern EFI_GUID gMfciPpiGuid;

#endif // __MFCI_POLICY_PPI_H__
