| *** Settings *** |
# @file
#
# Start the UefiSerialLogger.py to capture the UEFI log from a serial port
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

Documentation    Platform implemented Keywords used to get serial logs for UEFI on the DUT
| Library     | Process

*** Variables ***

*** Keywords ***
Start SerialLog
    [Arguments]     ${logfile}
    Start Process   python  ${CURDIR}${/}UefiSerialLogger.py    --SerialLogOutput   ${logFile}
