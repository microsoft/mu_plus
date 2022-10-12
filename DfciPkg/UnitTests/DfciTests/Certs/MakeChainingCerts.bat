@echo off
rem @file
rem
rem Script to create the full test certificate set for DFCI testing.
rem
rem Copyright (c), Microsoft Corporation
rem SPDX-License-Identifier: BSD-2-Clause-Patent
rem

pushd .
cd %~dp0

REM Creating Certs requires the Win10 WDK.  If you don't have the MakeCert tool you can try the 8.1 kit (just change the KIT=10 to KIT=8.1, and VER=bin)

set KIT=10
set VER=bin\10.0.17763.0

rem Certs for ZTD

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -r -cy authority -len 4096 -m 120 -a sha256 -sv ZTD_Root.pvk -pe -ss my -n "CN=ZTD_Root, O=Palindrome, C=US" ZTD_Root.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk ZTD_Root.pvk -spc ZTD_Root.cer -pfx ZTD_Root.pfx

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -cy authority -len 4096 -m 61 -a sha256 -ic ZTD_Root.cer -iv ZTD_Root.pvk -sv ZTD_CA.pvk -pe -ss my -n "CN=ZTD_CA, O=Palindrome, C=US" ZTD_CA.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk ZTD_CA.pvk -spc ZTD_CA.cer -pfx ZTD_CA.pfx

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -len 4096 -m 60 -a sha256 -ic ZTD_CA.cer -iv ZTD_CA.pvk -sv ZTD_Leaf.pvk -pe -ss my -sky exchange -n "CN=ZTD_Leaf, O=Palindrome, C=US" ZTD_Leaf.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk ZTD_Leaf.pvk -spc ZTD_Leaf.cer -pfx ZTD_Leaf.pfx

rem Certs for DDS

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -r -cy authority -len 4096 -m 120 -a sha256 -sv DDS_Root.pvk -pe -ss my -n "CN=DDS.OnMicrosoft.com Device Guard Root, O=OnMicrosoft.com, C=US" DDS_Root.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk DDS_Root.pvk -spc DDS_Root.cer -pfx DDS_Root.pfx

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -cy authority -len 4096 -m 61 -a sha256 -ic DDS_Root.cer -iv DDS_Root.pvk -sv DDS_CA.pvk -pe -ss my -n "CN=DDS.OnMicrosoft.com CA, O=OnMicrosoft.com, C=US" DDS_CA.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk DDS_CA.pvk -spc DDS_CA.cer -pfx DDS_CA.pfx

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -len 2048 -m 60 -a sha256 -ic DDS_CA.cer -iv DDS_CA.pvk -sv DDS_Leaf.pvk -pe -ss my -n "CN=DDS.OnMicrosoft.com, O=OnMicrosoft.com, C=US" DDS_Leaf.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk DDS_Leaf.pvk -spc DDS_Leaf.cer -pfx DDS_Leaf.pfx

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -cy authority -len 4096 -m 61 -a sha256 -ic DDS_Root.cer -iv DDS_Root.pvk -sv DDS_CA2.pvk -pe -ss my -n "CN=DDS2.OnMicrosoft.com CA, O=OnMicrosoft.com, C=US" DDS_CA2.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk DDS_CA2.pvk -spc DDS_CA2.cer -pfx DDS_CA2.pfx

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -len 2048 -m 60 -a sha256 -ic DDS_CA2.cer -iv DDS_CA2.pvk -sv DDS_Leaf2.pvk -pe -ss my -n "CN=DDS2.OnMicrosoft.com, O=OnMicrosoft.com, C=US" DDS_Leaf2.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk DDS_Leaf2.pvk -spc DDS_Leaf2.cer -pfx DDS_Leaf2.pfx

rem Certs for MDM

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -r -cy authority -len 4096 -m 120 -a sha256 -sv MDM_Root.pvk -pe -ss my -n "CN=Sample_MDM_Root, O=Corportaion, C=US" MDM_Root.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk MDM_Root.pvk -spc MDM_Root.cer -pfx MDM_Root.pfx

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -cy authority -len 2048 -m 61 -a sha256 -ic MDM_Root.cer -iv MDM_Root.pvk -sv MDM_CA.pvk -pe -ss my -n "CN=Sample_MDM_CA, O=Corportaion, C=US" MDM_CA.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk MDM_CA.pvk -spc MDM_CA.cer -pfx MDM_CA.pfx

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -len 2048 -m 60 -a sha256 -ic MDM_CA.cer -iv MDM_CA.pvk -sv MDM_Leaf.pvk -pe -ss my -n "CN=Sample_MDM_Leaf, O=Corportaion, C=US" MDM_Leaf.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk MDM_Leaf.pvk -spc MDM_Leaf.cer -pfx MDM_Leaf.pfx

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -cy authority -len 2048 -m 61 -a sha256 -ic MDM_Root.cer -iv MDM_Root.pvk -sv MDM_CA2.pvk -pe -ss my -n "CN=Sample_MDM_CA2, O=Corporation, C=US" MDM_CA2.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk MDM_CA2.pvk -spc MDM_CA2.cer -pfx MDM_CA2.pfx

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -len 2048 -m 60 -a sha256 -ic MDM_CA2.cer -iv MDM_CA2.pvk -sv MDM_Leaf2.pvk -pe -ss my -n "CN=Sample_MDM_Leaf2, O=Corporation, C=US" MDM_Leaf2.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk MDM_Leaf2.pvk -spc MDM_Leaf2.cer -pfx MDM_Leaf2.pfx

rem Cert for HTTPS

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -eku 1.3.6.1.5.5.7.3.1  /n "CN=mikeytbds3.eastus.cloudapp.azure.com, O=Dfci Testing, C=US" /r /h 0 -sky signature /sv DFCI_HTTPS.pvk DFCI_HTTPS.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe" /pvk DFCI_HTTPS.pvk /spc DFCI_HTTPS.cer /pfx DFCI_HTTPS.pfx

:end
 popd

