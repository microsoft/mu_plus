# @file
# Generate the test MFCI policy blob for
# the MFCI Policy Parsing Library Unit Test
#
# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

from edk2toollib.windows.policy.firmware_policy import FirmwarePolicy


policyManuf = FirmwarePolicy()
nonce = 0x0123456789abcdef
TargetInfo = {# EV Certificate Subject CN="<foo>"
              # Recommend also matches SmbiosSystemManufacturer, SMBIOS Table 1, offset 04h (System Manufacturer) 
              'Manufacturer': 'Contoso Computers, LLC',
              # SmbiosSystemProductName, SMBIOS Table 1, offset 05h (System Product Name)
              'Product': 'Laptop Foo',
              # SmbiosSystemSerialNumber, SMBIOS System Information (Type 1 Table) -> Serial NumberP
              'SerialNumber': 'F0013-000243546-X02',
              # Yours to define, or not use (NULL string), ODM name is suggested
              'OEM_01': 'ODM Foo',                       
              'OEM_02': '',                              # Yours to define, or not use (NULL string)
              'Nonce': nonce}
policyManuf.SetDeviceTarget(TargetInfo)

policy = FirmwarePolicy.MFCI_POLICY_VALUE_ACTION_SECUREBOOT_CLEAR + FirmwarePolicy.MFCI_POLICY_VALUE_ACTION_TPM_CLEAR
policyManuf.SetDevicePolicy(policy=policy)

fsout = open('.\\policy_good_manufacturing.bin','wb')
policyManuf.Serialize(output=fsout)
fsout.close()
policyManuf.Print()
