*** Settings ***
# @file
#
Documentation    DFCI Shared Keywords
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

Library     OperatingSystem
Library     Process

Library     Support${/}Python${/}DFCI_SupportLib.py

*** Keywords ***


############################################################
#           Print Routines for each Package                #
############################################################
Print Provisioning Package
    [Arguments]     ${binfile}  ${stdoutfile}
    ${result} =     Run Process     ${DFCI_PY_PATH}${/}GenerateCertProvisionData.py  -p  ${binfile}  shell=Yes   timeout=10sec    stdout=${stdoutfile}
    Log File    ${stdoutfile}
    Should Be Equal As Integers     ${result.rc}    0

Print Permission Package
    [Arguments]     ${binfile}  ${stdoutfile}
    ${result} =     Run Process     ${DFCI_PY_PATH}${/}GeneratePermissionPacketData.py  -p  ${binfile}  shell=Yes   timeout=10sec    stdout=${stdoutfile}
    Log File    ${stdoutfile}
    Should Be Equal As Integers     ${result.rc}    0

Print Settings Package
    [Arguments]     ${binfile}  ${stdoutfile}
    ${result} =     Run Process     ${DFCI_PY_PATH}${/}GenerateSettingsPacketData.py    -p  ${binfile}  shell=Yes   timeout=10sec    stdout=${stdoutfile}
    Log File    ${stdoutfile}
    Should Be Equal As Integers     ${result.rc}    0


############################################################
#              Get results of each  Package                #
############################################################
Get Provisioning Result Package
    [Arguments]     ${stdoutfile}
    ${result} =     Run Process     ${DFCI_PY_PATH}${/}GetSEMResultData.py  --Provisioning  --IpAddress ${IP_OF_DUT}    shell=Yes   timeout=10sec    stdout=${stdoutfile}
    Log File    ${stdoutfile}
    Should Be Equal As Integers     ${result.rc}    0


Get Permission Result Package
    [Arguments]     ${stdoutfile}
    ${result} =     Run Process     ${DFCI_PY_PATH}${/}GetSEMResultData.py  --Permissions   --IpAddress ${IP_OF_DUT}    shell=Yes   timeout=10sec    stdout=${stdoutfile}
    Log File    ${stdoutfile}
    Should Be Equal As Integers     ${result.rc}    0


Get Settings Result Package
    [Arguments]     ${stdoutfile}
    ${result} =     Run Process     ${DFCI_PY_PATH}${/}GetSEMResultData.py  --Settings  --IpAddress ${IP_OF_DUT}    shell=Yes   timeout=10sec    stdout=${stdoutfile}
    Log File    ${stdoutfile}
    Should Be Equal As Integers     ${result.rc}    0


# Create an Unenroll Identity Package File
#

# binFile        = Output binary package to send to DUT
# signPfx        = Pfx file to sign the package
# testSignPfx    = Pfx file to verify signing with cert file
# certFile       = Cert used to verify incoming pkts
# KEY_INDEX      = Which key to unenroll
# TargetParms    = list with version and target information
#
Create Dfci Provisioning Package
    [Arguments]    ${binfile}  ${signPfx}  ${testSignPfx}  ${certFile}  ${KEY_INDEX}  @{TargetParms}

    File Should Exist   ${signPfx}
    File Should Exist   ${testSignPfx}
    File Should Exist   ${certFile}

    ${Result}=    Run Process    python.exe  ${GEN_IDENTITY}  --CertFilePath  ${certFile}  --Step2AEnable  --Signing2APfxFile  ${testSignPfx}  --Step2BEnable  --Step2Enable  --SigningPfxFile  ${signPfx}  --Step3Enable  --FinalizeResultFile  ${binfile}  --Step1Enable  --Identity  ${KEY_INDEX}  @{TargetParms}

    Log     all stdout: ${result.stdout}
    Log     all stderr: ${result.stderr}

    Should Be Equal As Integers     ${result.rc}    0
    File Should Exist   ${binfile}


# Create an Unenroll Identity Package File
#
# binFile        = Output binary package to send to DUT
# signPfx        = Pfx file to sign the package
# KEY_INDEX      = Which key to unenroll
# TargetParms    = list with version and target information
#
Create Dfci UnEnroll Package
    [Arguments]     ${binfile}  ${signPfx}  ${KEY_INDEX}  @{TargetParms}

    File Should Exist   ${signPfx}

    ${Result}=    Run Process    python.exe    ${GEN_IDENTITY}  --Step2Enable  --SigningPfxFile  ${signPfx}  --Step3Enable  --FinalizeResultFile  ${binfile}  --Step1Enable  --Identity  ${KEY_INDEX}  @{TargetParms}

    Log     all stdout: ${result.stdout}
    Log     all stderr: ${result.stderr}

    Should Be Equal As Integers     ${result.rc}    0
    File Should Exist   ${binfile}


# Create a Permissions Package File
#
# binFile        = Output binary package to send to DUT
# signPfx        = Pfx file to sign the package
# xmlFile        = The permissions XML file to apply
# TargetParms    = list with version and target information
#
Create Dfci Permission Package
    [Arguments]     ${binfile}  ${signPfx}  ${xmlFile}  @{TargetParms}

    File Should Exist   ${signPfx}
    File Should Exist   ${xmlFile}

    ${Result}=    Run Process    python.exe    ${GEN_PERMISSIONS}  --Step2Enable  --SigningPfxFile  ${signPfx}  --Step3Enable  --FinalizeResultFile  ${binfile}  --Step1Enable  --XmlFilePath  ${xmlFile}  @{TargetParms}

    Log     all stdout: ${result.stdout}
    Log     all stderr: ${result.stderr}

    Should Be Equal As Integers     ${result.rc}    0
    File Should Exist   ${binfile}


# Create a Settings Package File
#
# binFile        = Output binary package to send to DUT
# signPfx        = Pfx file to sign the package
# xmlFile        = The settings XML file to apply
# TargetParms    = list with version and target information
#
Create Dfci Settings Package
    [Arguments]     ${binfile}  ${signPfx}  ${xmlFile}  @{TargetParms}
    File Should Exist   ${xmlFile}

    IF  '${signPfx}' == 'UNSIGNED'
        ${Result}=    Run Process    python.exe    ${GEN_SETTINGS}  --HdrVersion  2  --Step1Enable  --PrepResultFile  ${binfile}  --XmlFilePath  ${xmlFile}  @{TargetParms}
    ELSE
        File Should Exist   ${signPfx}
        ${Result}=    Run Process    python.exe    ${GEN_SETTINGS}  --HdrVersion  2  --Step1Enable  --Step2Enable  --SigningPfxFile  ${signPfx}  --Step3Enable  --FinalizeResultFile  ${binfile}  --XmlFilePath  ${xmlFile}  @{TargetParms}
    END

    Log     all stdout: ${result.stdout}
    Log     all stderr: ${result.stderr}

    Should Be Equal As Integers     ${result.rc}    0
    File Should Exist   ${binfile}

