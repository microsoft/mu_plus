@echo off
rem @file
rem
rem Script to create an unsigned settings packet.
rem
rem Copyright (c), Microsoft Corporation
rem SPDX-License-Identifier: BSD-2-Clause-Patent
rem

setlocal

if .%1 NEQ .* goto Specific

set MfgParm=--SMBIOSMfg ""
set ProductParm=--SMBIOSProd ""
set SerialParm=--SMBIOSSerial ""

set OutputFileName=DfciUpdate.Dfi

goto Continue

:Specific

if .%1 EQU . goto Error
if .%2 EQU . goto Error
if .%3 EQU . goto Error

set MfgParm=--SMBIOSMfg %1
set ProductParm=--SMBIOSProd %2
set SerialParm=--SMBIOSSerial %3

set OutputFileName=%3_%2_%1.Dfi

:Continue

echo Creating  %OutputFileName%

echo python.exe ..\..\Support\Python\GenerateSettingsPacketData.py --Step1Enable --PrepResultFile Unsigned_Settings_apply.bin --XmlFilePath UnsignedSettings.xml --HdrVersion 2 %MfgParm% %ProductParm% %SerialParm%
python.exe ..\..\Support\Python\GenerateSettingsPacketData.py --Step1Enable --PrepResultFile Unsigned_Settings_apply.bin --XmlFilePath DfciSettings.xml --HdrVersion 2 %MfgParm% %ProductParm% %SerialParm%

..\..\Support\Python\BldDskPkt.py -s Unsigned_Settings_apply.bin -o %OutputFileName%

rem erase Unsigned_Settings_apply.bin

goto Done

:Error
echo  "GenUsb <MfgName> <Product Name> <SerialNumber>"
echo  "or"
echo  "GenUsb *"
:Done