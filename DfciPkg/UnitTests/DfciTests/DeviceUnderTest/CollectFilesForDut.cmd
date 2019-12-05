@echo off
rem @file
rem
rem Script to collect the subset of files needed on the DUT to run RobotFramework.
rem
rem Copyright (c), Microsoft Corporation
rem SPDX-License-Identifier: BSD-2-Clause-Patent
rem

if .%1 EQU . goto Help

if NOT EXIST %1\ mkdir %1

copy %~p0\..\Support\Python\PyRobotRemote.py %1
copy %~p0\..\Support\Python\UefiVariablesSupportLib.py %1
copy %~p0\PyRobotServer.xml %1
copy %~p0\SetupDUT.* %1

goto Done

:Help
echo CollectFileForDUT PathOnUsb

:Done