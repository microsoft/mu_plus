@echo off
rem @file
rem
rem Script to create the same Intune Unenroll test packet for a device under test.
rem
rem Copyright (c), Microsoft Corporation
rem SPDX-License-Identifier: BSD-2-Clause-Patent
rem

if .%1 EQU . goto Error
if .%2 EQU . goto Error
if .%3 EQU . goto Error

set Mfg=%1
set Product=%2
set Serial=%3

python.exe ..\..\Support\Python\GenerateCertProvisionData.py --Step2Enable --SigningPfxFile ..\..\Certs\DDS_Leaf.pfx --Step3Enable --FinalizeResultFile OwnerUnenroll_Provision_apply.bin --Step1Enable --Identity 1 --HdrVersion 2 --SMBIOSMfg %Mfg% --SMBIOSProd %Product% --SMBIOSSerial %Serial%

..\..\Support\Python\BldDskPkt.py -i OwnerUnenroll_Provision_apply.bin -o %Serial%_%Product%_%Mfg%.Dfi

erase OwnerUnenroll_Provision_apply.bin
goto Done

:Error
echo  GenUsb <MfgName> <Product Name> <SerialNumber>

:Done