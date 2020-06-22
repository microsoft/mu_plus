/** @file
  Manufacturer Firmware Configuration Interface (MFCI) Policy flavor bitfield
  Defines the meaning of the bits in the policy payload of the MFCI Policy
  Also defines the EKU used for pinning the leaf trust anchor

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MFCI_POLICY_TYPE_H__
#define __MFCI_POLICY_TYPE_H__

/**
  This is the definition of MFCI policies that this package natively support.

**/
typedef UINT64                          MFCI_POLICY_TYPE;

#define CUSTOMER_STATE                  0             // Manufacturing abilities are disabled
#define STD_ACTION_SECURE_BOOT_CLEAR    BIT0          // Unlock secure boot on policy transition
#define STD_ACTION_TPM_CLEAR            BIT1          // Clear TPM on policy transition

#define MFCI_POLICY_VALUE_DEFINED_MASK 0x00000000FFFFFFFF
#define MFCI_POLICY_VALUE_OEM_MASK     0xFFFFFFFF00000000
#define MFCI_POLICY_VALUE_ACTIONS_MASK 0x0000FFFF0000FFFF
#define MFCI_POLICY_VALUE_STATES_MASK  0xFFFF0000FFFF0000

#endif //__MFCI_POLICY_TYPE_H__
