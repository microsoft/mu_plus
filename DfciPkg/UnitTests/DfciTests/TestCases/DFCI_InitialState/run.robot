*** Settings ***
# @file
#
Documentation
...     DFCI Initial State test - Verifies that there are no enrolled identities,
...     that the proper thumbprint is installed for the ZTD key, and verifies the
...     initial state of the permission store.
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

MetaData
...     - Read the current identities and permissions
...     - Verify the system is Opted In (ie Ztd key installed)
...     - Verify no owner certificates are installed
...     - Verify the initial permissions are correct for DFCI

Library         OperatingSystem
Library         Process
Library         Collections

Library         Support${/}Python${/}DFCI_SupportLib.py
Library         Support${/}Python${/}DependencyLib.py
Library         Support${/}Python${/}PermissionsXMLLib.py
Library         Remote  http://${IP_OF_DUT}:${RF_PORT}

#Import the Generic Shared keywords
Resource        Support${/}Robot${/}DFCI_Shared_Paths.robot
Resource        Support${/}Robot${/}CertSupport.robot
Resource        UefiSerial_Keywords.robot

#Import the DFCI Shared keywords
Resource        Support${/}Robot${/}DFCI_Shared_Keywords.robot

#
# Use the following line for Python remote write to the UEFI Variables
Resource        Support${/}Robot${/}DFCI_VariableTransport.robot

Suite setup       Make Dfci Output
Suite Teardown    Terminate All Processes    kill=True


*** Variables ***
#default var but should be changed on the command line
${IP_OF_DUT}            127.0.0.1
${RF_PORT}              8270
#test output directory for data from this test run.
${TEST_OUTPUT_BASE}     ..${/}TEST_OUTPUT

#Test output location
${TEST_OUTPUT}          ${TEST_OUTPUT_BASE}

#Test Root Dir
${TEST_ROOT_DIR}        TestCases

${TOOL_DATA_OUT_DIR}    ${TEST_OUTPUT}${/}bindata
${TOOL_STD_OUT_DIR}     ${TEST_OUTPUT}${/}stdout
${BOOT_LOG_OUT_DIR}     ${TEST_OUTPUT}${/}uefilogs

${CERTS_DIR}            ${TEST_ROOT_DIR}${/}certs

${TARGET_VERSION}       V2

${DDS_CA_THUMBPRINT}      'Thumbprint Not Set'
${MDM_CA_THUMBPRINT}      'Thumbprint Not Set'
${ZTD_LEAF_THUMBPRINT}    'Thumbprint Not Set'


*** Keywords ***

Initialize lists of tests
#[Documentation]
#... Each permission/PMask/DMask tuple is a list of three elements - the permission, PMask, and the DMask.
#... Establish a list of the permission tuples for the permissions to be set, and another
#... list for the permissions to be checked after the reboot.

    @{VTEST_01_CHECK1}=    Create List    Dfci.OwnerKey.Enum             9   128
    @{VTEST_01_CHECK2}=    Create List    Dfci.ZtdKey.Enum               1   ${None}
    @{VTEST_01_CHECK3}=    Create List    Dfci.ZtdUnenroll.Enable        0   ${None}
    @{VTEST_01_CHECK4}=    Create List    Dfci.Ztd.Recovery.Enable       0   ${None}
    @{INITIAL_CHECKS}=     Create List    ${VTEST_01_CHECK1}    ${VTEST_01_CHECK2}    ${VTEST_01_CHECK3}    ${VTEST_01_CHECK4}

    Set suite variable     ${INITIAL_CHECKS}
#
#
#
Process Initial Permission Check
    [Arguments]    ${Testname}
    ${currentPermXmlFile}=      Set Variable    ${TOOL_DATA_OUT_DIR}${/}Testcase_currentPermission.xml

    Get and Print Current Permissions     ${currentPermXmlFile}
    #
    ${rc}=    Validate Current Permissions    ${Testname}    ${currentPermXmlFile}   ${INITIAL_CHECKS}
    Should Be True    ${rc}


Get The DFCI Settings
    [Arguments]    ${nameOfTest}

    ${deviceIdXmlFile}=         Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_deviceIdentifier.xml
    ${currentIdXmlFile}=        Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentIdentities.xml
    ${currentPermXmlFile}=      Set Variable    ${TOOL_DATA_OUT_DIR}${/}Testcase_currentPermission.xml
    ${currentSettingsXmlFile}=  Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentSettings.xml

    Get and Print Device Identifier    ${deviceIdXmlFile}

    Get and Print Current Identities   ${currentIdxmlFile}

    Get and Print Current Permissions  ${currentPermxmlFile}

    Get and Print Current Settings     ${currentSettingsxmlFile}

    Set Suite Variable    ${currentIdXmlFile}


#------------------------------------------------------------------*
#  Test Cases                                                      *
#------------------------------------------------------------------*
*** Test Cases ***


Ensure Mailboxes Are Clean
#Documentation Ensure all mailboxes are clear at the beginning of a test.  If there are any mailboxes that have an element, a previous test failed.
    Verify No Mailboxes Have Data

    Log To Console    .
    Log To Console    ${SUITE SOURCE}

Verify System Unter Test is Opted In
    [Setup]    Require test case    Ensure Mailboxes Are Clean

                       Get The DFCI Settings    InitialSettings

    ${ZtdThumbprint}=  Get Thumbprint Element    ${currentIdXmlFile}  ZeroTouch

    Should Be True     '${ZtdThumbprint}' != 'Cert not installed'


Check that the starting DFCI Ownership is Unenrolled
    [Setup]    Require test case    Verify System Unter Test is Opted In

    ${OwnerThumbprint}=    Get Thumbprint Element    ${currentIdXmlFile}  Owner
    ${UserThumbprint}=     Get Thumbprint Element    ${currentIdXmlFile}  User
    ${User1Thumbprint}=    Get Thumbprint Element    ${currentIdXmlFile}  User1
    ${User2Thumbprint}=    Get Thumbprint Element    ${currentIdXmlFile}  User2

    Should Be True    '${OwnerThumbprint}' == 'Cert not installed'
    Should Be True    '${UserThumbprint}' == 'Cert not installed'
    Should Be True    '${User1Thumbprint}' == 'Cert not installed'
    Should Be True    '${User2Thumbprint}' == 'Cert not installed'


Obtain Target Parameters From Target
    [Setup]    Require test case    Check that the starting DFCI Ownership is Unenrolled

    ${nameofTest}=           Set Variable     GetParameters
    ${SerialNumber}=         Get System Under Test SerialNumber
    ${Manufacturer}=         Get System Under Test Manufacturer
    ${Model}=                Get System Under Test ProductName
    @{TARGET_PARAMETERS}=    Build Target Parameters  ${TARGET_VERSION}  ${SerialNumber}  ${Manufacturer}  ${Model}
    Set Suite Variable       @{TARGET_PARAMETERS}

    ${currentXmlFile}=    Set Variable   ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_UefiDeviceId.xml

    Get Device Identifier    ${currentXmlFile}
    Verify Identity Current  ${currentXmlFile}  ${Manufacturer}  ${Model}  ${SerialNumber}


Verify Initial Permissions
    [Setup]    Require test case    Obtain Target Parameters From Target

    ${nameofTest}=           Set Variable     VerifyInitialPermissions

    Log To Console    Initializing testcases
    Initialize lists of tests

    Log To Console    Running test

    Process Initial Permission Check    ${nameofTest}
