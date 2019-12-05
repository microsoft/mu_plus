@echo off
rem @file
rem
rem Script to run the SetupDUT.ps1 file.
rem
rem Copyright (c), Microsoft Corporation
rem SPDX-License-Identifier: BSD-2-Clause-Patent

rem rem Usage: SetupDUT.cmd

pushd "%~dp0"
powershell -ExecutionPolicy bypass -file "%~dpn0.ps1" %*
popd

