*** Settings ***
# @file
#
Documentation    This test suite verifies the actions of the Refresh Server.
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent


Library         OperatingSystem
Library         Process
Library         Collections
Library         String

Library         Support${/}Python${/}DFCI_SupportLib.py
Library         Support${/}Python${/}DependencyLib.py

#Import the Generic Shared keywords
Resource        Support${/}Robot${/}DFCI_Shared_Paths.robot
Resource        Support${/}Robot${/}CertSupport.robot

Suite setup     Make Dfci Output
Test Teardown   Terminate All Processes    kill=True


*** Variables ***
#test output directory for data from this test run.
${TEST_OUTPUT_BASE}     ..${/}TEST_OUTPUT

#Test output location
${TEST_OUTPUT}          ${TEST_OUTPUT_BASE}

#Test Root Dir
${TEST_ROOT_DIR}        TestCases
${TEST_CASE_DIR}        ${TEST_ROOT_DIR}${/}DFCI_RefreshServer

${TOOL_DATA_OUT_DIR}    ${TEST_OUTPUT}${/}bindata
${TOOL_STD_OUT_DIR}     ${TEST_OUTPUT}${/}stdout
${BOOT_LOG_OUT_DIR}     ${TEST_OUTPUT}${/}uefilogs

${CERTS_DIR}            Certs

${DDS_CA_THUMBPRINT}    'Thumbprint Not Set'
${MDM_CA_THUMBPRINT}    'Thumbprint Not Set'


*** Keywords ***
Get Response header
    [Arguments]    ${header_title}  ${header_file}

    File Should Exist  ${header_file}
    ${header_contents}=  Get File  ${header_file}
    ${header_count}=  Get Line Count  ${header_contents}
    Should Be True  ${header_count} > 0

    ${lines}=           Split To Lines  ${header_contents}
    ${value}=           Set Variable  Not Found
    FOR  ${index}       IN RANGE  ${header_count}
        ${line}=  Get From List  ${lines}  ${index}
        ${good}=  Evaluate  "${line}" != ""
        Should Be True  ${good}
        ${words}=  Split String  ${line}  max_split=1
        ${key}=  Get From List  ${words}  0
        ${word_lower}=  Evaluate  "${key}".lower()
        ${header_title}=  Evaluate  "${header_title}".lower()
        IF  "${word_lower}"=="${header_title}"
            ${value}=  Get From List  ${words}  1
            BREAK
        END
    END

    ${good}=  Evaluate  "${value}" != "Not Found"
    Should Be True  ${good}

    [return]  ${value}


*** Test Cases ***
Get the host name
    ${DfciConfig}=                 get test config
    Dictionary Should Contain Key  ${DfciConfig}  DfciTest
    ${DfciTestConfig}=             Get From Dictionary  ${DfciConfig}  DfciTest
    Dictionary Should Contain Key  ${DfciTestConfig}  server_host_name
    ${HostName}=                   Get From Dictionary  ${DfciTestConfig}  server_host_name
    Set Suite Variable             ${HostName}

    Set HTTPS cert

Check for port 80
    [Setup]         Require test case    Get the host name

    ${url}=         Catenate  SEPARATOR=  http://  ${HostName}
    ${result}=      Run Process    curl.exe  ${url}
    Log             ${result.stdout}
    Log             ${result.stderr}
    Should Be True  ${result.rc}==0
    ${good}=        Evaluate  "Http - Hello, World! DFCI Test Server V 3.0" in """${result.stdout}"""

    Log To Console  .
    Log To Console  ${result.stdout}
    Should Be True  ${good}


Check for port 443 with no TLS cert
    [Setup]         Require test case    Get the host name

#
# This tests to make sure the DFCI_HTTPS cert is not loaded in the default store
#
    ${url}=         Catenate  SEPARATOR=  https://  ${HostName}
    ${result}=      Run Process    curl.exe  ${url}
    Log             ${result.stdout}
    Log             ${result.stderr}
    Should Be True  ${result.rc}!=0


Check for port 443 with valid cert
    [Setup]         Require test case    Get the host name

    ${url}=         Catenate  SEPARATOR=  https://  ${HostName}
    ${result}=      Run Process    curl.exe  --cacert  ${HTTPS_PEM}  ${url}
    Log             ${result.stdout}
    Log             ${result.stderr}
    Should Be True  ${result.rc}==0
    ${good}=        Evaluate  "Https - Hello, World! DFCI Test Server V 3.0" in """${result.stdout}"""

    Log To Console  .
    Log To Console  ${result.stdout}
    Should Be True  ${good}


Check for port 443 with invalid cert
    [Setup]         Require test case    Get the host name

    ${url}=         Catenate  SEPARATOR=  https://  ${HostName}
    ${result}=      Run Process    curl.exe  --cacert  ${HTTPS2_PEM}  ${url}
    Log             ${result.stdout}
    Log             ${result.stderr}
    Should Be True  ${result.rc}!=0


Check for return of 429 as http
    [Setup]         Require test case    Get the host name
    ${response_header_file}=  Set Variable  ${TOOL_DATA_OUT_DIR}${/}Return_429_http.ResponseHeaders.json

    ${url}=         Catenate  SEPARATOR=  http://  ${HostName}  /return_429
    ${result}=      Run Process    curl.exe  -s  -D  ${response_header_file}  ${url}
    Log             ${result.stdout}
    Log             ${result.stderr}
    Should Be True  ${result.rc}==0

    ${value}=      Get Response header  HTTP/1.1  ${response_header_file}

    ${good}=        Evaluate  "429" in "${value}"
    Should Be True  ${good}


Check for return of 429 as https
    [Setup]         Require test case    Get the host name
    ${response_header_file}=  Set Variable  ${TOOL_DATA_OUT_DIR}${/}Return_429_https.ResponseHeaders.json

    ${url}=         Catenate  SEPARATOR=  https://  ${HostName}  /return_429
    ${result}=      Run Process    curl.exe  --cacert  ${HTTPS_PEM}  -D  ${response_header_file}  ${url}
    Log             ${result.stdout}
    Log             ${result.stderr}
    Should Be True  ${result.rc}==0

    ${value}=      Get Response header  HTTP/1.1  ${response_header_file}

    ${good}=        Evaluate  "429" in "${value}"
    Should Be True  ${good}


Check current host name
    [Setup]         Require test case    Get the host name

    ${url}=         Catenate  SEPARATOR=  https://  ${HostName}  /GetHostName
    ${result}=      Run Process    curl.exe  --cacert  ${HTTPS_PEM}  ${url}
    Log             ${result.stdout}
    Log             ${result.stderr}
    Should Be True  ${result.rc}==0

    ${good}=        Evaluate  "${HostName}"=="${result.stdout}"
    Should Be True  ${good}


Check current host name as http
    [Setup]         Require test case    Get the host name
    ${response_header_file}=  Set Variable  ${TOOL_DATA_OUT_DIR}${/}Current_host_http.ResponseHeaders.json

    File Should Not Exist  ${response_header_file}

    ${url}=         Catenate  SEPARATOR=  http://  ${HostName}  /GetHostName
    ${result}=      Run Process    curl.exe  -D  ${response_header_file}  ${url}
    Log             ${result.stdout}
    Log             ${result.stderr}
    Should Be True  ${result.rc}==0

    File Should Exist  ${response_header_file}

    ${value}=      Get Response header  HTTP/1.1  ${response_header_file}
    ${good}=        Evaluate  "502" in "${value}"
    Should Be True  ${good}


#
# make sure the host name did not change
#
    ${url}=         Catenate  SEPARATOR=  https://  ${HostName}  /GetHostName
    ${result}=      Run Process    curl.exe  --cacert  ${HTTPS_PEM}  ${url}
    Log             ${result.stdout}
    Log             ${result.stderr}
    Should Be True  ${result.rc}==0
    ${good}=        Evaluate  "${HostName}"=="${result.stdout}"
    Should Be True  ${good}


Initial Bootstrap Request with CA keys
    [Setup]         Require test case    Get the host name

#
# This first test is when the owner keys are using the xxx_CA.pfx keys.  Before the unenroll
# packets can be sent, transition packets need to roll the certs to the zzz_CA2.pfx keys.
#
    ${body}=  Set Variable  RefreshServer${/}Src${/}Responses${/}UnExpected_Request.json
    ${response_header_file}=  Set Variable  ${TOOL_DATA_OUT_DIR}${/}RequestCA.ResponseHeaders.json

    File Should Exist      ${body}
    File Should Not Exist  ${response_header_file}

    ${body_size}=  Get File Size  ${body}

    ${h1}=  Set Variable  Host: ${HostName}
    ${h2}=  Set Variable  Accept: */*
    ${h3}=  Set Variable  Content-Length: ${body_size}
    ${h4}=  Set Variable  Content-Type: application/json

    ${url}=         Catenate  SEPARATOR=  http://  ${HostName}  /ztd/unauthenticated/dfci/recovery-bootstrap
    ${result}=      Run Process    curl.exe  -D  ${response_header_file}  -A  DFCI-Agent  -H  ${h1}  -H  ${h2}  -H  ${h3}  -H  ${h4}  --data-binary  @${body}  ${url}
    Log             ${result.stdout}
    Log             ${result.stderr}
    Should Be True  ${result.rc}==0

    File Should Exist  ${response_header_file}
    ${value}=          Get Response Header  HTTP/1.1  ${response_header_file}
    ${good}=           Evaluate  "202 ACCEPTED" in "${value}"
    Should Be True     ${good}
    ${location}=       Get Response Header  Location:  ${response_header_file}
    ${location2}=      Catenate  SEPARATOR=  http://  ${HostName}  /ztd/unauthenticated/dfci/recovery-bootstrap-status/request-id
    ${good}=           Evaluate  "${location}"=="${location2}"
    Should Be True     ${good}

    Set Suite Variable    ${location}

Initial Bootstrap Request to redirected location
    [Setup]         Require test case    Get the host name
    ${response_header_file}=  Set Variable  ${TOOL_DATA_OUT_DIR}${/}RedirectedCA.ResponseHeaders.json

#
# The simulated server builds the transition packets, and now the task is to get the transition
# packets.
#
    ${transition_packet_file}=  Set Variable  ${TOOL_DATA_OUT_DIR}${/}Transition1.json
    ${expected_file}=  Set Variable  RefreshServer${/}Src${/}Responses${/}Bootstrap_Response.json

    File Should Not Exist  ${transition_packet_file}
    File Should Exist      ${expected_file}

    ${h1}=  Set Variable  Host: ${HostName}
    ${h2}=  Set Variable  Accept: */*
    ${h3}=  Set Variable  Content-Length: 0
    ${h4}=  Set Variable  Content-Type: application/json

    ${result}=      Run Process    curl.exe  -D   ${response_header_file}  -A  DFCI-Agent  -H  ${h1}  -H  ${h2}  -H  ${h3}  -H  ${h4}  -o  ${transition_packet_file}  ${location}
    Log             ${result.stdout}
    Log             ${result.stderr}
    Should Be True  ${result.rc}==0

    File Should Exist  ${transition_packet_file}
    File Should Exist  ${response_header_file}

    ${value}=      Get Response Header  HTTP/1.1  ${response_header_file}
    ${good}=        Evaluate  "200 OK" in "${value}"
    Should Be True  ${good}


    ${good}=    Compare Json Files  ${transition_packet_file}  ${expected_file}
    Should Be True  ${good}


Second Bootstrap Request with CA2 keys
    [Setup]         Require test case    Get the host name

#
# This first test is when the owner keys are using the xxx_CA.pfx keys.  Before the unenroll
# packets can be sent, transition packets need to roll the certs to the zzz_CA2.pfx keys.
#
    ${body}=  Set Variable  RefreshServer${/}Src${/}Requests${/}Expected_Request.json
    ${response_header_file}=  Set Variable  ${TOOL_DATA_OUT_DIR}${/}RequestCA2.ResponseHeaders.json

    File Should Exist      ${body}
    File Should Not Exist  ${response_header_file}

    ${body_size}=  Get File Size  ${body}

    ${h1}=  Set Variable  Host: ${HostName}
    ${h2}=  Set Variable  Accept: */*
    ${h3}=  Set Variable  Content-Length: ${body_size}
    ${h4}=  Set Variable  Content-Type: application/json

    ${url}=         Catenate  SEPARATOR=  http://  ${HostName}  /ztd/unauthenticated/dfci/recovery-bootstrap
    ${result}=      Run Process    curl.exe  -D  ${response_header_file}  -A  DFCI-Agent  -H  ${h1}  -H  ${h2}  -H  ${h3}  -H  ${h4}  --data-binary  @${body}  ${url}
    Log             ${result.stdout}
    Log             ${result.stderr}
    Should Be True  ${result.rc}==0

    File Should Exist  ${response_header_file}
    ${value}=          Get Response Header  HTTP/1.1  ${response_header_file}
    ${good}=           Evaluate  "202 ACCEPTED" in "${value}"
    Should Be True     ${good}

#
# curl option -D - dumps the response headers into stdout.  Break stdout into words, and look for
# case insensitive "Location:" to get the next URL.
#
    ${location}=    Get Response Header  Location:  ${response_header_file}
    ${location2}=   Catenate  SEPARATOR=  http://  ${HostName}  /ztd/unauthenticated/dfci/recovery-bootstrap-status/request-id

    ${good}=        Evaluate  "${location}"=="${location2}"
    Should Be True  ${good}

    Set Suite Variable    ${location}


Second Bootstrap Request to redirected location
    [Setup]         Require test case    Get the host name
    ${response_header_file}=  Set Variable  ${TOOL_DATA_OUT_DIR}${/}RedirectedCA2.ResponseHeaders.json

#
# The simulated server builds the transition packets, and now the task is to get the transition
# packets.
#
    ${transition_packet_file}=  Set Variable  ${TOOL_DATA_OUT_DIR}${/}SecondBootstrapTransition.json
    ${expected_file}=  Set Variable  RefreshServer${/}Src${/}Responses${/}Bootstrap_NULLResponse.json

    File Should Not Exist  ${transition_packet_file}
    File Should Exist      ${expected_file}

    ${h1}=  Set Variable  Host: ${HostName}
    ${h2}=  Set Variable  Accept: */*
    ${h3}=  Set Variable  Content-Length: 0
    ${h4}=  Set Variable  Content-Type: application/json

    ${result}=      Run Process    curl.exe  -D   ${response_header_file}  -A  DFCI-Agent  -H  ${h1}  -H  ${h2}  -H  ${h3}  -H  ${h4}  -o  ${transition_packet_file}  ${location}
    Log             ${result.stdout}
    Log             ${result.stderr}
    Should Be True  ${result.rc}==0

    File Should Exist  ${transition_packet_file}
    File Should Exist  ${response_header_file}

    ${value}=      Get Response Header  HTTP/1.1  ${response_header_file}
    ${good}=        Evaluate  "200 OK" in "${value}"
    Should Be True  ${good}

    ${good}=    Compare Json Files  ${transition_packet_file}  ${expected_file}
    Should Be True  ${good}


Recovery Packet Request with CA2 keys
    [Setup]         Require test case    Get the host name

#
# This test causes the server to transfer the un-enroll packet.
#
    ${body}=  Set Variable  RefreshServer${/}Src${/}Requests${/}Expected_Request.json
    ${response_header_file}=  Set Variable  ${TOOL_DATA_OUT_DIR}${/}RecoveryCA2.ResponseHeaders.json

    File Should Exist  ${body}
    File Should Not Exist  ${response_header_file}

    ${body_size}=  Get File Size  ${body}

    ${h1}=  Set Variable  Host: ${HostName}
    ${h2}=  Set Variable  Accept: */*
    ${h3}=  Set Variable  Content-Length: ${body_size}
    ${h4}=  Set Variable  Content-Type: application/json

    ${url}=     Catenate  SEPARATOR=  https://  ${HostName}  /ztd/unauthenticated/dfci/recovery-packets
    ${result}=  Run Process    curl.exe  -D  ${response_header_file}  -A  DFCI-Agent  -H  ${h1}  -H  ${h2}  -H  ${h3}  -H  ${h4}  --data-binary  @${body}  --cacert  ${HTTPS_PEM}  ${url}

    Log             ${result.stdout}
    Log             ${result.stderr}
    Should Be True  ${result.rc}==0

#
# curl option -D - dumps the response headers into stdout.  Break stdout into words, and look for
# case insensitive "Location:" to get the next URL.
#
    File Should Exist   ${response_header_file}
    ${location}=        Get Response Header  Location:  ${response_header_file}
    ${value}=           Get Response Header  HTTP/1.1  ${response_header_file}
    ${location2}=       Catenate  SEPARATOR=  https://  ${HostName}  /ztd/unauthenticated/dfci/recovery-packets-status/request-id
    ${good}=            Evaluate  "202 ACCEPTED" in "${value}"
    Should Be True      ${good}

    ${good}=            Evaluate  "${location}"=="${location2}"
    Should Be True      ${good}
    Set Suite Variable  ${location}


Recovery Packet Request to redirected location
    [Setup]         Require test case    Get the host name

    ${response_header_file}=  Set Variable  ${TOOL_DATA_OUT_DIR}${/}RedirectedRecoveryCA2.ResponseHeaders.json

    File Should Not Exist  ${response_header_file}

#
# The simulated server builds the transition packets, and now the task is to get the transition
# packets.
#
    ${transition_packet_file}=  Set Variable  ${TOOL_DATA_OUT_DIR}${/}RecoveryTransition.json
    ${expected_file}=           Set Variable  RefreshServer${/}Src${/}Responses${/}Recovery_Response.json

    File Should Exist      ${expected_file}
    File Should Not Exist  ${transition_packet_file}

    ${h1}=  Set Variable  Host: ${HostName}
    ${h2}=  Set Variable  Accept: */*
    ${h3}=  Set Variable  Content-Length: 0
    ${h4}=  Set Variable  Content-Type: application/json

    ${result}=      Run Process    curl.exe  -D   ${response_header_file}  -A  DFCI-Agent  -H  ${h1}  -H  ${h2}  -H  ${h3}  -H  ${h4}  -o  ${transition_packet_file}  --cacert  ${HTTPS_PEM}  ${location}
    Log             ${result.stdout}
    Log             ${result.stderr}
    Should Be True  ${result.rc}==0

    File Should Exist   ${response_header_file}
    ${value}=           Get Response Header  HTTP/1.1  ${response_header_file}
    ${good}=            Evaluate  "200 OK" in "${value}"
    Should Be True      ${good}

    File Should Exist  ${transition_packet_file}

    ${good}=    Compare Json Files  ${transition_packet_file}  ${expected_file}
    Should Be True  ${good}
