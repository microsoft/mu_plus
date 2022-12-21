@echo off
rem
echo * --- Building Expected_Request.json --- *
rem
python.exe GenResponses.py -l ..\..\Certs\DFCI_HTTPS.pfx -w ..\..\Certs\DDS_CA2.pfx -o ..\src\Requests\Expected_Request.json
if %ERRORLEVEL% NEQ 0 goto :error

rem
echo * --- Building UnExpected_Request.json --- *
rem
python.exe GenResponses.py -l ..\..\Certs\DFCI_HTTPS.pfx -w ..\..\Certs\DDS_CA.pfx -o ..\src\Responses\UnExpected_Request.json
if %ERRORLEVEL% NEQ 0 goto :error

rem
echo * --- Building Recovery_Response.json --- *
rem
python.exe ..\..\Support\Python\GenerateCertProvisionData.py --SMBIOSMfg "" --SMBIOSProd "" --SMBIOSSerial "" --Step2Enable --SigningPfxFile ..\..\Certs\DDS_Leaf2.pfx --Step3Enable --FinalizeResultFile UnEnroll_apply.bin --Step1Enable --Identity 1 --HdrVersion 2
if %ERRORLEVEL% NEQ 0 goto :error
python.exe GenResponses.py -i UnEnroll_apply.bin -o ..\src\Responses\Recovery_Response.json
if %ERRORLEVEL% NEQ 0 goto :error

rem
echo * --- Building Bootstrap_NULLResponse.json --- *
rem
python.exe GenResponses.py -null -o ..\src\Responses\Bootstrap_NULLResponse.json
if %ERRORLEVEL% NEQ 0 goto :error

rem
echo * --- Building Bootstrap_Response.json --- *
rem
rem Roll owner key from DDS_CA.cer to DDS_CA2.cer
rem Roll user key from MDM_CA.cer to MDM_CA2.cer
rem Replace TLS cert with same on as enrolled.
rem
python.exe ..\..\Support\Python\GenerateCertProvisionData.py --SMBIOSMfg "" --SMBIOSProd "" --SMBIOSSerial "" --CertFilePath ..\..\Certs\DDS_CA2.cer --Step2AEnable --Signing2APfxFile ..\..\Certs\DDS_Leaf2.pfx --Step2BEnable --Step2Enable --SigningPfxFile ..\..\Certs\DDS_Leaf.pfx --Step3Enable --FinalizeResultFile OwnerTransition_apply.bin --Step1Enable --Identity 1 --HdrVersion 2
if %ERRORLEVEL% NEQ 0 goto :error
python.exe ..\..\Support\Python\GenerateCertProvisionData.py --SMBIOSMfg "" --SMBIOSProd "" --SMBIOSSerial "" --CertFilePath ..\..\Certs\MDM_CA2.cer --Step2AEnable --Signing2APfxFile ..\..\Certs\MDM_Leaf2.pfx --Step2BEnable --Step2Enable --SigningPfxFile ..\..\Certs\MDM_Leaf.pfx --Step3Enable --FinalizeResultFile UserTransition_apply.bin --Step1Enable --Identity 2 --HdrVersion 2
if %ERRORLEVEL% NEQ 0 goto :error
python.exe ..\..\Support\Python\InsertCertIntoXML.py --BinFilePath ..\..\certs\DFCI_HTTPS.cer --OutputFilePath DfciSettings.xml --PatternFilePath DfciSettingsPattern.xml
if %ERRORLEVEL% NEQ 0 goto :error
python.exe ..\..\Support\Python\GenerateSettingsPacketData.py --SMBIOSMfg "" --SMBIOSProd "" --SMBIOSSerial "" --HdrVersion 2 --Step1Enable --Step2Enable --SigningPfxFile ..\..\Certs\DDS_Leaf2.pfx --Step3Enable --FinalizeResultFile OwnerSettings_apply.bin --XmlFilePath DfciSettings.xml --HdrVersion 2
if %ERRORLEVEL% NEQ 0 goto :error
python.exe GenResponses.py -t OwnerTransition_apply.bin -t2 UserTransition_apply.bin -s OwnerSettings_apply.bin -o ..\src\Responses\Bootstrap_Response.json
if %ERRORLEVEL% NEQ 0 goto :error

erase UnEnroll_apply.bin
erase OwnerTransition_apply.bin
erase UserTransition_apply.bin
erase OwnerSettings_apply.bin
erase DfciSettings.xml

goto exit

:error
echo * ==== An ERROR occurred === *
:exit
echo * --- Responses built successfully --- *
