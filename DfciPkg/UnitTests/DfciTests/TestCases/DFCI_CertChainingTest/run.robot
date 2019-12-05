*** Settings ***
# @file
#
Documentation    This test suite uses verifies that the ZTD leaf cert will verify against a packet signed by the ZTD_CA.pfx.
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
Resource        Support${/}Robot${/}DFCI_Shared_Keywords.robot
Resource        Support${/}Robot${/}CertSupport.robot

#Import the platform specific log support
Resource        UefiSerial_Keywords.robot

# Use the following line for Python remote write to the UEFI Variables
Resource        Support${/}Robot${/}DFCI_VariableTransport.robot

Suite setup     Make Dfci Output
Test Teardown   Terminate All Processes


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
${TEST_CASE_DIR}        ${TEST_ROOT_DIR}${/}DFCI_CertChainingTest

${TOOL_DATA_OUT_DIR}    ${TEST_OUTPUT}${/}bindata
${TOOL_STD_OUT_DIR}     ${TEST_OUTPUT}${/}stdout
${BOOT_LOG_OUT_DIR}     ${TEST_OUTPUT}${/}uefilogs

${CERTS_DIR}            Certs

${TARGET_VERSION}       V2

${DDS_CA_THUMBPRINT}    'Thumbprint Not Set'
${MDM_CA_THUMBPRINT}    'Thumbprint Not Set'
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


Get the starting DFCI Settings
    [Setup]    Require test case    Ensure Mailboxes Are Clean

    ${nameofTest}=           Set Variable    DisplaySettingsAtStart
    ${ZtdLeafPfxFile}=       Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.pfx
    ${ZTD_LEAF_THUMBPRINT}=    Get Thumbprint From Pfx    ${ZtdLeafPfxFile}

    ${currentIdXmlFile}=     Get The DFCI Settings    ${nameOfTest}

    ${OwnerThumbprint}=    Get Thumbprint Element    ${currentIdxmlFile}  Owner
    ${UserThumbprint}=     Get Thumbprint Element    ${currentIdxmlFile}  User
    ${ZtdThumbprint}=      Get Thumbprint Element    ${currentIdxmlFile}  ZeroTouch

    Initialize Thumbprints    '${OwnerThumbprint}'    '${UserThumbprint}'

    Should Be True    '${OwnerThumbprint}' == 'Cert not installed'
    Should Be True    '${UserThumbprint}' == 'Cert not installed'

    Log To Console    .
    Log To Console    The following test insures that the test ZTD_Leaf.cer
    Log To Console    is the cert installed.  If not, this test cannot be run.
    Should Be True    '${ZtdThumbprint}' == ${ZTD_LEAF_THUMBPRINT}


Obtain Target Parameters From Target
    [Setup]    Require test case    Get the starting DFCI Settings

    ${nameofTest}=    Set Variable     GetParameters
    ${SerialNumber}=  Get System Under Test SerialNumber
    ${Manufacturer}=  Get System Under Test Manufacturer
    ${Model}=         Get System Under Test ProductName
    @{TARGET_PARAMETERS}=    Build Target Parameters  ${TARGET_VERSION}  ${SerialNumber}  ${Manufacturer}  ${Model}
    Set Suite Variable  @{TARGET_PARAMETERS}

    ${currentXmlFile}=    Set Variable   ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_UefiDeviceId.xml

    Get Device Identifier    ${currentXmlFile}
    Verify Identity Current  ${currentXmlFile}  ${Manufacturer}  ${Model}  ${SerialNumber}


Attempt Enroll DDS CA cert Signed by ZTD_CA to System Being Enrolled
    [Setup]    Require test case    Obtain Target Parameters From Target

    ${nameofTest}=        Set Variable    DDSwithBadKey
    ${ownerPfxFile}=      Set Variable    ${CERTS_DIR}${/}DDS_CA.pfx
    ${ownerCertFile}=     Set Variable    ${CERTS_DIR}${/}DDS_CA.cer
    ${signerPfxFile}=     Set Variable    ${CERTS_DIR}${/}ZTD_CA.pfx
    ${currentIdxmlFile}=  Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentIdentities.xml

    Start SerialLog     ${BOOT_LOG_OUT_DIR}${/}DDSwithBadKey.log

    Process Provision Packet     ${nameofTest}  1  ${signerPfxFile}  ${ownerPfxFile}  ${ownerCertFile}  ${OWNER_KEY_INDEX}  @{TARGET_PARAMETERS}

    ${OwnerCertThumbprint}=    Get Thumbprint From Pfx    ${ownerPfxFile}
    Log To Console    .
    Log To Console    ${OwnerCertThumbprint}
    Log To Console    Should be prompted,  CANCEL THE PROMPT

    Reboot System And Wait For System Online

    Validate Provision Status    ${nameofTest}  1  ${STATUS_ABORTED}
    Get and Print Current Identities   ${currentIdxmlFile}

    ${rc}=    Get Thumbprint Element    ${currentIdxmlFile}    ZeroTouch
    Should Be True    '${rc}' != 'Cert not installed'

    ${rc}=    Get Thumbprint Element    ${currentIdxmlFile}    Owner
    Should Be True    '${rc}' == 'Cert not installed'

    ${rc}=    Get Thumbprint Element    ${currentIdxmlFile}    User
    Should Be True    '${rc}' == 'Cert not installed'


Enroll DDS CA cert Signed by ZTD_Leaf to System Being Enrolled
    [Setup]    Require test case    Attempt Enroll DDS CA cert Signed by ZTD_CA to System Being Enrolled

    ${nameofTest}=          Set Variable  DDSwithGoodKey
    ${ownerCertFile}=       Set Variable  ${CERTS_DIR}${/}DDS_CA.cer
    ${ZTDsignerPfxFile}=    Set Variable  ${CERTS_DIR}${/}ZTD_Leaf.pfx
    ${DDSsignerPfxFile}=    Set Variable  ${CERTS_DIR}${/}DDS_Leaf.pfx
    ${xmlPayloadFile}=      Set Variable  ${TEST_CASE_DIR}${/}DfciPermission.xml
    ${currentIdxmlFile}=    Set Variable  ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentIdentities.xml
    ${currentPermxmlFile}=  Set Variable  ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentPermission.xml

    # The DDS Owner Enroll is signed by the ZTD_Leaf in order to have zero touch enroll
    Process Provision Packet     ${nameofTest}  1  ${ZTDsignerPfxFile}  ${DDSsignerPfxFile}  ${ownerCertFile}  ${OWNER_KEY_INDEX}  @{TARGET_PARAMETERS}

    # Must grant permissions to enroll the MDM.  These permissions need to be signed
    # by the owner key, in this case DDS_Leaf
    Process Permission Packet    ${nameofTest}  1  ${DDSsignerPfxFile}  ${xmlPayloadFile}  @{TARGET_PARAMETERS}

    Start SerialLog     ${BOOT_LOG_OUT_DIR}${/}DDSwithGoodKey.log

    Reboot System And Wait For System Online

    Get and Print Current Identities   ${currentIdxmlFile}
    Get and Print Current Permissions  ${currentPermxmlFile}

    Validate Provision Status    ${nameofTest}  1  ${STATUS_SUCCESS}
    Validate Permission Status   ${nameofTest}  1  ${STATUS_SUCCESS}

    ${rc}=    Get Thumbprint Element    ${currentIdxmlFile}    ZeroTouch
    Should Be True    '${rc}' != 'Cert not installed'

    ${rc}=    Get Thumbprint Element    ${currentIdxmlFile}    Owner
    Should Be True    '${rc}' != 'Cert not installed'

    ${rc}=    Get Thumbprint Element    ${currentIdxmlFile}    User
    Should Be True    '${rc}' == 'Cert not installed'


Enroll MDM CA cert Signed by DDS_Leaf to System Being Enrolled
    [Setup]    Require test case    Enroll DDS CA cert Signed by ZTD_Leaf to System Being Enrolled

    ${nameofTest}=       Set Variable    MDMwithGoodKey
    ${ownerPfxFile}=     Set Variable    ${CERTS_DIR}${/}MDM_CA.pfx
    ${ownerCertFile}=    Set Variable    ${CERTS_DIR}${/}MDM_CA.cer
    ${signerPfxFile}=    Set Variable    ${CERTS_DIR}${/}DDS_Leaf.pfx
    ${currentIdxmlFile}=  Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentIdentities.xml

    Process Provision Packet     ${nameofTest}  1  ${signerPfxFile}  ${ownerPfxFile}  ${ownerCertFile}  ${USER_KEY_INDEX}  @{TARGET_PARAMETERS}

    Start SerialLog     ${BOOT_LOG_OUT_DIR}${/}MDMwithGoodKey.log

    Reboot System And Wait For System Online

    Validate Provision Status    ${nameofTest}  1  ${STATUS_SUCCESS}
    Get and Print Current Identities   ${currentIdxmlFile}

    ${rc}=    Get Thumbprint Element    ${currentIdxmlFile}    ZeroTouch
    Should Be True    '${rc}' != 'Cert not installed'

    ${rc}=    Get Thumbprint Element    ${currentIdxmlFile}    Owner
    Should Be True    '${rc}' != 'Cert not installed'

    ${rc}=    Get Thumbprint Element    ${currentIdxmlFile}    User
    Should Be True    '${rc}' != 'Cert not installed'


Send DDS Unenroll signed by DDS_Leaf to complete UnEnroll
    [Setup]    Require test case    Enroll MDM CA cert Signed by DDS_Leaf to System Being Enrolled

    ${nameofTest}=       Set Variable    UnEnrollDDS
    ${ownerPfxFile}=     Set Variable    ${CERTS_DIR}${/}DDS_Leaf.pfx
    ${currentIdxmlFile}=  Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentIdentities.xml

    Process UnEnroll Packet     ${nameofTest}  1  ${ownerPfxFile}  ${OWNER_KEY_INDEX}  @{TARGET_PARAMETERS}

    Start SerialLog     ${BOOT_LOG_OUT_DIR}${/}UnEnrollDDS.log

    Reboot System And Wait For System Online

    Validate UnEnroll Status    ${nameofTest}  1  ${STATUS_SUCCESS}
    Get and Print Current Identities   ${currentIdxmlFile}

    ${rc}=    Get Thumbprint Element    ${currentIdxmlFile}    ZeroTouch
    Should Be True    '${rc}' != 'Cert not installed'

    ${rc}=    Get Thumbprint Element    ${currentIdxmlFile}    Owner
    Should Be True    '${rc}' == 'Cert not installed'

    ${rc}=    Get Thumbprint Element    ${currentIdxmlFile}    User
    Should Be True    '${rc}' == 'Cert not installed'


Get the ending DFCI Settings
    ${nameofTest}=              Set Variable    DisplaySettingsAtExit
    ${currentIdxmlFile}=        Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentIdentities.xml
    ${currentPermxmlFile}=      Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentPermission.xml
    ${currentSettingsxmlFile}=  Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentSettings.xml

    Get and Print Current Identities   ${currentIdxmlFile}

    Get and Print Current Permissions  ${currentPermxmlFile}

    Get and Print Current Settings     ${currentSettingsxmlFile}

    ${rc}=   Get Thumbprint Element    ${currentIdxmlFile}    ZeroTouch
    Should Be True    '${rc}' == ${ZTD_LEAF_THUMBPRINT}

    ${rc}=   Get Thumbprint Element    ${currentIdxmlFile}    Owner
    Should Be True    '${rc}' == 'Cert not installed'

    ${rc}=    Get Thumbprint Element    ${currentIdxmlFile}    User
    Should Be True    '${rc}' == 'Cert not installed'
