/** @file
  The MFCI Policy contains name/value fields.  Define the fields of
  interest, their name strings, and maximum sizes of the field values

  Enumerated fields 0 through 4 are specified by the OEM to target individual devices

  The Nonce is randomly generated by the MFCI Policy DXE driver
  on every policy change.  The field in the policy blob must match for a policy
  to be installed.

  The "policy" field is the bitfield payload describing the flavor of the policy

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MFCI_POLICY_FIELDS_H__
#define __MFCI_POLICY_FIELDS_H__

#include <MfciVariables.h>

/**
  This is the definition of MFCI policies that this package natively support.

**/
#define MFCI_POLICY_FIELD_MAX_LEN  (1 << 8) // 512 CHAR16's (including NULL), 1024 bytes

/**
  Enum below enables a helper function to verify that a field is valid
  If we added a struct that included the types of each field, then the helper
  could more simply iterate to MFCI_POLICY_FIELD_COUNT and dedicated logic for different field
  types could be removed, but not merited at this time and structure is not expected to
  grow significantly over time.
**/
typedef enum {
  MFCI_POLICY_TARGET_MANUFACTURER,
  MFCI_POLICY_TARGET_PRODUCT,
  MFCI_POLICY_TARGET_SERIAL_NUMBER,
  MFCI_POLICY_TARGET_OEM_01,
  MFCI_POLICY_TARGET_OEM_02,
  MFCI_POLICY_TARGET_NONCE,
  MFCI_POLICY_FIELD_UEFI_POLICY,
  MFCI_POLICY_FIELD_COUNT
} MFCI_POLICY_FIELD;

/**
  The strings of the names in the MFCI Policy name/value pairs
**/
STATIC CONST CHAR16  gPolicyBlobFieldName[MFCI_POLICY_FIELD_COUNT][MFCI_POLICY_FIELD_MAX_LEN] = {
  L"Target\\Manufacturer",
  L"Target\\Product",
  L"Target\\SerialNumber",
  L"Target\\OEM_01",
  L"Target\\OEM_02",
  L"Target\\Nonce", // this is nonce targeted by the binary policy blob
  L"UEFI\\Policy"
};

/**
  A helper that maps static MFCI Policy targeting fields to their corresponding UEFI variable names
**/
#define TARGET_POLICY_COUNT  5

extern CONST CHAR16  gPolicyTargetFieldVarNames[TARGET_POLICY_COUNT][MFCI_VAR_NAME_MAX_LENGTH];

#endif //__MFCI_POLICY_FIELDS_H__
