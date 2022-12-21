*** Settings ***
# @file
#
Documentation    DFCI Certificate Support
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

Library     OperatingSystem
Library     Process
Library     ${CURDIR}${/}..${/}Python${/}CertSupportLib.py

*** Keywords ***

Set DDS CA cert
    Set Global Variable    ${NEW_OWNER_CERT}   ${CERTS_DIR}${/}DDS_CA.cer
    Set Global Variable    ${NEW_OWNER_PFX}    ${CERTS_DIR}${/}DDS_Leaf.pfx
    Set Global Variable    ${OLD_OWNER_PFX}    ${CERTS_DIR}${/}DDS_Leaf2.pfx
    Set Global Variable    ${OLD_OWNER_CERT}   ${CERTS_DIR}${/}DDS_CA2.cer


Set DDS CA2 cert
    Set Global Variable    ${NEW_OWNER_CERT}   ${CERTS_DIR}${/}DDS_CA2.cer
    Set Global Variable    ${NEW_OWNER_PFX}    ${CERTS_DIR}${/}DDS_Leaf2.pfx
    Set Global Variable    ${OLD_OWNER_PFX}    ${CERTS_DIR}${/}DDS_Leaf.pfx
    Set Global Variable    ${OLD_OWNER_CERT}   ${CERTS_DIR}${/}DDS_CA.cer


Set MDM CA cert
    Set Global Variable    ${NEW_USER_CERT}    ${CERTS_DIR}${/}MDM_CA.cer
    Set Global Variable    ${NEW_USER_PFX}     ${CERTS_DIR}${/}MDM_Leaf.pfx
    Set Global Variable    ${OLD_USER_PFX}     ${CERTS_DIR}${/}MDM_Leaf2.pfx
    Set Global Variable    ${OLD_USER_CERT}    ${CERTS_DIR}${/}MDM_CA2.cer


Set MDM CA2 cert
    Set Global Variable    ${NEW_USER_CERT}    ${CERTS_DIR}${/}MDM_CA2.cer
    Set Global Variable    ${NEW_USER_PFX}     ${CERTS_DIR}${/}MDM_Leaf2.pfx
    Set Global Variable    ${OLD_USER_PFX}     ${CERTS_DIR}${/}MDM_Leaf.pfx
    Set Global Variable    ${OLD_USER_CERT}    ${CERTS_DIR}${/}MDM_CA.cer

Set HTTPS cert
    Set Global Variable    ${HTTPS_PEM}        ${CERTS_DIR}${/}DFCI_HTTPS.pem
    Set Global Variable    ${HTTPS2_PEM}       ${CERTS_DIR}${/}DFCI_HTTPS2.pem

Initialize Thumbprints
    [Arguments]    ${OwnerThumbprint}    ${UserThumbprint}

    ${DdsCA}=      Set Variable    ${CERTS_DIR}${/}DDS_CA.pfx
    ${MdmCA}=      Set Variable    ${CERTS_DIR}${/}MDM_CA.pfx
    ${ZtdLeaf}=    Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.pfx
    ${ZtdCert}=    Set Variable    ${CERTS_DIR}${/}ZTD_Leaf.cer

    ${DdsThumbprint}=      Get Thumbprint From Pfx    ${DdsCA}
    ${MdmThumbprint}=      Get Thumbprint From Pfx    ${MdmCA}
    ${ZtdThumbprint}=      Get Thumbprint From Pfx    ${ZtdLeaf}

    Run Keyword If   ${OwnerThumbprint} == '${DdsThumbprint}'
    ...        Set DDS CA2 cert
    ...    ELSE
    ...        Set DDS CA cert
    Run Keyword If   ${UserThumbprint} == '${MdmThumbprint}'
    ...        Set MDM CA2 cert
    ...    ELSE
    ...        Set MDM CA cert

    Set HTTPS cert

    Set Global Variable    ${ZTD_LEAF_PFX}            ${ZtdLeaf}
    Set Global Variable    ${ZTD_LEAF_CERT}           ${ZtdCert}
    Set Global Variable    ${DDS_CA_THUMBPRINT}      '${DdsThumbprint}'
    Set Global Variable    ${MDM_CA_THUMBPRINT}      '${MdmThumbprint}'
    Set Global Variable    ${ZTD_LEAF_THUMBPRINT}    '${ZtdThumbprint}'
