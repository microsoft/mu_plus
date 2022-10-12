*** Settings ***
# @file
#
Documentation
...     DFCI InTune Settings test
...     This test suite checks the action of setting a setting, and the settings
...     of group settings.
...
...     NOTE:
...
...     The ASSET TAG test are dependent upon the DFCI PCD's being set to these values:
...
...         gDfciPkgTokenSpaceGuid.PcdDfciAssetTagChars|"0123456789-.ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"|VOID*|0x40000017
...         gDfciPkgTokenSpaceGuid.PcdDfciAssetTagLen | 36 | UINT16 | 0x40000018
...
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

MetaData
...     - Build a settings packet
...     - Send it to the system under test
...     - Reboot the system under test to apply the settings
...     - Get the new "Current Settings"
...     - Verify the settings that were changed

Library         OperatingSystem
Library         Process
Library         Collections

Library         Support${/}Python${/}DFCI_SupportLib.py
Library         Support${/}Python${/}DependencyLib.py
Library         Support${/}Python${/}SettingsXMLLib.py
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
${TEST_CASE_DIR}        ${TEST_ROOT_DIR}${/}DFCI_EnableTPM

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
#... Each setting/value pair is a list of two elements - the setting, and the value.
#... Establish a list of the settings pairs for the settings to be set, and another
#... list for the settings to be checked after the reboot.

    @{VTEST_01_SET1}=      Create List    Device.Tpm.Enable    Disabled

    @{VTEST_01_CHECK1}=    Create List    Device.Tpm.Enable    Disabled

    ${VTEST_01_RESULTS}=   Create Dictionary     Device.Tpm.Enable    ${STATUS_SUCCESS}

    @{VTEST_01_SETS}=      Create List    ${VTEST_01_SET1}
    @{VTEST_01_CHECKS}=    Create List    ${VTEST_01_CHECK1}

    @{VTEST_02_SET1}=      Create List    Device.Tpm.Enable    Enabled

    @{VTEST_02_CHECK1}=    Create List    Device.Tpm.Enable     Enabled

    ${VTEST_02_RESULTS}=   Create Dictionary     Device.Tpm.Enable    ${STATUS_SUCCESS}

    @{VTEST_02_SETS}=      Create List    ${VTEST_01_SET1}
    @{VTEST_02_CHECKS}=    Create List    ${VTEST_01_CHECK1}

    # The full tests are here

    @{VTEST_01}=          Create List     Test1    ${VTEST_01_SETS}    ${VTEST_01_CHECKS}    ${VTEST_01_RESULTS}
    @{VTEST_02}=          Create List     Test1    ${VTEST_02_SETS}    ${VTEST_02_CHECKS}    ${VTEST_02_RESULTS}

    # Export one master test variable.  Each entry in the MASTER TEST variable is a set of two lists and a dictionary of results.
    # Variables to be set before a reboot, and a set of variables to be checked after a reboot, and a dictionary of expected results
    # for each setting.  For two tests, that means:
    # 1. Test 1 Sets
    # 2. reboot
    # 3. Test 1 Checks with return codes
    # 4. Test 2 Sets
    # 5. reboot
    # 6. Test 2 Checks with return codes
    #
    @{MASTER_TEST_V1}=   Create List  ${VTEST_01}

    @{MASTER_TEST_V2}=   Create List  ${VTEST_02}


    # Default to all the tests
    Set suite variable   ${MASTER_TEST_V1}

    Set suite variable   ${MASTER_TEST_V2}

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
# create a settings payload, package it, send it to the system under test,
# restart the system to apply the settings, nd then validate that the
# settings in the checklist are correct.
#
    FOR    ${Testname}    ${Sets}    ${Checks}    ${Results}    IN    @{ATest}
        ${newSettingsXmlFile}=       Set Variable   ${TOOL_DATA_OUT_DIR}${/}${Testname}_NewSettings.xml
        ${currentSettingXmlFile}=    Set Variable   ${TOOL_DATA_OUT_DIR}${/}${Testname}_CurrentSettings.xml
        #
        #
        Log To Console    .
        Log To Console    Starting test ${Testname}
        #
        # Create the settings packet
        #
        Create Settings XML    ${newSettingsXmlFile}    2    2    ${Sets}
        File should Exist    ${newSettingsXmlFile}
        #
        #Enable the serial log if the platform supports it
        #
        Start SerialLog     ${BOOT_LOG_OUT_DIR}${/}${Testname}_ApplySettings.log
        #
        # Send the user(2) settings packet to the system under test
        #
        Process Settings Packet     ${Testname}  2  ${OLD_USER_PFX}  ${newSettingsXmlFile}  @{TARGET_PARAMETERS}
        #
        # Restart the system to apply the settings
        #
        Log To Console    Restarting the system under test
        Reboot System And Wait For System Online
        #
        #
        Get and Print Current Settings     ${currentSettingXmlFile}
        #
        # Ensure all of the setting set, were applied correctly
        #
        ${xmlSettingsRslt}=    Validate Settings Status    ${Testname}  2  ${STATUS_SUCCESS}  BASIC
        #
        # Validate the individual settings after the reboot
        #
        ${rc}=    Validate Current Settings    ${Testname}    ${currentSettingXmlFile}    ${Checks}
        Should Be True    ${rc}
        #
        ${rc}=    Check Setting Status By Dictionary    ${xmlSettingsRslt}    ${Results}
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


Process Complete Testcase List V1

    Log To Console    Initializing testcases
    Initialize lists of tests

    Log To Console    Running test V1, Disable TPM

    FOR    ${ATest}    IN    @{MASTER_TEST_V1}
        Process TestCases    @{ATest}
    END


Process Complete Testcase List V2

    Log To Console    Initializing testcases
    Initialize lists of tests

    Log To Console    Runnint test V2, Enable TPM

    FOR    ${ATest}    IN    @{MASTER_TEST_V2}
        Process TestCases    @{ATest}
    END


Get the ending DFCI Settings
    ${nameofTest}=              Set Variable    DisplaySettingsAtExit

    ${currentIdXmlFile}=    Get The DFCI Settings    ${nameOfTest}


Clean Up Mailboxes
    Verify No Mailboxes Have Data
