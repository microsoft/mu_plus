*** Settings ***
# @file
#
Documentation    This test suite enrolls an owner and user into DFCI.
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

Suite setup     Make Dfci Output
Test Teardown   Terminate All Processes    kill=True


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
${TEST_CASE_DIR}        ${TEST_ROOT_DIR}${/}DFCI_InTuneEnroll

${TOOL_DATA_OUT_DIR}    ${TEST_OUTPUT}${/}bindata
${TOOL_STD_OUT_DIR}     ${TEST_OUTPUT}${/}stdout
${BOOT_LOG_OUT_DIR}     ${TEST_OUTPUT}${/}uefilogs

${CERTS_DIR}            Certs

${TARGET_VERSION}       V2

${DDS_CA_THUMBPRINT}    'Thumbprint Not Set'
${MDM_CA_THUMBPRINT}    'Thumbprint Not Set'


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


Get the starting DFCI Settings
    [Setup]    Require test case    Ensure Mailboxes Are Clean
    ${nameofTest}=              Set Variable    DisplaySettingsAtStart

    ${currentIdXmlFile}=     Get The DFCI Settings    ${nameOfTest}

    ${OwnerThumbprint}=    Get Thumbprint Element    ${currentIdxmlFile}  Owner
    ${UserThumbprint}=     Get Thumbprint Element    ${currentIdxmlFile}  User

    Initialize Thumbprints    '${OwnerThumbprint}'    '${UserThumbprint}'

    Should Be True    '${OwnerThumbprint}' == 'Cert not installed'
    Should Be True    '${UserThumbprint}' == 'Cert not installed'


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

Send Owner Enroll Packet to System Being Enrolled
    [Setup]    Require test case    Obtain Target Parameters From Target

    ${nameofTest}=       Set Variable    OwnerEnroll

    Process Provision Packet     ${nameofTest}  1  ${ZTD_LEAF_PFX}  ${NEW_OWNER_PFX}  ${NEW_OWNER_CERT}  ${OWNER_KEY_INDEX}  @{TARGET_PARAMETERS}


Send Owner Permission Packet to Enrolled System
    [Setup]    Require test case    Send Owner Enroll Packet to System Being Enrolled
    ${nameofTest}=         Set Variable    OwnerPermissions
    ${xmlPayloadFile}=     Set Variable    ${TEST_CASE_DIR}${/}DfciPermission.xml

    Process Permission Packet     ${nameofTest}  1  ${NEW_OWNER_PFX}  ${xmlPayloadFile}  @{TARGET_PARAMETERS}


Send User Enroll to System Being Enrolled
    [Setup]    Require test case    Send Owner Permission Packet to Enrolled System

    # This adds the User key, packet signed by ownerkey

    ${nameofTest}=       Set Variable    UserEnroll

    Process Provision Packet     ${nameofTest}  2  ${NEW_OWNER_PFX}  ${NEW_USER_PFX}  ${NEW_USER_CERT}  ${USER_KEY_INDEX}  @{TARGET_PARAMETERS}


Send User Permission Packet to Enrolled System
    [Setup]    Require test case    Send User Enroll to System Being Enrolled
    #Files for the Permission package
    ${nameofTest}=         Set Variable    UserPermissions
    ${xmlPayloadFile}=     Set Variable    ${TEST_CASE_DIR}${/}DfciPermission2.xml

    Process Permission Packet     ${nameofTest}  2  ${NEW_USER_PFX}  ${xmlPayloadFile}  @{TARGET_PARAMETERS}


Send Owner Settings Packet to Enrolled System
    [Setup]    Require test case    Send User Permission Packet to Enrolled System
    #Initial settings for Enrolled System
    ${nameofTest}=      Set Variable    OwnerSettings
    ${xmlPayloadFile}=  Set Variable    ${TEST_CASE_DIR}${/}DfciSettings.xml

    Process Settings Packet     ${nameofTest}  1  ${NEW_OWNER_PFX}  ${xmlPayloadFile}  @{TARGET_PARAMETERS}


Send User Settings Packet to Enrolled System
    [Setup]    Require test case    Send Owner Settings Packet to Enrolled System
    #Initial settings for Enrolled System
    ${nameofTest}=      Set Variable    UserSettings
    ${xmlPayloadFile}=  Set Variable    ${TEST_CASE_DIR}${/}DfciSettings2.xml

    Process Settings Packet     ${nameofTest}  2  ${NEW_USER_PFX}  ${xmlPayloadFile}  @{TARGET_PARAMETERS}


Restart System to Apply Enrollment
# Start serial log to capture UEFI log during the restart
    [Setup]    Require test case    Send User Settings Packet to Enrolled System

    Start SerialLog     ${BOOT_LOG_OUT_DIR}${/}ApplyEnrollment.log

    Log To Console    Restart
    Log To Console    If test Ztd cert is not installed, you will be prompted
    Log To Console    for the last two characters of the following thumbprint:
    Log To Console    Enrolling with ${DDS_CA_THUMBPRINT}

    Reboot System And Wait For System Online


Verify Owner Enrolled System Identity Results
    ${nameofTest}=   Set Variable    OwnerEnroll

    Validate Provision Status    ${nameofTest}  1  ${STATUS_SUCCESS}


Verify Owner Enrolled System Permission Results
    ${nameofTest}=   Set Variable    OwnerPermissions

    ${xmlPermissionsRslt}=    Validate Permission Status    ${nameofTest}  1  ${STATUS_SUCCESS}
    ${rc}    Check All Permission Status    ${xmlPermissionsRslt}    ${STATUS_SUCCESS}
    Should Be True    ${rc}


Verify User Enrolled System Identity Results
    ${nameofTest}=   Set Variable    UserEnroll

    Validate Provision Status    ${nameofTest}  2  ${STATUS_SUCCESS}


Verify User Enrolled System Permission Results
    ${nameofTest}=   Set Variable    UserPermissions

    ${xmlPermissionsRslt}=    Validate Permission Status    ${nameofTest}  2  ${STATUS_SUCCESS}
    ${rc}    Check All Permission Status    ${xmlPermissionsRslt}    ${STATUS_SUCCESS}
    Should Be True    ${rc}


Verify Owner Enrolled System Settings Results
    ${nameofTest}=   Set Variable    OwnerSettings

    ${xmlPermissionsRslt}=   Validate Settings Status    ${nameofTest}    1    ${STATUS_SUCCESS}  FULL
    ${rc}    Check All Setting Status    ${xmlPermissionsRslt}    ${STATUS_SUCCESS}
    Should Be True    ${rc}

Verify User Enrolled System Settings Results
    ${nameofTest}=   Set Variable    UserSettings

    ${xmlPermissionsRslt}=   Validate Settings Status    ${nameofTest}    2    ${STATUS_SUCCESS}  FULL
    ${rc}    Check All Setting Status    ${xmlPermissionsRslt}    ${STATUS_SUCCESS}
    Should Be True    ${rc}


Get the ending DFCI Settings
    ${nameofTest}=              Set Variable    DisplaySettingsAtExit

    ${currentIdXmlFile}=    Get The DFCI Settings    ${nameOfTest}

    ${OwnerThumbprint}     Get Thumbprint From Pfx    ${NEW_OWNER_CERT}
    ${rc}=   Get Thumbprint Element    ${currentIdxmlFile}    Owner
    Should Be True    '${rc}' == '${OwnerThumbprint}'

    ${UserThumbprint}     Get Thumbprint From Pfx    ${NEW_USER_CERT}
    ${rc}=   Get Thumbprint Element    ${currentIdxmlFile}    User
    Should Be True    '${rc}' == '${UserThumbprint}'


Clean Up Mailboxes
    Verify No Mailboxes Have Data


Restart System to Verify Device Setting
# Start serial log to capture UEFI log during the restart
    [Setup]    Require test case    Restart System to Apply Enrollment

    Start SerialLog     ${BOOT_LOG_OUT_DIR}${/}VerifyDeviceSettings.log

    Log To Console    Restarting to firmware
    Log To Console    Check the device settings to insure that all
    Log To Console    of the camera devices, and radio devices, are
    Log To Console    off and grayed out. Other devices should be
    Log To Console    available for the user to control.

    Reboot System To Firmware And Wait For System Online
