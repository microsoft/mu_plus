*** Settings ***
# @file
#
Documentation    This test suite rolls the certificates used for owner and user.
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

Library         OperatingSystem
Library         Process

Library         Support${/}Python${/}DFCI_SupportLib.py
Library         Support${/}Python${/}DependencyLib.py
Library         Remote  http://${IP_OF_DUT}:${RF_PORT}

#Import the Generic Shared keywords
Resource        Support${/}Robot${/}DFCI_Shared_Paths.robot
Resource        Support${/}Robot${/}CertSupport.robot
Resource        Support${/}Robot${/}DFCI_Shared_Keywords.robot

#Import the platform specific log support
Resource        UefiSerial_Keywords.robot

# Use the following line for Python remote write to the UEFI Variables
Resource        Support${/}Robot${/}DFCI_VariableTransport.robot

Suite setup       Make Dfci Output
Suite Teardown    Terminate All Processes    kill=True


*** Variables ***
#default var but should be changed on the command line
${IP_OF_DUT}            127.0.0.1
${RF_PORT}              8270
#test output dir for data from this test run.
${TEST_OUTPUT_BASE}     ..${/}TEST_OUTPUT

#Test output location
${TEST_OUTPUT}          ${TEST_OUTPUT_BASE}

#Test Root Dir
${TEST_ROOT_DIR}        TestCases
${TEST_CASE_DIR}        ${TEST_ROOT_DIR}${/}DFCI_InTuneRollCerts

${TOOL_DATA_OUT_DIR}    ${TEST_OUTPUT}${/}bindata
${TOOL_STD_OUT_DIR}     ${TEST_OUTPUT}${/}stdout
${BOOT_LOG_OUT_DIR}     ${TEST_OUTPUT}${/}uefilogs

${CERTS_DIR}            Certs

${TARGET_VERSION}       V2

${DDS_CA_THUMBPRINT}      'Thumbprint Not Set'
${MDM_CA_THUMBPRINT}      'Thumbprint Not Set'
${ZTD_LEAF_THUMBPRINT}    'Thumbprint Not Set'


*** Keywords ***


Get The DFCI Settings
    [Arguments]    ${nameOfTest}
    ${deviceIdXmlFile}=         Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_deviceIdentifier.xml
    ${currentIdXmlFile}=        Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentIdentities.xml
    ${currentPermXmlFile}=      Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentPermission.xml
    ${currentSettingsXmlFile}=  Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentSettings.xml

    Get and Print Device Identifier    ${deviceIdXmlFile}

    Get and Print Current Identities   ${currentIdxmlFile}

    Get and Print Current Permissions  ${currentPermxmlFile}

    Get and Print Current Settings     ${currentSettingsxmlFile}

    [return]    ${currentIdxmlFile}


*** Test Cases ***


Ensure Mailboxes Are Clean
#Documentation Ensure all mailboxes are clear at the beginning of a test.  If there are any mailboxes that have an element, a previous test failed.
    Verify No Mailboxes Have Data


Get the starting DFCI Settings
    ${nameofTest}=              Set Variable    DisplaySettingsAtStart

    ${currentIdXmlFile}=     Get The DFCI Settings    ${nameOfTest}

    ${OwnerThumbprint}=    Get Thumbprint Element    ${currentIdxmlFile}  Owner
    ${UserThumbprint}=     Get Thumbprint Element    ${currentIdxmlFile}  User
    Should Be True    '${OwnerThumbprint}' != 'Cert not installed'
    Should Be True    '${UserThumbprint}' != 'Cert not installed'

    Initialize Thumbprints    '${OwnerThumbprint}'    '${UserThumbprint}'


Obtain Target Parameters From Target
    [Setup]    Require test case    Get the starting DFCI Settings
    ${nameofTest}=           Set Variable     GetParameters
    ${SerialNumber}=         Get System Under Test SerialNumber
    ${Manufacturer}=         Get System Under Test Manufacturer
    ${Model}=                Get System Under Test ProductName
    @{TARGET_PARAMETERS}=    Build Target Parameters  ${TARGET_VERSION}  ${SerialNumber}  ${Manufacturer}  ${Model}
    Set Suite Variable       @{TARGET_PARAMETERS}

    ${currentXmlFile}=    Set Variable   ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_UefiDeviceId.xml

    Get Device Identifier    ${currentXmlFile}
    Verify Identity Current  ${currentXmlFile}  ${Manufacturer}  ${Model}  ${SerialNumber}


Send Owner Roll Certificate Packet to System Being Transitioned
    [Setup]    Require test case    Obtain Target Parameters From Target

    # This replaces the owner key, packet signed by previous owner leaf

    ${nameofTest}=       Set Variable    OwnerEnroll

    Process Provision Packet     ${nameofTest}  1  ${OLD_OWNER_PFX}  ${NEW_OWNER_PFX}  ${NEW_OWNER_CERT}  ${OWNER_KEY_INDEX}  @{TARGET_PARAMETERS}


Send User Roll Certificate to System Being Transitioned
    [Setup]    Require test case    Send Owner Roll Certificate Packet to System Being Transitioned

    # This replaces the User key, packet signed by previous owner leaf

    ${nameofTest}=       Set Variable    UserEnroll

    Process Provision Packet     ${nameofTest}  2  ${OLD_USER_PFX}  ${NEW_USER_PFX}  ${NEW_USER_CERT}  ${USER_KEY_INDEX}  @{TARGET_PARAMETERS}


Restart System to Apply Enrollment
# Start serial log to capture UEFI log during the restart
    [Setup]    Require test case    Send User Roll Certificate to System Being Transitioned

    Start SerialLog     ${BOOT_LOG_OUT_DIR}${/}ApplyEnrollment.log

    Reboot System And Wait For System Online


Verify Owner Roll Ceritificate Identity Results
    ${nameofTest}=   Set Variable    OwnerEnroll

    Validate Provision Status    ${nameofTest}  1  ${STATUS_SUCCESS}


Verify User Roll ceritificate Identity Results
    ${nameofTest}=   Set Variable    UserEnroll

    Validate Provision Status    ${nameofTest}  2  ${STATUS_SUCCESS}


Get the ending DFCI Settings
    ${nameofTest}=              Set Variable    DisplaySettingsAtExit

    ${currentIdXmlFile}=    Get The DFCI Settings    ${nameOfTest}

    ${OwnerThumbprint}=     Get Thumbprint From Pfx    ${NEW_OWNER_CERT}
    ${rc}=   Get Thumbprint Element    ${currentIdxmlFile}    Owner
    Should Be True    '${rc}' == '${OwnerThumbprint}'

    ${UserThumbprint}=      Get Thumbprint From Pfx    ${NEW_USER_CERT}
    ${rc}=   Get Thumbprint Element    ${currentIdxmlFile}    User
    Should Be True    '${rc}' == '${UserThumbprint}'


Clean Up Mailboxes
    Verify No Mailboxes Have Data
