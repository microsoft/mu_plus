*** Settings ***
# @file
#
Documentation    This test suite verifies the different signing methods for DFCI packets.
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

Library         OperatingSystem
Library         Process

Library         Support${/}Python${/}DFCI_SupportLib.py
Library         Support${/}Python${/}DependencyLib.py

#Import the Generic Shared keywords
Resource        Support${/}Robot${/}DFCI_Shared_Paths.robot
Resource        Support${/}Robot${/}DFCI_Shared_Keywords2.robot
Resource        Support${/}Robot${/}DFCI_Shared_Paths.robot
Resource        Support${/}Robot${/}CertSupport.robot

#Import the platform specific log support
Resource        UefiSerial_Keywords.robot

Suite setup     Make Dfci Output
Test Teardown   Terminate All Processes    kill=True


*** Variables ***
#test output directory for data from this test run.
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

${TOOL_DFCI_VERIFY}     Support${/}Tools${/}DfciVerify.exe


*** Keywords ***


*** Test Cases ***

Get the starting DFCI Settings

    ${ZtdLeafPfxFile}=       Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.pfx
    ${ZTD_LEAF_THUMBPRINT}=  Get Thumbprint From Pfx    ${ZtdLeafPfxFile}


Obtain Target Parameters From Target
    [Setup]    Require test case    Get the starting DFCI Settings

    ${nameofTest}=         Set Variable     GetParameters
    ${SerialNumber}=       Set Variable     1234567890
    ${Manufacturer}=       Set Variable     DFCI_TEST_SHOP
    ${Model}=              Set Variable     CERT_CHAIN_TEST
    @{TARGET_PARAMETERS}=  Build Target Parameters  ${TARGET_VERSION}  ${SerialNumber}  ${Manufacturer}  ${Model}
    Set Suite Variable     @{TARGET_PARAMETERS}


Build Enroll of DDS_CA cert, signed by ZTD_CA.pfx, and verify
    [Setup]    Require test case    Obtain Target Parameters From Target

    ${nameofTest}=    Set Variable    DDSwithBadKey
    ${ownerPfxFile}=  Set Variable    ${CERTS_DIR}${/}DDS_Leaf.pfx
    ${ownerCerFile}=  Set Variable    ${CERTS_DIR}${/}DDS_CA.cer
    ${signPfxFile}=   Set Variable    ${CERTS_DIR}${/}ZTD_CA.pfx
    ${signCerFile}=   Set Variable    ${CERTS_DIR}${/}ZTD_CA.cer
    ${uefiPfxFile}=   Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.pfx
    ${uefiCerFile}=   Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.cer
    ${altPfxFile}=    Set Variable    ${CERTS_DIR}${/}ZTD_Leaf3.pfx
    ${altCerFile}=    Set Variable    ${CERTS_DIR}${/}ZTD_Leaf3.cer

    File Should Exist  ${ownerPfxFile}
    File Should Exist  ${ownerCerFile}
    File Should Exist  ${signPfxFile}
    File Should Exist  ${signCerFile}
    File Should Exist  ${uefiPfxFile}
    File Should Exist  ${uefiCerFile}
    File Should Exist  ${altCerFile}

    ${currentIdxmlFile}=  Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentIdentities.xml
    ${binPackageFile}=    Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_Provision_apply.bin
    ${applyPackageFile}=  Set Variable    ${TOOL_STD_OUT_DIR}${/}${TestName}_Provision_apply.log

    Create Dfci Provisioning Package  ${binPackageFile}  ${signPfxFile}  ${ownerPfxFile}  ${ownerCerFile}  ${OWNER_KEY_INDEX}  @{TARGET_PARAMETERS}
    Print Provisioning Package        ${binPackageFile}  ${applyPackageFile}

    #
    # The following verification operations are expected to pass
    #

    # Verify that the package can be verified with the signed CA.cer
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${signCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the owner .cer
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${ownerCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the UEFI Leaf cer file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${uefiCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the alt Leaf cer file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${altCerFile}
    Should Be True  ${Result.rc} == 0

    #
    # The following verification operations are expected to fail
    #

    # Verify that the package can not be verified with the signed CA.pfx
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${signPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the owner .pfx
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${ownerPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the UEFI leaf pfx file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${uefiPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the alt Leaf pfx file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${altPfxFile}
    Should Be True  ${Result.rc} != 0


Build Enroll of DDS CA cert, signed by ZTD_Leaf.pfx, and verify
    [Setup]    Require test case    Build Enroll of DDS_CA cert, signed by ZTD_CA.pfx, and verify

    ${nameofTest}=    Set Variable    DDSEnroll
    ${ownerPfxFile}=  Set Variable    ${CERTS_DIR}${/}DDS_Leaf.pfx
    ${ownerCerFile}=  Set Variable    ${CERTS_DIR}${/}DDS_CA.cer
    ${signPfxFile}=   Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.pfx
    ${signCerFile}=   Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.cer
    ${uefiPfxFile}=   Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.pfx
    ${uefiCerFile}=   Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.cer
    ${altPfxFile}=    Set Variable    ${CERTS_DIR}${/}DDS_Leaf3.pfx
    ${altCerFile}=    Set Variable    ${CERTS_DIR}${/}DDS_Leaf3.cer

    File Should Exist  ${ownerPfxFile}
    File Should Exist  ${ownerCerFile}
    File Should Exist  ${signPfxFile}
    File Should Exist  ${signCerFile}
    File Should Exist  ${uefiPfxFile}
    File Should Exist  ${uefiCerFile}
    File Should Exist  ${altPfxFile}
    File Should Exist  ${altCerFile}

    ${currentIdxmlFile}=  Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentIdentities.xml
    ${binPackageFile}=    Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_Provision_apply.bin
    ${applyPackageFile}=  Set Variable    ${TOOL_STD_OUT_DIR}${/}${TestName}_Provision_apply.log

    Create Dfci Provisioning Package  ${binPackageFile}  ${signPfxFile}  ${ownerPfxFile}  ${ownerCerFile}  ${OWNER_KEY_INDEX}  @{TARGET_PARAMETERS}
    Print Provisioning Package        ${binPackageFile}  ${applyPackageFile}

    #
    # The following verification operations are expected to pass
    #

    # Verify that the package can be verified with the signed CA.cer
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${signCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the owner .cer
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${ownerCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the UEFI Leaf cer file
    #
    # This is the standard UEFI method.  Owner Cert is the CA cert, signed by the ZTD Leaf cert.
    # The Owner Cert will self verify its own signing, and the ZTD Leaf will be used to sign
    # the overall packet.
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${uefiCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the alt Leaf cer file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${altCerFile}
    Should Be True  ${Result.rc} == 0

    #
    # The following verification operations are expected to fail
    #

    # Verify that the package can not be verified with the signed CA.pfx
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${signPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the owner .pfx
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${ownerPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the UEFI leaf pfx file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${uefiPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the alt Leaf pfx file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${altPfxFile}
    Should Be True  ${Result.rc} != 0


Build Enroll of DDS CA cert, signed by DDS_Leaf.pfx, and verify
    [Setup]    Require test case    Build Enroll of DDS CA cert, signed by ZTD_Leaf.pfx, and verify

    ${nameofTest}=    Set Variable    DDSEnroll
    ${ownerPfxFile}=  Set Variable    ${CERTS_DIR}${/}DDS_Leaf.pfx
    ${ownerCerFile}=  Set Variable    ${CERTS_DIR}${/}DDS_CA.cer
    ${signPfxFile}=   Set Variable    ${CERTS_DIR}${/}DDS_Leaf.pfx
    ${signCerFile}=   Set Variable    ${CERTS_DIR}${/}DDS_Leaf.cer
    ${uefiPfxFile}=   Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.pfx
    ${uefiCerFile}=   Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.cer
    ${altPfxFile}=    Set Variable    ${CERTS_DIR}${/}DDS_Leaf3.pfx
    ${altCerFile}=    Set Variable    ${CERTS_DIR}${/}DDS_Leaf3.cer

    File Should Exist  ${ownerPfxFile}
    File Should Exist  ${ownerCerFile}
    File Should Exist  ${signPfxFile}
    File Should Exist  ${signCerFile}
    File Should Exist  ${uefiPfxFile}
    File Should Exist  ${uefiCerFile}
    File Should Exist  ${altPfxFile}
    File Should Exist  ${altCerFile}

    ${currentIdxmlFile}=  Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentIdentities.xml
    ${binPackageFile}=    Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_Provision_apply.bin
    ${applyPackageFile}=  Set Variable    ${TOOL_STD_OUT_DIR}${/}${TestName}_Provision_apply.log

    Create Dfci Provisioning Package  ${binPackageFile}  ${signPfxFile}  ${ownerPfxFile}  ${ownerCerFile}  ${OWNER_KEY_INDEX}  @{TARGET_PARAMETERS}
    Print Provisioning Package        ${binPackageFile}  ${applyPackageFile}

    #
    # The following verification operations are expected to pass
    #

    # Verify that the package can be verified with the signed CA.cer
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${signCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the owner .cer
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${ownerCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the UEFI Leaf cer file
    #
    # This is the standard UEFI method.  Owner Cert is the CA cert, signed by the Leaf cert.
    # The Owner Cert will self verify its own signing, and the Leaf will be used to sign
    # the overall packet.
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${uefiCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the alt Leaf cer file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${altCerFile}
    Should Be True  ${Result.rc} == 0

    #
    # The following verification operations are expected to fail
    #

    # Verify that the package can not be verified with the signed CA.pfx
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${signPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the owner .pfx
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${ownerPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the UEFI leaf pfx file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${uefiPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the alt Leaf pfx file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${altPfxFile}
    Should Be True  ${Result.rc} != 0


Build Enroll of MDM CA cert, signed by DDS_Leaf.pfx, and verify
    [Setup]    Require test case    Build Enroll of DDS CA cert, signed by DDS_Leaf.pfx, and verify

    ${nameofTest}=    Set Variable    MDMEnroll
    ${ownerPfxFile}=  Set Variable    ${CERTS_DIR}${/}MDM_CA.pfx
    ${ownerCerFile}=  Set Variable    ${CERTS_DIR}${/}MDM_CA.cer
    ${signPfxFile}=   Set Variable    ${CERTS_DIR}${/}DDS_Leaf.pfx
    ${signCerFile}=   Set Variable    ${CERTS_DIR}${/}DDS_Leaf.cer
    ${uefiPfxFile}=   Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.pfx
    ${uefiCerFile}=   Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.cer
    ${altPfxFile}=    Set Variable    ${CERTS_DIR}${/}DDS_Leaf3.pfx
    ${altCerFile}=    Set Variable    ${CERTS_DIR}${/}DDS_Leaf3.cer

    File Should Exist  ${ownerPfxFile}
    File Should Exist  ${ownerCerFile}
    File Should Exist  ${signPfxFile}
    File Should Exist  ${signCerFile}
    File Should Exist  ${uefiPfxFile}
    File Should Exist  ${uefiCerFile}
    File Should Exist  ${altPfxFile}
    File Should Exist  ${altCerFile}

    ${currentIdxmlFile}=  Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentIdentities.xml
    ${binPackageFile}=    Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_Provision_apply.bin
    ${applyPackageFile}=  Set Variable    ${TOOL_STD_OUT_DIR}${/}${TestName}_Provision_apply.log

    Create Dfci Provisioning Package  ${binPackageFile}  ${signPfxFile}  ${ownerPfxFile}  ${ownerCerFile}  ${OWNER_KEY_INDEX}  @{TARGET_PARAMETERS}
    Print Provisioning Package        ${binPackageFile}  ${applyPackageFile}

    #
    # The following verification operations are expected to pass
    #

    # Verify that the package can be verified with the signed CA.cer
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${signCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the owner .cer
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${ownerCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the DDS Leaf cer file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${uefiCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the alt Leaf cer file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${altCerFile}
    Should Be True  ${Result.rc} == 0

    #
    # The following verification operations are expected to fail
    #

    # Verify that the package can not be verified with the signed CA.pfx
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${signPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the owner .pfx
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${ownerPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the DDS leaf pfx file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${uefiPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the alt Leaf pfx file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${altPfxFile}
    Should Be True  ${Result.rc} != 0


Build Enroll of MDM CA cert, signed by MDM_Leaf.pfx, and verify
    [Setup]    Require test case    Build Enroll of MDM CA cert, signed by DDS_Leaf.pfx, and verify

    ${nameofTest}=    Set Variable    MDMEnroll
    ${ownerPfxFile}=  Set Variable    ${CERTS_DIR}${/}MDM_CA.pfx
    ${ownerCerFile}=  Set Variable    ${CERTS_DIR}${/}MDM_CA.cer
    ${signPfxFile}=   Set Variable    ${CERTS_DIR}${/}MDM_Leaf.pfx
    ${signCerFile}=   Set Variable    ${CERTS_DIR}${/}MDM_Leaf.cer
    ${uefiPfxFile}=   Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.pfx
    ${uefiCerFile}=   Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.cer
    ${altPfxFile}=    Set Variable    ${CERTS_DIR}${/}MDM_Leaf3.pfx
    ${altCerFile}=    Set Variable    ${CERTS_DIR}${/}MDM_Leaf3.cer

    File Should Exist  ${ownerPfxFile}
    File Should Exist  ${ownerCerFile}
    File Should Exist  ${signPfxFile}
    File Should Exist  ${signCerFile}
    File Should Exist  ${uefiPfxFile}
    File Should Exist  ${uefiCerFile}
    File Should Exist  ${altPfxFile}
    File Should Exist  ${altCerFile}

    ${currentIdxmlFile}=  Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentIdentities.xml
    ${binPackageFile}=    Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_Provision_apply.bin
    ${applyPackageFile}=  Set Variable    ${TOOL_STD_OUT_DIR}${/}${TestName}_Provision_apply.log

    Create Dfci Provisioning Package  ${binPackageFile}  ${signPfxFile}  ${ownerPfxFile}  ${ownerCerFile}  ${OWNER_KEY_INDEX}  @{TARGET_PARAMETERS}
    Print Provisioning Package        ${binPackageFile}  ${applyPackageFile}

    #
    # The following verification operations are expected to pass
    #

    # Verify that the package can be verified with the signed CA.cer
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${signCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the owner .cer
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${ownerCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the DDS Leaf cer file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${uefiCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the alt Leaf cer file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${altCerFile}
    Should Be True  ${Result.rc} == 0

    #
    # The following verification operations are expected to fail
    #

    # Verify that the package can not be verified with the signed CA.pfx
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${signPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the owner .pfx
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${ownerPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the DDS leaf pfx file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${uefiPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the alt Leaf pfx file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${altPfxFile}
    Should Be True  ${Result.rc} != 0


Build DDS Unenroll signed by DDS_Leaf.pfx, and verify
    [Setup]    Require test case    Build Enroll of MDM CA cert, signed by MDM_Leaf.pfx, and verify

    ${nameofTest}=    Set Variable    DDSUnenroll
    ${ownerPfxFile}=  Set Variable    ${CERTS_DIR}${/}DDS_CA.pfx
    ${ownerCerFile}=  Set Variable    ${CERTS_DIR}${/}DDS_CA.cer
    ${signPfxFile}=   Set Variable    ${CERTS_DIR}${/}DDS_Leaf.pfx
    ${signCerFile}=   Set Variable    ${CERTS_DIR}${/}DDS_Leaf.cer
    ${uefiPfxFile}=   Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.pfx
    ${uefiCerFile}=   Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.cer
    ${altPfxFile}=    Set Variable    ${CERTS_DIR}${/}DDS_Leaf3.pfx
    ${altCerFile}=    Set Variable    ${CERTS_DIR}${/}DDS_Leaf3.cer

    File Should Exist  ${ownerPfxFile}
    File Should Exist  ${ownerCerFile}
    File Should Exist  ${signPfxFile}
    File Should Exist  ${signCerFile}
    File Should Exist  ${uefiPfxFile}
    File Should Exist  ${uefiCerFile}
    File Should Exist  ${altPfxFile}
    File Should Exist  ${altCerFile}

    ${currentIdxmlFile}=  Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_currentIdentities.xml
    ${binPackageFile}=    Set Variable    ${TOOL_DATA_OUT_DIR}${/}${nameofTest}_Provision_apply.bin
    ${applyPackageFile}=  Set Variable    ${TOOL_STD_OUT_DIR}${/}${nameofTest}_Provision_apply.log

    Create Dfci UnEnroll Package  ${binPackageFile}  ${signPfxFile}  ${OWNER_KEY_INDEX}  @{TARGET_PARAMETERS}
    Print Provisioning Package    ${binPackageFile}  ${applyPackageFile}

    # The following verification operations are expected to pass

    # Verify that the package can be verified with the signed CA.cer
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${signCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the owner .cer
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${ownerCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the UEFI Leaf cer file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${uefiCerFile}
    Should Be True  ${Result.rc} == 0

    # Verify that the package can not be verified with the alt Leaf cer file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${altCerFile}
    Should Be True  ${Result.rc} == 0

    # The following verification operations are expected to fail

    # Verify that the package can not be verified with the signed CA.pfx
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${signPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the owner .pfx
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${ownerPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the UEFI leaf pfx file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${uefiPfxFile}
    Should Be True  ${Result.rc} != 0

    # Verify that the package can not be verified with the alt Leaf pfx file
    ${Result}=      Run Process    ${TOOL_DFCI_VERIFY}  ${binPackageFile}  ${altPfxFile}
    Should Be True  ${Result.rc} != 0
