@echo off

REM Copyright (c) 2016, Microsoft Corporation
REM
REM All rights reserved.
REM Redistribution and use in source and binary forms, with or without 
REM modification, are permitted provided that the following conditions are met:
REM 1. Redistributions of source code must retain the above copyright notice,
REM this list of conditions and the following disclaimer.
REM 2. Redistributions in binary form must reproduce the above copyright notice,
REM this list of conditions and the following disclaimer in the documentation
REM and/or other materials provided with the distribution.
REM
REM THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
REM ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
REM WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
REM IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
REM INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
REM BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
REM DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
REM LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
REM OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
REM ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


@echo This is a sample tool and should not be used in production environments
@echo:

pause
@echo:

pushd .
cd %~dp0

REM Creating Certs requires the Win10 WDK.  If you don't have the MakeCert tool you can try the 8.1 kit (just change the 10 to 8.1)
set KIT=8.1

"C:\Program Files (x86)\Windows Kits\%KIT%\bin\x64\MakeCert.exe" /n "CN=SAMPLE_DEVELOPMENT, O=Microsoft Corporation, C=US" /r /h 0 -sky exchange /sv SAMPLE_DEVELOPMENT.pvk SAMPLE_DEVELOPMENT.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\bin\x64\Pvk2Pfx.exe" /pvk SAMPLE_DEVELOPMENT.pvk /spc SAMPLE_DEVELOPMENT.cer /pfx SAMPLE_DEVELOPMENT.pfx
"C:\Program Files (x86)\Windows Kits\%KIT%\bin\x64\MakeCert.exe" /n "CN=SAMPLE_PRODUCTION, O=Microsoft Corporation, C=US" /r /h 0 -sky exchange /sv SAMPLE_PRODUCTION.pvk SAMPLE_PRODUCTION.cer
"C:\Program Files (x86)\Windows Kits\%KIT%\bin\x64\Pvk2Pfx.exe" /pvk SAMPLE_PRODUCTION.pvk /spc SAMPLE_PRODUCTION.cer /pfx SAMPLE_PRODUCTION.pfx

:end
 popd
