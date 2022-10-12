*** Settings ***
# @file
#
Documentation    This test suite unenrolls the system after being enrolled with InTuneEnroll.
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

Library         OperatingSystem
Library         Process

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
${TEST_CASE_DIR}        ${TEST_ROOT_DIR}${/}DFCI_InTuneUnenroll

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

    Log To Console    .
    Log To Console    ${SUITE SOURCE}


Get the current DFCI Settings Before Unenroll
    ${nameofTest}=              Set Variable    BeforeUnenroll

    ${currentIdXmlFile}=     Get The DFCI Settings    ${nameOfTest}

    ${OwnerThumbprint}=    Get Thumbprint Element    ${currentIdxmlFile}  Owner
    ${UserThumbprint}=     Get Thumbprint Element    ${currentIdxmlFile}  User
    Should Be True    '${OwnerThumbprint}' != 'Cert not installed'
    Should Be True    '${UserThumbprint}' != 'Cert not installed'

    Initialize Thumbprints    '${OwnerThumbprint}'    '${UserThumbprint}'


Obtain Target Parameters From Target
    [Setup]    Require test case    Get the current DFCI Settings Before UnEnroll
    ${nameofTest}=    Set Variable     GetParameters
    ${SerialNumber}=  Get System Under Test SerialNumber
    ${Manufacturer}=  Get System Under Test Manufacturer
    ${Model}=         Get System Under Test ProductName
    @{TARGET_PARAMETERS}=    Build Target Parameters  ${TARGET_VERSION}  ${SerialNumber}  ${Manufacturer}  ${Model}
    Set Suite Variable  @{TARGET_PARAMETERS}

    ${currentXmlFile}=    Set Variable   ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_UefiDeviceId.xml

    Get Device Identifier    ${currentXmlFile}
    Verify Identity Current  ${currentXmlFile}  ${Manufacturer}  ${Model}  ${SerialNumber}


Send User Settings Packet to Enrolled System
    [Setup]    Require test case    Obtain Target Parameters From Target
    #Initial settings for Enrolled System
    ${nameofTest}=      Set Variable    UserSettings
    ${xmlPayloadFile}=  Set Variable    ${TEST_CASE_DIR}${/}DfciSettings2.xml


    Process Settings Packet     ${nameofTest}  2  ${OLD_USER_PFX}  ${xmlPayloadFile}  @{TARGET_PARAMETERS}


Send Owner Unenroll to System
    [Setup]    Require test case    Send User Settings Packet to Enrolled System
    ${nameofTest}=       Set Variable    Unenroll

    Process UnEnroll Packet     ${nameofTest}  1  ${OLD_OWNER_PFX}  ${OWNER_KEY_INDEX}  @{TARGET_PARAMETERS}


Restart System to UnEnroll
#Documentation  Only run the restart test if all the previous setup operations passed
    [Setup]    Require test case    Send Owner Unenroll to System

    Start SerialLog     ${BOOT_LOG_OUT_DIR}${/}InTuneUnenroll.log

    Reboot System And Wait For System Online


Verify User Enrolled System Settings Results
    ${nameofTest}=   Set Variable    UserSettings

    ${xmlUserSettingsRslt}=   Validate Settings Status    ${nameofTest}    2    ${STATUS_SUCCESS}  FULL
    ${rc}    Check All Setting Status    ${xmlUserSettingsRslt}    ${STATUS_SUCCESS}
    Should Be True    ${rc}


Verify Owner UnEnroll Results
    ${nameofTest}=   Set Variable    Unenroll

    Validate UnEnroll Status    ${nameofTest}  1  ${STATUS_SUCCESS}


Get the current DFCI Settings after Unenroll
    ${nameofTest}=              Set Variable    AfterUnenroll

    ${currentIdXmlFile}=    Get The DFCI Settings    ${nameOfTest}

    ${rc}=   Get Thumbprint Element    ${currentIdxmlFile}    Owner
    Should Be True    '${rc}' == 'Cert not installed'

    ${rc}=   Get Thumbprint Element    ${currentIdxmlFile}    User
    Should Be True    '${rc}' == 'Cert not installed'


Verify Settings Returned To Defaults
    ${nameofTest}=              Set Variable   VerifyDefaults
    ${currentSettingXmlFile}=   Set Variable   ${TOOL_DATA_OUT_DIR}${/}${Testname}_CurrentSettings.xml


#Documentation  Verify the no preboot UI settings are back to default
    @{RTD_CHECK01}=   Create List   Dfci.HttpsCert.Binary              ${EMPTY}
    @{RTD_CHECK02}=   Create List   Dfci.RecoveryBootstrapUrl.String   ${EMPTY}
    @{RTD_CHECK03}=   Create List   Dfci.RecoveryUrl.String            ${EMPTY}
    @{RTD_CHECK04}=   Create List   Dfci.RegistrationId.String         ${EMPTY}
    @{RTD_CHECK05}=   Create List   Dfci.TenantId.String               ${EMPTY}
    @{RTD_CHECK06}=   Create List   Dfci3.AssetTag.String              ${EMPTY}
    @{RTD_CHECK07}=   Create List   Dfci3.OnboardWpbt.Enable           Enabled
    @{RTD_CHECK08}=   Create List   MDM.FriendlyName.String            ${EMPTY}
    @{RTD_CHECK09}=   Create List   MDM.TenantName.String              ${EMPTY}

#Documentation  Verify the settings that require explicit reset reset to default
    @{RTD_CHECK10}=   Create List   Dfci.OnboardCameras.Enable         Enabled
    @{RTD_CHECK11}=   Create List   Dfci.OnboardRadios.Enable          Enabled
    @{RTD_CHECK12}=   Create List   Dfci.BootExternalMedia.Enable      Enabled
    @{RTD_CHECK13}=   Create List   Dfci3.ProcessorSMT.Enable          Enabled
    @{RTD_CHECK14}=   Create List   Dfci3.AssetTag.String              ${EMPTY}

    @{RTD_CHECKS}=    Create List   ${RTD_CHECK01}
...                                 ${RTD_CHECK02}
...                                 ${RTD_CHECK03}
...                                 ${RTD_CHECK04}
...                                 ${RTD_CHECK05}
...                                 ${RTD_CHECK06}
...                                 ${RTD_CHECK07}
...                                 ${RTD_CHECK08}
...                                 ${RTD_CHECK09}
...                                 ${RTD_CHECK10}
...                                 ${RTD_CHECK11}
...                                 ${RTD_CHECK12}
...                                 ${RTD_CHECK13}
...                                 ${RTD_CHECK14}

    Get and Print Current Settings     ${currentSettingXmlFile}

    ${rc}=    Validate Current Settings    ${Testname}    ${currentSettingXmlFile}    ${RTD_CHECKS}
    Should Be True    ${rc}


Clean Up Mailboxes
    Verify No Mailboxes Have Data
