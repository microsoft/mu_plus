/** @file
  Manufacturer Firmware Configuration Interface (MFCI) UEFI Variable interface
  Defines the public UEFI variables & attributes to both determine the
  current in-effect MFCI Policy and to request installation
  of a new MFCI Policy

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MFCI_POLICY_VAR_H__
#define __MFCI_POLICY_VAR_H__


/**
  The Vendor Guid for all of the MFCI Policy UEFI variables
  Ideally this would have been split into 2 Vendor Guids, one for state engine variables and
  another for targeting variables, but that would require updates to some end-to-end tooling
**/
#define MFCI_VAR_VENDOR_GUID \
        gMfciVendorGuid

/**
  Defines the attributes for MFCI Policy persistent data
  Note that some variables are protected by variable policy not visible from attributes.
**/
#define MFCI_POLICY_VARIABLE_ATTR \
        (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

/**
  Defines the maximum number of CHAR16's in a MFCI Policy variable name (including the terminating NULL)
**/
#define MFCI_VAR_NAME_MAX_LENGTH 32

/**
  Defines the maximum number of bytes in a MFCI Policy variable value
**/
#define MFCI_VAR_MAX_SIZE (1 << 9)

/**
  Policy Engine Runtime Variables
  Below are the variables that control MFCI Policy change requests.  They are
  non-volatile and both visible and writable from the OS.
  To set a new policy, write a correctly-signed, correctly-nonced, correctly
  targeted policy to NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME variable and reboot.
  To delete an MFCI policy and return to default device policy, delete
  CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME and reboot.
  In both cases, the policy state change is processed during the DXE phase of
  the next boot, listeners are notified, and the system will reboot a second
  time so that the PEI phase can take action based upon the new policy.
**/

/**
  Name of the variable that holds the signed binary MFCI policy blob in effect for this boot
  Delete this variable and reboot to restore a system to default policy.
**/
#define CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME \
        L"CurrentMfciPolicyBlob"

/**
  Name of the variable to receive the next policy blob to be authenticated & installed
  This is the writable mailbox where OS software puts a policy blob which UEFI will
  attempt to authenticate and install during the DXE phase of the subsequent boot
**/
#define NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME \
        L"NextMfciPolicyBlob"


/**
  Policy Engine Read-Only Variables
  Below are the variables that hold security-sensitive policy engine state information

  The variables below are locked (become Read-Only) using variable policy immediately
  prior to BDS (gMsStartOfBdsNotifyGuid), and cannot be modified by an OS.
**/

/**
  Name of the variable that holds the policy in effect for the current boot
  (i.e. STD_ACTION_SECURE_BOOT_CLEAR, STD_ACTION_TPM_CLEAR, etc.)
**/
#define CURRENT_MFCI_POLICY_VARIABLE_NAME \
        L"CurrentMfciPolicy"

/**
  Name of the variable that holds the trusted nonce for the policy in effect for the current boot
**/
#define CURRENT_MFCI_NONCE_VARIABLE_NAME \
        L"CurrentMfciPolicyNonce"

/**
  Name of the variable that holds a nonce for the next policy to be applied
  An attacker must not control the nonce
**/
#define NEXT_MFCI_NONCE_VARIABLE_NAME \
        L"NextMfciPolicyNonce"


/**
  Policy Engine Per-Device Targeting Variable Names
  Below are the variable names that are populated by OEM code during the DXE phase to enable per-device
  policy targeting.

  The variables below are locked (become Read-Only) using variable policy immediately
  prior to BDS (gMsStartOfBdsNotifyGuid), and cannot be modified by an OS.

  The variable values are wide-NULL-terminated CHAR16's.  They may contain UTF-16 values (full Unicode
  beyond the range of UC2 that is supported in UEFI), but should not contain Unicode escape sequences.
  Why?  The web interfaces that provide for generation of binary policies may accept UTF-8 JSON including
  escape sequences, but they will expand these into unescaped UTF-16 prior to generation of the signed
  policy binaries.
**/

/**
  The variable attributes for MFCI Policy per-device targeting
  Note that, not visible via attributes, these variables are locked by variable policy prior to BDS
**/
#define MFCI_POLICY_TARGETING_VARIABLE_ATTR \
        (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

/**
  Name of the variable that the OEM populates with the manufacturer name

  Regarding the variable value...
  Must match the EV certificate Subject Common Name value, e.g. CN="<foo>"
  Recommend matches SmbiosSystemManufacturer, SMBIOS Table 1, offset 04h (System Manufacturer)
  Example value: L"Contoso Computers, LLC"
**/
#define MFCI_MANUFACTURER_VARIABLE_NAME \
        L"Target\\Manufacturer"

/**
  Name of the variable that the OEM populates with the product name

  Regarding the variable value...
  Recommend matches SmbiosSystemProductName, SMBIOS Table 1, offset 05h (System Product Name)
  Example value: L"Laptop Foo"
**/
#define MFCI_PRODUCT_VARIABLE_NAME \
        L"Target\\Product"

/**
  Name of the variable that the OEM populates with the serial number

  Regarding the variable value...
  Recommend matches SmbiosSystemSerialNumber, SMBIOS System Information (Type 1 Table) -> Serial Number
  Example value: L"F0013-000243546-X02"
**/
#define MFCI_SERIALNUMBER_VARIABLE_NAME \
        L"Target\\SerialNumber"

/**
  Name of the variable that the OEM populates with an OEM-specified targeting value

  Regarding the variable value...
  Must be present, but may be a wide NULL if additional targeting is not used
  Example value: L"ODM Foo"
**/
#define MFCI_OEM_01_VARIABLE_NAME \
        L"Target\\OEM_01"

/**
  Name of the variable that the OEM populates with an OEM-specified targeting value

  Regarding the variable value...
  Must be present, but may be NULL string if additional targeting is not used
  Example value: L""  (not specified, must be present as WIDE NULL string)
**/
#define MFCI_OEM_02_VARIABLE_NAME \
        L"Target\\OEM_02"


/**
  Invalid Nonce value placeholder
  The server will refuse to sign policies with this nonce value
**/
#define MFCI_POLICY_INVALID_NONCE 0


/**
  The following variable uses Variable Policy to lock protected Windows
  MFCI variables

  Its VendorGuid namespace is gMuVarPolicyWriteOnceStateVarGuid
**/
#define MFCI_LOCK_VAR_NAME \
        L"MfciVarLock"

#define MFCI_LOCK_VAR_VALUE \
        0x01

#endif //__MFCI_POLICY_VAR_H__
