*** Settings ***
# @file
#
Documentation    This test suite tests the standard DFCI feature set for regressions.
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

*** Variables ***
#DFCI python scripts
${DFCI_PY_PATH}     Support${/}Python
# Tools used
${GEN_IDENTITY}     ${DFCI_PY_PATH}${/}GenerateCertProvisionData.py
${GEN_PERMISSIONS}  ${DFCI_PY_PATH}${/}GeneratePermissionPacketData.py
${GEN_SETTINGS}     ${DFCI_PY_PATH}${/}GenerateSettingsPacketData.py


${OWNER_KEY_INDEX}  1
${USER_KEY_INDEX}   2
${USER_KEY1_INDEX}  3
${USER_KEY2_INDEX}  4
${ZTD_KEY_INDEX}    5


${STATUS_SUCCESS}               0
${STATUS_VARIABLE_NOT_FOUND}    203
${STATUS_LOAD_ERROR}            0x8000000000000001
${STATUS_INVALID_PARAMETER}     0x8000000000000002
${STATUS_UNSUPPORTED}           0x8000000000000003
${STATUS_BAD_BUFFER_SIZE}       0x8000000000000004
${STATUS_BUFFER_TO_SMALL}       0x8000000000000005
${STATUS_NOT_READY}             0x8000000000000006
${STATUS_DEVICE_ERROR}          0x8000000000000007
${STATUS_NOT_FOUND}             0x800000000000000E
${STATUS_ACCESS_DENIED}         0x800000000000000F
${STATUS_NO_MAPPING}            0x8000000000000011
${STATUS_ABORTED}               0x8000000000000015
${STATUS_SECURITY_VIOLATION}    0x800000000000001A

#
# Device Identifier Variables
#
${DEVICE_ID_GUID}           4123a1a9-6f50-4b58-9c3d-56fc24c6c89e

${DEVICE_ID_CURRENT}        DfciDeviceIdentifier

#
# Identity Variables
#
${DFCI_ATTRIBUTES}         7

${IDENTITY_GUID}           DE6A8726-05DF-43CE-B600-92BD5D286CFD

${IDENTITY_CURRENT}        DfciIdentityCurrent
${IDENTITY_APPLY}          DfciIdentityApply
${IDENTITY_RESULT}         DfciIdentityResult
${IDENTITY2_APPLY}         DfciIdentity2Apply
${IDENTITY2_RESULT}        DfciIdentity2Result

#
# Permission Variables
#
${PERMISSION_GUID}         3a9777ea-0d9f-4b65-9ef3-7caa7c41994b

${PERMISSION_CURRENT}      DfciPermissionCurrent
${PERMISSION_APPLY}        DfciPermissionApply
${PERMISSION_RESULT}       DfciPermissionResult
${PERMISSION2_APPLY}       DfciPermission2Apply
${PERMISSION2_RESULT}      DfciPermission2Result

#
# Settings Variables
#
${SETTINGS_GUID}           D41C8C24-3F5E-4EF4-8FDD-073E1866CD01

${SETTINGS_CURRENT}        DfciSettingsCurrent
${SETTINGS_APPLY}          DfciSettingsRequest
${SETTINGS_RESULT}         DfciSettingsResult
${SETTINGS2_APPLY}         DfciSettings2Request
${SETTINGS2_RESULT}        DfciSettings2Result
