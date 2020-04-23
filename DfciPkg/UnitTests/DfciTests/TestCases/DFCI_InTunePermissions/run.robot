*** Settings ***
# @file
#
Documentation
...     DFCI InTune Permissions test
...     This test suite checks the action of setting a permission and the various
...     PMASK and DMASK combinations.
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

MetaData
...     - Build a permissions packet
...     - Send it to the system under test
...     - Reboot the system under test to apply the permissions
...     - Get the new "Current Permissions"
...     - Verify the permissions are currect

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
${TEST_CASE_DIR}        ${TEST_ROOT_DIR}${/}DFCI_InTunePermissions

${TOOL_DATA_OUT_DIR}    ${TEST_OUTPUT}${/}bindata
${TOOL_STD_OUT_DIR}     ${TEST_OUTPUT}${/}stdout
${BOOT_LOG_OUT_DIR}     ${TEST_OUTPUT}${/}uefilogs

${CERTS_DIR}            Certs

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

    @{VTEST_01_SET1}=      Create List    Dfci.OnboardCameras.Enable    64    64
    @{VTEST_01_SET2}=      Create List    Dfci.OnboardAudio.Enable      64    64
    @{VTEST_01_CHECK1}=    Create List    Dfci.OnboardCameras.Enable    64    64
    @{VTEST_01_CHECK2}=    Create List    Dfci.OnboardAudio.Enable      64    64
    @{VTEST_01_SETS}=      Create List    ${VTEST_01_SET1}    ${VTEST_01_SET2}
    @{VTEST_01_CHECKS}=    Create List    ${VTEST_01_CHECK1}    ${VTEST_01_CHECK2}

    # Testcase 2
    @{VTEST_02_SET1}=      Create List    Dfci.OnboardRadios.Enable     64    64
    @{VTEST_02_CHECK1}=    Create List    Dfci.OnboardRadios.Enable     64    64
    @{VTEST_02_CHECK2}=    Create List    Dfci.OnboardCameras.Enable    ${None}
    @{VTEST_02_CHECK3}=    Create List    Dfci.OnboardAudio.Enable      ${None}
    @{VTEST_02_SETS}=      Create List    ${VTEST_02_SET1}
    @{VTEST_02_CHECKS}=    Create List    ${VTEST_02_CHECK1}    ${VTEST_02_CHECK2}    ${VTEST_02_CHECK3}

    # Other tests here

    @{VTEST_01}=          Create List     Test1    ${VTEST_01_SETS}    ${VTEST_01_CHECKS}    192    192    192    192
    @{VTEST_02}=          Create List     Test2    ${VTEST_02_SETS}    ${VTEST_02_CHECKS}    193    193    193    193

    # Export one master test variable.  Each entry in the MASTER TEST variable is a set of two lists - Variables
    # to be set before a reboot, and a set of variables to be checked after a reboot.  For two tests, that means:
    # 1. Test 1 Sets
    # 2. reboot
    # 3. Test 1 Checks
    # 4. Test 2 Sets
    # 5. reboot
    # 6. Test 2 Checks
    #
    @{MASTER_TEST}=        Create List    ${VTEST_01}    ${VTEST_02}
    Set suite variable     ${MASTER_TEST}

#
#        Use the following to ensure the lists are built correctly
#
#        Log To Console    .
#        Log To Console    ${VTEST_01_SET1}
#        Log To Console    ${VTEST_01_SET2}
#        Log To Console    ${VTEST_01_SETS}
#        Log To Console    ${VTEST_01_CHECKS}
#        Log To Console    ${VTEST_01}



#
#
#
Process TestCases
    [Arguments]    @{ATest}

#
# This function iterates over each of the test cases.  For each test case,
# create a permissions payload, package it, send it to the system under test,
# restart the system to apply the permissions, and then validate that the
# permissions in the checklist are correct.
#
    FOR    ${Testname}    ${Sets}    ${Checks}    ${PMask}    ${DMask}    ${CheckPMask}    ${CheckDMask}    IN    @{ATest}
        ${newPermissionsXmlFile}=        Set Variable   ${TOOL_DATA_OUT_DIR}${/}${Testname}_NewPermissions.xml
        ${currentPermissionsXmlFile}=    Set Variable   ${TOOL_DATA_OUT_DIR}${/}${Testname}_CurrentPermissions.xml
        #
        #
        Log To Console    .
        Log To Console    Starting test ${Testname}
        #
        # Create the permissions packet
        #
        Create Permissions XML    ${newPermissionsXmlFile}    2    2    ${PMask}    ${DMask}    ${Sets}
        File should Exist    ${newPermissionsXmlFile}
        #
        #Enable the serial log if the platform supports it
        #
        Start SerialLog     ${BOOT_LOG_OUT_DIR}${/}${Testname}_ApplyPermissions.log
        #
        # Send the user(2) permissions packet to the system under test
        #
        Process Permission Packet     ${Testname}  2  ${OLD_USER_PFX}  ${newPermissionsXmlFile}  @{TARGET_PARAMETERS}
        #
        # Restart the system to apply the permissions
        #
        Log To Console    Restarting the system under test
        Reboot System And Wait For System Online
        #
        #
        Get and Print Current Permissions     ${currentPermissionsXmlFile}
        #
        # Ensure all of the permissions set, were applied correctly
        #
        ${xmlPermissionsRslt}=    Validate Permission Status    ${Testname}  2  ${STATUS_SUCCESS}
        #
        # Validate the individual settings after the reboot
        #
        ${rc}=   Validate Current Permission Defaults    ${Testname}    ${currentPermissionsXmlFile}    ${CheckPMask}    ${CheckDMask}
        Should Be True    ${rc}
        #
        ${rc}=    Validate Current Permissions    ${Testname}    ${currentPermissionsXmlFile}    ${Checks}
        Should Be True    ${rc}
        #
        ${rc}    Check All Permission Status    ${xmlPermissionsRslt}    ${STATUS_SUCCESS}
        Should Be True    ${rc}
    END


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


#------------------------------------------------------------------*
#  Test Cases                                                      *
#------------------------------------------------------------------*
*** Test Cases ***


Ensure Mailboxes Are Clean
#Documentation Ensure all mailboxes are clear at the beginning of a test.  If there are any mailboxes that have an element, a previous test failed.
    Verify No Mailboxes Have Data

    Log To Console    .
    Log To Console    ${SUITE SOURCE}


Get the starting DFCI Settings
    [Setup]    Require test case    Ensure Mailboxes Are Clean
    ${nameofTest}=              Set Variable    DisplaySettingsAtStart

    ${currentIdXmlFile}=     Get The DFCI Settings    ${nameOfTest}

    ${OwnerThumbprint}=    Get Thumbprint Element    ${currentIdxmlFile}  Owner
    ${UserThumbprint}=     Get Thumbprint Element    ${currentIdxmlFile}  User

    Initialize Thumbprints    '${OwnerThumbprint}'    '${UserThumbprint}'

    Should Be True    '${OwnerThumbprint}' != 'Cert not installed'
    Should Be True    '${UserThumbprint}' != 'Cert not installed'


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


Process Complete Testcase List

    Log To Console    Initializing testcases
    Initialize lists of tests

    Log To Console    Running test

    FOR    ${ATest}    IN    @{MASTER_TEST}
        Process TestCases    @{ATest}
    END


Get the ending DFCI Settings
    ${nameofTest}=              Set Variable    DisplaySettingsAtExit

    ${currentIdXmlFile}=    Get The DFCI Settings    ${nameOfTest}


Clean Up Mailboxes
    Verify No Mailboxes Have Data
