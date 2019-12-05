@echo off
rem @file
rem
rem Script to run a testcase.
rem
rem Copyright (c), Microsoft Corporation
rem SPDX-License-Identifier: BSD-2-Clause-Patent

setlocal
for /f "tokens=2 delims==" %%a in ('wmic OS Get localdatetime /value') do set "dt=%%a"
set "YY=%dt:~2,2%" & set "YYYY=%dt:~0,4%" & set "MM=%dt:~4,2%" & set "DD=%dt:~6,2%"
set "HH=%dt:~8,2%" & set "Min=%dt:~10,2%" & set "Sec=%dt:~12,2%"

set date=%YYYY%%MM%%DD%_%HH%%Min%%Sec%

set Platform=Platforms\SimpleFTDI\Args.txt

if .%2 EQU . (set TargetIP=10.124.238.229) else (set TargetIP=%2)

set OutputBase=C:\TestLogs\robot\%~n1\logs_%date%

@echo on
python.exe -m robot.run -L TRACE -x %~n1.xml -A %Platform% -v IP_OF_DUT:%TargetIp% -v TEST_OUTPUT_BASE:%OutputBase% -d %OutputBase% TestCases\%~n1\run.robot

@echo off
endlocal
