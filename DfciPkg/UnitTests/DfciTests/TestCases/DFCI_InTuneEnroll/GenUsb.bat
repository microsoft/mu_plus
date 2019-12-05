@echo off
rem @file
rem
rem Script to create the same Intune Enroll test packets for a device under test.
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

python.exe ..\..\Support\Python\GenerateCertProvisionData.py --CertFilePath ..\..\Certs\DDS_CA.cer --Step2AEnable --Signing2APfxFile ..\..\Certs\DDS_Leaf.pfx --Step2BEnable --Step2Enable --SigningPfxFile ..\..\Certs\ZTD_Leaf.pfx --Step3Enable --FinalizeResultFile OwnerEnroll_Provision_apply.bin --Step1Enable --Identity 1 --HdrVersion 2 --SMBIOSMfg %Mfg% --SMBIOSProd %Product% --SMBIOSSerial %Serial%

python.exe ..\..\Support\Python\GenerateCertProvisionData.py --CertFilePath ..\..\Certs\MDM_CA.cer --Step2AEnable --Signing2APfxFile ..\..\Certs\MDM_Leaf.pfx --Step2BEnable --Step2Enable --SigningPfxFile ..\..\Certs\DDS_Leaf.pfx --Step3Enable --FinalizeResultFile UserEnroll_Provision_apply.bin --Step1Enable --Identity 2 --HdrVersion 2 --SMBIOSMfg %Mfg% --SMBIOSProd %Product% --SMBIOSSerial %Serial%

python.exe ..\..\Support\Python\GeneratePermissionPacketData.py --Step2Enable --SigningPfxFile ..\..\Certs\DDS_Leaf.pfx --Step3Enable --FinalizeResultFile OwnerPermissions_Permission_apply.bin --Step1Enable --XmlFilePath DfciPermission.xml --HdrVersion 2 --SMBIOSMfg %Mfg% --SMBIOSProd %Product% --SMBIOSSerial %Serial%

python.exe ..\..\Support\Python\GeneratePermissionPacketData.py --Step2Enable --SigningPfxFile ..\..\Certs\MDM_Leaf.pfx --Step3Enable --FinalizeResultFile UserPermissions_Permission_apply.bin --Step1Enable --XmlFilePath DfciPermission2.xml --HdrVersion 2 --SMBIOSMfg %Mfg% --SMBIOSProd %Product% --SMBIOSSerial %Serial%

python.exe ..\..\Support\Python\GenerateSettingsPacketData.py --Step2Enable --SigningPfxFile ..\..\Certs\DDS_Leaf.pfx --Step3Enable --FinalizeResultFile OwnerSettings_Settings_apply.bin --Step1Enable --XmlFilePath DfciSettings.xml --HdrVersion 2 --SMBIOSMfg %Mfg% --SMBIOSProd %Product% --SMBIOSSerial %Serial%

python.exe ..\..\Support\Python\GenerateSettingsPacketData.py --Step2Enable --SigningPfxFile ..\..\Certs\MDM_Leaf.pfx --Step3Enable --FinalizeResultFile UserSettings_Settings_apply.bin --Step1Enable --XmlFilePath DfciSettings2.xml --HdrVersion 2 --SMBIOSMfg %Mfg% --SMBIOSProd %Product% --SMBIOSSerial %Serial%

..\..\Support\Python\BldDskPkt.py -i OwnerEnroll_Provision_apply.bin -p OwnerPermissions_Permission_apply.bin -s OwnerSettings_Settings_apply.bin -i2 UserEnroll_Provision_apply.bin -p2 UserPermissions_Permission_apply.bin -s2 UserSettings_Settings_apply.bin -o %Serial%_%Product%_%Mfg%.Dfi

erase OwnerEnroll_Provision_apply.bin OwnerPermissions_Permission_apply.bin OwnerSettings_Settings_apply.bin
erase UserEnroll_Provision_apply.bin UserPermissions_Permission_apply.bin UserSettings_Settings_apply.bin

goto Done

:Error
echo  GenUsb <MfgName> <Product Name> <SerialNumber>

:Done