*** Settings ***
# @file
#
Documentation    DFCI Shared Keywords
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

Library     OperatingSystem
Library     Process
Library     Remote  http://${IP_OF_DUT}:${RF_PORT}
Library     Support${/}Python${/}DFCI_SupportLib.py

Resource     Support${/}Robot${/}DFCI_Shared_Keywords2.robot


*** Variables ***
${CMD_MFG}              Get-CimInstance -ClassName Win32_ComputerSystem -Property Manufacturer | Select-Object -ExpandProperty Manufacturer
${CMD_MODEL}            Get-CimInstance -ClassName Win32_ComputerSystem -Property Model | Select-Object -ExpandProperty Model
${CMD_SERIALNUMBER}     Get-CimInstance -ClassName Win32_systemenclosure -Property SerialNumber  | Select-Object -ExpandProperty SerialNumber
${CMD_UUID}             Get-CimInstance -ClassName Win32_computersystemproduct -Property uuid | Select-Object -ExpandProperty uuid


*** Keywords ***
Compare Files
    [Arguments]     ${CompareFile1}  ${CompareFile2}  ${ExpectedRC}

    ${result}=  Run Process    fc.exe  /b  ${CompareFile1}  ${CompareFile2}
    Log     all stdout: ${result.stdout}
    Log     all stderr: ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  ${ExpectedRC}


############################################################
#           Get system under test Information              #
############################################################

Get System Under Test SerialNumber
    ${Value}=   Run PowerShell And Return Output   ${CMD_SERIALNUMBER}
    Should Be True  '${Value}' != 'Error'
    Should Be True  '${Value}' != ''
    [Return]        ${Value}


Get System Under Test Manufacturer
    ${Value}=   Run PowerShell And Return Output   ${CMD_MFG}
    Should Be True  '${Value}' != 'Error'
    Should Be True  '${Value}' != ''
    [Return]        ${Value}


Get System Under Test ProductName
    ${Value}=   Run PowerShell And Return Output   ${CMD_MODEL}
    Should Be True  '${Value}' != 'Error'
    Should Be True  '${Value}' != ''
    [Return]        ${Value}


############################################################
#              Get Current Settings Value in XML           #
############################################################
Get Current Settings Package
    [Arguments]     ${stdoutfile}
    ${result} =     Run Process     ${DFCI_PY_PATH}${/}GetSEMResultData.py  --CurrentSettings   --IpAddress ${IP_OF_DUT}    shell=Yes   timeout=10sec    stdout=${stdoutfile}
    Log File    ${stdoutfile}
    Should Be Equal As Integers     ${result.rc}    0


Verify Provision Response
    [Arguments]     ${pktfile}  ${ResponseFile}  ${ExpectedRc}
    @{rc2}=     get status and sessionid from identity results  ${ResponseFile}
    ${id2}=     get sessionid from identity packet  ${pktfile}
    ${rc2zstring}=      get uefistatus string    ${rc2}[0]
    ${ExpectedString}=  get uefistatus string    ${ExpectedRc}
    Should Be Equal As Integers     ${rc2}[1]    ${id2}
    Should Be Equal As strings      ${rc2zstring}   ${ExpectedString}


Verify Permission Response
    [Arguments]     ${pktfile}  ${ResponseFile}  ${ExpectedRc}
    @{rc2}=     get status and sessionid from permission results  ${ResponseFile}
    ${id2}=     get sessionid from permission packet  ${pktfile}
    ${rc2zstring}=      get uefistatus string    ${rc2}[0]
    ${ExpectedString}=  get uefistatus string    ${ExpectedRc}
    Should Be Equal As Integers     ${rc2}[1]   ${id2}
    Should Be Equal As strings      ${rc2zstring}   ${ExpectedString}


Verify Settings Response
    [Arguments]     ${pktfile}  ${ResponseFile}  ${ExpectedRc}  ${checktype}
    @{rc2}=     get status and sessionid from settings results  ${ResponseFile}  ${checktype}
    ${id2}=     get sessionid from settings packet  ${pktfile}
    ${rc2zstring}=      get uefistatus string    ${rc2}[0]
    ${ExpectedString}=  get uefistatus string    ${ExpectedRc}
    Should Be Equal As Integers     ${rc2}[1]   ${id2}
    Should Be Equal As strings      ${rc2zstring}   ${ExpectedString}


Verify Identity Current
    [Arguments]     ${xmlfile}  ${Mfg}  ${ProdName}  ${SerialNumber}
    ${rc}=      Verify Device Id    ${xmlfile}  ${Mfg}  ${ProdName}  ${SerialNumber}
    Should Be Equal As Integers     ${rc}   0
    ${rc}=    Verify Dfci Version   ${xmlfile}   2
    Should Be True    ${rc}


Get and Print Current Identities
    [Arguments]   ${currentxmlFile}

    Get Current Identities   ${currentxmlFile}
    Print Xml Payload        ${currentxmlFile}


Get and Print Current Permissions
    [Arguments]   ${currentxmlFile}

    Get Current Permissions  ${currentxmlFile}
    Print Xml Payload        ${currentxmlFile}


Get and Print Current Settings
    [Arguments]   ${currentxmlFile}

    Get Current Settings    ${currentxmlFile}
    Print Xml Payload       ${currentxmlFile}


Get and Print Device Identifier
    [Arguments]   ${currentxmlFile}

    Get Device Identifier   ${currentxmlFile}
    Print Xml Payload       ${currentxmlFile}


############################################################
#      Resetting system and wait for reboot complete       #
############################################################

Wait For System Online
    [Arguments]     ${retries}
    FOR    ${index}    IN RANGE    ${retries}
       ${result} =     Is Device Online    ${IP_OF_DUT}
       Exit For Loop If    '${result}' == 'True'
       Sleep   5sec    "Waiting for system to come back Online"
    END
    Should Be True    ${result}    System failed to come online: pinging ${IP_OF_DUT} failed ${retries} times

Wait For System Offline
    [Arguments]     ${retries}
    FOR    ${index}    IN RANGE    ${retries}
       ${result} =     Is Device Online    ${IP_OF_DUT}
       Exit For Loop If    '${result}' == 'False'
       Sleep   5sec    "Waiting for system to go offline"
    END
    Should Not Be True    ${result}    System failed to go offline: pinged ${IP_OF_DUT} ${retries} times

Wait For Remote Robot
    [Arguments]     ${timeinseconds}
    FOR    ${retries}  IN RANGE    ${timeinseconds}
       Log To Console      Waiting for Robot To Ack ${retries}
       ${status}   ${message}  Run Keyword And Ignore Error    Remote Ack
       Return From Keyword If      '${status}' == 'PASS'   ${message}
       Sleep       1
    END
    Return From Keyword     ${False}

Reboot System And Wait For System Online
    remote_warm_reboot
    Wait For System Offline    60
    Wait For System Online    60
    Wait For Remote Robot    15

Reboot System To Firmware And Wait For System Online
    remote_reboot_to_firmware
    Wait For System Offline    60
    Wait For System Online    120
    Wait For Remote Robot    15


############################################################
#      Verify NO APPLY variables present                   #
############################################################

Verify No Mailboxes Have Data

    @{rcid}=    GetUefiVariable    ${IDENTITY_APPLY}  ${IDENTITY_GUID}  ${None}
    Run Keyword If   ${rcid}[0] != ${STATUS_VARIABLE_NOT_FOUND}
    ...    SetUefiVariable    ${IDENTITY_APPLY}  ${IDENTITY_GUID}

    @{rcid2}=    GetUefiVariable    ${IDENTITY2_APPLY}  ${IDENTITY_GUID}  ${None}
    Run Keyword If   ${rcid2}[0] != ${STATUS_VARIABLE_NOT_FOUND}
    ...    SetUefiVariable    ${IDENTITY2_APPLY}  ${IDENTITY_GUID}

    @{rcperm}=    GetUefiVariable    ${PERMISSION_APPLY}  ${PERMISSION_GUID}  ${None}
    Run Keyword If   ${rcperm}[0] != ${STATUS_VARIABLE_NOT_FOUND}
    ...    SetUefiVariable    ${PERMISSION_APPLY}  ${IDENTITY_GUID}

    @{rcperm2}=    GetUefiVariable    ${PERMISSION2_APPLY}  ${PERMISSION_GUID}  ${None}
    Run Keyword If   ${rcperm2}[0] != ${STATUS_VARIABLE_NOT_FOUND}
    ...    SetUefiVariable    ${PERMISSION2_APPLY}  ${IDENTITY_GUID}

    @{rcset}=    GetUefiVariable    ${SETTINGS_APPLY}  ${SETTINGS_GUID}  ${None}
    Run Keyword If   ${rcset}[0] != ${STATUS_VARIABLE_NOT_FOUND}
    ...    SetUefiVariable    ${SETTINGS_APPLY}  ${IDENTITY_GUID}

    @{rcset2}=    GetUefiVariable    ${SETTINGS2_APPLY}  ${SETTINGS_GUID}  ${None}
    Run Keyword If   ${rcset2}[0] != ${STATUS_VARIABLE_NOT_FOUND}
    ...    SetUefiVariable    ${SETTINGS2_APPLY}  ${IDENTITY_GUID}

    Should Be True    ${rcid}[0] == ${STATUS_VARIABLE_NOT_FOUND}
    Should Be True    ${rcperm}[0] == ${STATUS_VARIABLE_NOT_FOUND}
    Should Be True    ${rcperm2}[0] == ${STATUS_VARIABLE_NOT_FOUND}
    Should Be True    ${rcset}[0] == ${STATUS_VARIABLE_NOT_FOUND}
    Should Be True    ${rcset2}[0] == ${STATUS_VARIABLE_NOT_FOUND}


########################################################################
#      Apply a Provision (Identity) Package, and check the results     #
########################################################################
Process Provision Packet
    [Arguments]    ${TestName}  ${mailbox}  ${signPfxFile}  ${testsignPfxFile}  ${ownerCertFile}  ${KEY_INDEX}  @{TargetParms}
    ${applyPackageFile}=   Set Variable    ${TOOL_STD_OUT_DIR}${/}${TestName}_Provision_apply.log
    ${binPackageFile}=     Set Variable    ${TOOL_DATA_OUT_DIR}${/}${TestName}_Provision_apply.bin

    #Create and deploy an identity packet

    Create Dfci Provisioning Package     ${binPackageFile}  ${signPfxFile}  ${testsignPfxFile}  ${ownerCertFile}  ${KEY_INDEX}  @{TargetParms}
    Print Provisioning Package           ${binPackageFile}  ${applyPackageFile}

    Apply Identity  ${mailbox}  ${binPackageFile}


Validate Provision Status
    [Arguments]    ${TestName}  ${mailbox}  ${expectedStatus}
    ${binPackageFile}=     Set Variable    ${TOOL_DATA_OUT_DIR}${/}${TestName}_Provision_apply.bin
    ${binResultFile}=      Set Variable    ${TOOL_DATA_OUT_DIR}${/}${TestName}_Provision_result.bin

    Get Identity Results  ${mailbox}  ${binResultFile}

    Verify Provision Response   ${binPackageFile}  ${binResultFile}  ${expectedStatus}


##############################################################
#      Apply a Permission Package, and check the results     #
##############################################################
Process Permission Packet
    [Arguments]  ${TestName}  ${mailbox}  ${ownerPfxFile}  ${PayloadFile}  @{TargetParms}
    ${applyPackageFile}=  Set Variable  ${TOOL_STD_OUT_DIR}${/}${TestName}_Permission_apply.log
    ${binPackageFile}=    Set Variable  ${TOOL_DATA_OUT_DIR}${/}${TestName}_Permission_apply.bin


    #Create and deploy a permissions packet

    Create Dfci Permission Package  ${binPackageFile}  ${ownerPfxFile}  ${PayloadFile}  @{TargetParms}
    Print Permission Package        ${binPackageFile}  ${applyPackageFile}

    Apply Permission  ${mailbox}  ${binPackageFile}


Validate Permission Status
    [Arguments]  ${TestName}  ${mailbox}  ${expectedStatus}
    ${binPackageFile}=  Set Variable  ${TOOL_DATA_OUT_DIR}${/}${TestName}_Permission_apply.bin
    ${binResultFile}=   Set Variable  ${TOOL_DATA_OUT_DIR}${/}${TestName}_Permission_result.bin
    ${xmlResultFile}=   Set Variable  ${TOOL_DATA_OUT_DIR}${/}${TestName}_Permission_result.xml

    Get Permission Results  ${mailbox}  ${binResultFile}

    Verify Permission Response  ${binPackageFile}  ${binResultFile}  ${expectedStatus}

    # V1 doesn't have permission payload
    Return From Keyword If  '${TARGET_VERSION}' == 'V1'

    Get Payload From Permissions Results  ${binResultFile}  ${xmlResultFile}
    File Should Exist  ${xmlResultFile}
    [return]  ${xmlResultFile}


############################################################
#      Apply a Settings Package, and check the results     #
############################################################
Process Settings Packet
    [Arguments]  ${TestName}  ${mailbox}  ${ownerPfxFile}  ${PayloadFile}  @{TargetParms}
    ${applyPackageFile}=  Set Variable  ${TOOL_STD_OUT_DIR}${/}${TestName}_Settings_apply.log
    ${binPackageFile}=    Set Variable  ${TOOL_DATA_OUT_DIR}${/}${TestName}_Settings_apply.bin

    #Create and deploy a settings packet

    Create Dfci Settings Package  ${binPackageFile}  ${ownerPfxFile}  ${PayloadFile}  @{TargetParms}
    Print Settings Package        ${binPackageFile}    ${applyPackageFile}

    Apply Settings  ${mailbox}  ${binPackageFile}


Validate Settings Status
    [Arguments]  ${TestName}  ${mailbox}  ${expectedStatus}  ${full}
    ${binPackageFile}=  Set Variable  ${TOOL_DATA_OUT_DIR}${/}${TestName}_Settings_apply.bin
    ${binResultFile}=   Set Variable  ${TOOL_DATA_OUT_DIR}${/}${TestName}_Settings_result.bin
    ${xmlResultFile}=   Set Variable  ${TOOL_DATA_OUT_DIR}${/}${TestName}_Settings_result.xml

    Get Settings Results  ${mailbox}  ${binResultFile}

    Verify Settings Response    ${binPackageFile}  ${binResultFile}  ${expectedStatus}  ${full}

    Get Payload From Settings Results  ${binResultFile}  ${xmlResultFile}
    Run Keyword If  '${expectedStatus}' == ${STATUS_SUCCESS}  File Should Exist  ${xmlResultFile}
    [return]  ${xmlResultFile}


########################################################################
#      Process Unenroll Package                                        #
########################################################################
Process UnEnroll Packet
    [Arguments]    ${TestName}  ${mailbox}  ${signPfxFile}  ${KEY_INDEX}  @{TargetParms}
    ${applyPackageFile}=   Set Variable    ${TOOL_STD_OUT_DIR}${/}${TestName}_UnEnroll_apply.log
    ${binPackageFile}=     Set Variable    ${TOOL_DATA_OUT_DIR}${/}${TestName}_UnEnroll_apply.bin

    #Create and deploy an identity packet

    Create Dfci Unenroll Package     ${binPackageFile}  ${signPfxFile}  ${KEY_INDEX}  @{TargetParms}
    Print Provisioning Package       ${binPackageFile}  ${applyPackageFile}

    Apply Identity  ${mailbox}  ${binPackageFile}


Validate UnEnroll Status
    [Arguments]    ${TestName}  ${mailbox}  ${expectedStatus}
    ${binPackageFile}=     Set Variable    ${TOOL_DATA_OUT_DIR}${/}${TestName}_UnEnroll_apply.bin
    ${binResultFile}=      Set Variable    ${TOOL_DATA_OUT_DIR}${/}${TestName}_UnEnroll_result.bin

    Get Identity Results  ${mailbox}  ${binResultFile}

    Verify Provision Response    ${binPackageFile}  ${binResultFile}  ${expectedStatus}
