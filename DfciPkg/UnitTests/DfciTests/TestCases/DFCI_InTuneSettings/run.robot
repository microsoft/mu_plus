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
${TEST_CASE_DIR}        ${TEST_ROOT_DIR}${/}DFCI_InTuneSettings

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

    @{VTEST_01_SET1}=      Create List    Dfci.OnboardCameras.Enable    Enabled
    @{VTEST_01_SET2}=      Create List    Device.IRCamera.Enable        Enabled
    @{VTEST_01_SET3}=      Create List    Dfci.OnboardAudio.Enable      Enabled

    @{VTEST_01_CHECK1}=    Create List    Device.FrontCamera.Enable     Enabled
    @{VTEST_01_CHECK2}=    Create List    Device.IRCamera.Enable        Enabled
    @{VTEST_01_CHECK3}=    Create List    Device.RearCamera.Enable      Enabled
    @{VTEST_01_CHECK4}=    Create List    Dfci.OnboardCameras.Enable    Enabled
    @{VTEST_01_CHECK5}=    Create List    Dfci.OnboardAudio.Enable      Enabled

    ${VTEST_01_RESULTS}=   Create Dictionary     Dfci.OnboardCameras.Enable    ${STATUS_SUCCESS}
    Set To Dictionary      ${VTEST_01_RESULTS}   Device.IRCamera.Enable        ${STATUS_SUCCESS}
    Set To Dictionary      ${VTEST_01_RESULTS}   Dfci.OnboardAudio.Enable      ${STATUS_SUCCESS}

    @{VTEST_01_SETS}=      Create List    ${VTEST_01_SET1}    ${VTEST_01_SET2}    ${VTEST_01_SET3}
    @{VTEST_01_CHECKS}=    Create List    ${VTEST_01_CHECK1}    ${VTEST_01_CHECK2}    ${VTEST_01_CHECK3}    ${VTEST_01_CHECK4}    ${VTEST_01_CHECK5}


    # Testcase 2
    @{VTEST_02_SET1}=      Create List    Dfci.OnboardCameras.Enable    Disabled
    @{VTEST_02_SET2}=      Create List    Device.IRCamera.Enable        Enabled
    @{VTEST_02_SET3}=      Create List    Dfci.OnboardAudio.Enable      Disabled

    @{VTEST_02_CHECK1}=    Create List    Device.FrontCamera.Enable     Disabled
    @{VTEST_02_CHECK2}=    Create List    Device.IRCamera.Enable        Enabled
    @{VTEST_02_CHECK3}=    Create List    Device.RearCamera.Enable      Disabled
    @{VTEST_02_CHECK4}=    Create List    Dfci.OnboardCameras.Enable    Inconsistent
    @{VTEST_02_CHECK5}=    Create List    Dfci.OnboardAudio.Enable      Disabled

    ${VTEST_02_RESULTS}=   Create Dictionary     Dfci.OnboardCameras.Enable    ${STATUS_SUCCESS}
    Set To Dictionary      ${VTEST_02_RESULTS}   Device.IRCamera.Enable        ${STATUS_SUCCESS}
    Set To Dictionary      ${VTEST_02_RESULTS}   Dfci.OnboardAudio.Enable      ${STATUS_SUCCESS}

    @{VTEST_02_SETS}=      Create List    ${VTEST_02_SET1}    ${VTEST_02_SET2}    ${VTEST_02_SET3}
    @{VTEST_02_CHECKS}=    Create List    ${VTEST_02_CHECK1}    ${VTEST_02_CHECK2}    ${VTEST_02_CHECK3}    ${VTEST_02_CHECK4}    ${VTEST_02_CHECK5}


    # Testcase 3    V3 Set variables
    @{VTEST_03_SET1}=      Create List    Dfci3.AssetTag.String         ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890
    @{VTEST_03_SET2}=      Create List    Dfci3.OnboardWpbt.Enable      Enabled
    @{VTEST_03_SET3}=      Create List    Dfci3.ProcessorSMT.Enable     Enabled

    @{VTEST_03_CHECK1}=    Create List    Dfci3.AssetTag.String         ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890
    @{VTEST_03_CHECK2}=    Create List    Dfci3.OnboardWpbt.Enable      Enabled
    @{VTEST_03_CHECK3}=    Create List    Dfci3.ProcessorSMT.Enable     Enabled

    ${VTEST_03_RESULTS}    Create Dictionary     Dfci3.AssetTag.String         ${STATUS_SUCCESS}
    Set To Dictionary      ${VTEST_03_RESULTS}   Dfci3.OnboardWpbt.Enable      ${STATUS_SUCCESS}
    Set To Dictionary      ${VTEST_03_RESULTS}   Dfci3.ProcessorSMT.Enable     ${STATUS_SUCCESS}

    @{VTEST_03_SETS}=      Create List    ${VTEST_03_SET1}    ${VTEST_03_SET2}    ${VTEST_03_SET3}
    @{VTEST_03_CHECKS}=    Create List    ${VTEST_03_CHECK1}    ${VTEST_03_CHECK3}    ${VTEST_03_CHECK3}


    # Testcase 4    V3 Check Variables
    @{VTEST_04_SET1}=      Create List    Dfci3.AssetTag.String          ${EMPTY}
    @{VTEST_04_CHECK1}=    Create List    Dfci3.AssetTag.String          ${EMPTY}

    ${VTEST_04_RESULTS}    Create Dictionary    Dfci3.AssetTag.String         ${STATUS_SUCCESS}

    @{VTEST_04_SETS}=      Create List    ${VTEST_04_SET1}
    @{VTEST_04_CHECKS}=    Create List    ${VTEST_04_CHECK1}


    # Testcase 5    V3 Asset tag too long
    @{VTEST_05_SET1}=      Create List        Dfci3.AssetTag.String    ABCDEFGHIJKLMNOPQRSTUVWXYZ.1234567890
    @{VTEST_05_CHECK1}=    Create List        Dfci3.AssetTag.String    ${EMPTY}

    ${VTEST_05_RESULTS}=   Create Dictionary    Dfci3.AssetTag.String    ${STATUS_INVALID_PARAMETER}

    @{VTEST_05_SETS}=      Create List    ${VTEST_05_SET1}
    @{VTEST_05_CHECKS}=    Create List    ${VTEST_05_CHECK1}


    # Testcase 6    V3 Asset tag invalid characters
    @{VTEST_06_SET1}=      Create List        Dfci3.AssetTag.String   ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789%
    @{VTEST_06_CHECK1}=    Create List        Dfci3.AssetTag.String   ${EMPTY}

    ${VTEST_06_RESULTS}=   Create Dictionary  Dfci3.AssetTag.String   ${STATUS_INVALID_PARAMETER}

    @{VTEST_06_SETS}=      Create List    ${VTEST_06_SET1}
    @{VTEST_06_CHECKS}=    Create List    ${VTEST_06_CHECK1}


    # The full tests are here

    @{VTEST_01}=          Create List     Test1    ${VTEST_01_SETS}    ${VTEST_01_CHECKS}    ${VTEST_01_RESULTS}
    @{VTEST_02}=          Create List     Test2    ${VTEST_02_SETS}    ${VTEST_02_CHECKS}    ${VTEST_02_RESULTS}
    @{VTEST_03}=          Create List     Test3    ${VTEST_03_SETS}    ${VTEST_03_CHECKS}    ${VTEST_03_RESULTS}
    @{VTEST_04}=          Create List     Test4    ${VTEST_04_SETS}    ${VTEST_04_CHECKS}    ${VTEST_04_RESULTS}
    @{VTEST_05}=          Create List     Test5    ${VTEST_05_SETS}    ${VTEST_05_CHECKS}    ${VTEST_05_RESULTS}
    @{VTEST_06}=          Create List     Test6    ${VTEST_06_SETS}    ${VTEST_06_CHECKS}    ${VTEST_06_RESULTS}

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
    @{MASTER_TEST_V2}=   Create List  ${VTEST_01}  ${VTEST_02}
    @{MASTER_TEST_V3}=   Create List  ${VTEST_03}  ${VTEST_04}  ${VTEST_05}  ${VTEST_06}

    @{RESTORE_SETTINGS_V2}=   Create List  ${VTEST_01}
    @{RESTORE_SETTINGS_V3}=   Create List  ${VTEST_03}

    # Default to all the tests
    Set suite variable   ${MASTER_TEST_V2}
    Set suite variable   ${MASTER_TEST_V3}
    Set suite variable   ${RESTORE_SETTINGS_V2}
    Set suite variable   ${RESTORE_SETTINGS_V3}


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


Process Complete Testcase List V2

    Log To Console    Initializing testcases
    Initialize lists of tests

    Log To Console    Running test V2

    FOR    ${ATest}    IN    @{MASTER_TEST_V2}
        Process TestCases    @{ATest}
    END


Process Complete Testcase List V3

    Log To Console    Initializing testcases
    Initialize lists of tests

    Log To Console    Running test V3

    FOR    ${ATest}    IN    @{MASTER_TEST_V3}
        Process TestCases    @{ATest}
    END


Restore Settings V2

    Log To Console    Initializing testcases
    Initialize lists of tests

    Log To Console    Restoring settings V2

    FOR    ${ATest}    IN    @{RESTORE_SETTINGS_V2}
        Process TestCases    @{ATest}
    END


Restore Settings V3

    Log To Console    Initializing testcases
    Initialize lists of tests

    Log To Console    Restoring settings V3

    FOR    ${ATest}    IN    @{RESTORE_SETTINGS_V3}
        Process TestCases    @{ATest}
    END


Get the ending DFCI Settings
    ${nameofTest}=              Set Variable    DisplaySettingsAtExit

    ${currentIdXmlFile}=    Get The DFCI Settings    ${nameOfTest}


Clean Up Mailboxes
    Verify No Mailboxes Have Data
