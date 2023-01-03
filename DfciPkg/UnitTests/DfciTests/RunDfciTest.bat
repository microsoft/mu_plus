@echo off
rem @file
rem
rem Script to run a testcase.
rem
rem Copyright (c), Microsoft Corporation
rem SPDX-License-Identifier: BSD-2-Clause-Patent

rem A few testcases do not require an IP address.
if %2. EQU . (
  if "%1" EQU "TestCases\DFCI_RefreshServer" goto no_ip_required
  if "%1" EQU "TestCases\DFCI_CertChainingTest" goto no_ip_required
)

rem Check if a saved IP address is present
if %2. NEQ . (
  set DFCI_TARGET_IP=%2
)

rem For most testcases, an IP address of the target system is required
if .%DFCI_TARGET_IP% EQU . (
  echo Missing DFCI Target IP Address
  exit /b 8
)

:no_ip_required

setlocal
set hh=%time:~0,2%
if "%time:~0,1%"==" " set hh=0%hh:~1,1%

set date=%date:~10,4%%date:~4,2%%date:~7,2%_%hh%%time:~3,2%%time:~6,2%

set Platform=Platforms\SimpleFTDI\Args.txt

set OutputBase=C:\TestLogs\robot\%~n1\logs_%date%

@echo on
python.exe -m robot.run -L TRACE -x %~n1.xml -A %Platform% -v IP_OF_DUT:%DFCI_TARGET_IP% -v TEST_OUTPUT_BASE:%OutputBase% -d %OutputBase% TestCases\%~n1\run.robot

@echo off
endlocal
