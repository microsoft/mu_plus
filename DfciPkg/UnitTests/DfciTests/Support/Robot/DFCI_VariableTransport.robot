*** Settings ***
# @file
#
Documentation    DFCI Variable Transport Keywords
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

Library     OperatingSystem
Library     Process
Library     Remote  http://${IP_OF_DUT}:${RF_PORT}
Library     Support${/}Python${/}DFCI_SupportLib.py


*** Keywords ***


Generic Get With Variables
    [Arguments]   ${Variable}  ${VariableGuid}  ${outputXmlFile}  ${Trim}
    @{rc}=              GetUefiVariable    ${Variable}  ${VariableGuid}  ${Trim}
    Should Be True      ${rc}[0] == 0
    Create Binary File  ${outputXmlFile}  ${rc}[1]
    File Should Exist   ${outputXmlFile}


Generic Set With Variables
    [Arguments]   ${Variable}  ${VariableGuid}  ${VariableFile}
    File Should Exist  ${VariableFile}
    ${FileContents}=   Get Binary File  ${VariableFile}
    ${rc}=             SetUefiVariable    ${Variable}  ${VariableGuid}  ${DFCI_ATTRIBUTES}  ${FileContents}
    Should Be True     ${rc} == 1


#
# Device Identifier operations
#
Get Device Identifier
    [Arguments]  ${outputXmlFile}
    Generic Get With Variables    ${DEVICE_ID_CURRENT}  ${DEVICE_ID_GUID}  ${outputXmlFile}  trim


#
# Identity operations
#
Get Current Identities
    [Arguments]     ${outputXmlFile}
    Generic Get With Variables    ${IDENTITY_CURRENT}  ${IDENTITY_GUID}  ${outputXmlFile}  trim


Apply Identity
    [Arguments]     ${mailbox}  ${binPkgFile}
    ${identityApply}=  Set Variable If  '${mailbox}' == '1'  ${IDENTITY_APPLY}  ${IDENTITY2_APPLY}

    Generic Set With Variables    ${identityApply}  ${IDENTITY_GUID}  ${binPkgFile}


Get Identity Results
    [Arguments]     ${mailbox}  ${binResultPkgFile}
    ${identityResult}=  Set Variable If  '${mailbox}' == '1'  ${IDENTITY_RESULT}  ${IDENTITY2_RESULT}

    Generic Get With Variables    ${identityResult}  ${IDENTITY_GUID}  ${binResultPkgFIle}  ${None}


#
# Permissionoperations
#
Get Current Permissions
    [Arguments]     ${outputXmlFile}
    Generic Get With Variables    ${PERMISSION_CURRENT}  ${PERMISSION_GUID}  ${outputXmlFile}  trim


Apply Permission
    [Arguments]     ${mailbox}  ${binPkgFile}
    ${permissionApply}=  Set Variable If  '${mailbox}' == '1'  ${PERMISSION_APPLY}  ${PERMISSION2_APPLY}

    Generic Set With Variables    ${permissionApply}  ${PERMISSION_GUID}  ${binPkgFile}


Get Permission Results
    [Arguments]     ${mailbox}  ${binResultPkgFile}
    ${permissionResult}=  Set Variable If  '${mailbox}' == '1'  ${PERMISSION_RESULT}  ${PERMISSION2_RESULT}

    Generic Get With Variables    ${permissionResult}  ${PERMISSION_GUID}  ${binResultPkgFIle}  ${None}


#
# Settings Operations
#
Get Current Settings
    [Arguments]     ${outputXmlFile}
    Generic Get With Variables    ${SETTINGS_CURRENT}  ${SETTINGS_GUID}  ${outputXmlFile}  trim


Apply Settings
    [Arguments]     ${mailbox}  ${binPkgFile}
    ${settingsApply}=  Set Variable If  '${mailbox}' == '1'  ${SETTINGS_APPLY}  ${SETTINGS2_APPLY}

    Generic Set With Variables    ${settingsApply}  ${SETTINGS_GUID}  ${binPkgFile}


Get Settings Results
    [Arguments]     ${mailbox}  ${binResultPkgFile}
    ${settingsResult}=  Set Variable If  '${mailbox}' == '1'  ${SETTINGS_RESULT}  ${SETTINGS2_RESULT}

    Generic Get With Variables    ${settingsResult}  ${SETTINGS_GUID}  ${binResultPkgFIle}  ${None}