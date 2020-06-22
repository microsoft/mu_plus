@echo off
REM @file
REM
REM Copyright (c) Microsoft Corporation
REM SPDX-License-Identifier: BSD-2-Clause-Patent
REM
REM Helps sign MFCI policies using signtool.exe

pushd .
cd %~dp0
REM Signing using the Win10 WDK
echo "USAGE:  sign.bat <file_to_P7_embed_sign> <private_cert.pfx>"

set KIT=10
set VER=bin\10.0.18362.0
REM set OID=1.3.6.1.4.1.311.79.1
set OID=1.2.840.113549.1.7.1
set EKU_TEST=1.3.6.1.4.1.311.45.255.255
set EKU_INVALID=1.3.6.1.4.1.311.255.0.0
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\"signtool sign /fd SHA256 /p7 . /p7ce Embedded /p7co %OID% /f %2 %1
