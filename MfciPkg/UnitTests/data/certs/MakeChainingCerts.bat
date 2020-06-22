rem exit /B
@echo off

pushd .
cd %~dp0

REM Creating Certs requires the Win10 WDK.  If you don't have the MakeCert tool you can try the 8.1 kit (just change the KIT=10 to KIT=8.1, and VER=bin)

set KIT=10
set VER=bin\10.0.18362.0
set EKU_TEST=1.3.6.1.4.1.311.45.255.255,1.3.6.1.5.5.7.3.3
set EKU_INVALID=1.3.6.1.4.1.311.255.0.0,1.3.6.1.5.5.7.3.3

rem FwPolicy's Test Root, CAs, & Leaves 

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -r -cy authority -len 4096 -m 300 -a sha256 -sv Root.pvk -pe -ss my -n "CN=A Test Root, O=Contoso" Root.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk Root.pvk -spc Root.cer -pfx Root.pfx

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -cy authority -len 4096 -m 150 -a sha256 -ic Root.cer -iv Root.pvk -sv CA.pvk -pe -ss my -n "CN=A UEFI Test CA, O=Contoso" CA.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk CA.pvk -spc CA.cer -pfx CA.pfx

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -cy authority -len 4096 -m 150 -a sha256 -ic Root.cer -iv Root.pvk -sv CA-NotTrusted.pvk -pe -ss my -n "CN=NotTrusted UEFI Test CA, O=Contoso" CA-NotTrusted.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk CA-NotTrusted.pvk -spc CA-NotTrusted.cer -pfx CA-NotTrusted.pfx

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -len 3072 -m 30 -a sha256 -ic CA.cer -iv CA.pvk -sv Leaf-test.pvk -pe -ss my -sky exchange -n "CN=FwPolicy Test Leaf, O=Contoso" -eku %EKU_TEST% Leaf-test.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk Leaf-test.pvk -spc Leaf-test.cer -pfx Leaf-test.pfx

"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\MakeCert.exe" -len 3072 -m 30 -a sha256 -ic CA.cer -iv CA.pvk -sv Leaf-NoEku.pvk -pe -ss my -sky exchange -n "CN=Another Test Leaf Without an EKU, O=Contoso" Leaf-NoEku.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\%VER%\x64\Pvk2Pfx.exe"  -pvk Leaf-NoEku.pvk -spc Leaf-NoEku.cer -pfx Leaf-NoEku.pfx

ConvertCerts2Headers.py -p .
