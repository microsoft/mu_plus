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
@echo Please note that the capsule generated here does not set CAPSULE_FLAGS_PERSIST_ACROSS_RESET or
@echo CAPSULE_FLAGS_INITIATE_RESET flags so that the capsule can be tested with nt32 platform successfully.
@echo These flags may need to be set for a production platform by changing this script as following:
@echo GenTools\GenFv.exe -o 3rdPartyDevelopmentCapsule.cap -g 6dcbd5ed-e82d-4c44-bda1-7194199ad92a --capsule -v -f PayloadS3.bin --capFlag PersistAcrossReset --capFlag InitiateReset
@echo (GenTools\GenFv.exe -h for more info.)
@echo:

pause
@echo:

if not exist GenTools\GenMsPayloadHeader.exe goto ERROR
if not exist GenTools\GenFmpImageAuth.py goto ERROR
if not exist GenTools\GenFmpCap.exe goto ERROR
if not exist GenTools\GenFv.exe goto ERROR
if not exist SamplePayload\Payload.bin goto ERROR
if not exist SAMPLE_DEVELOPMENT.pfx goto ERROR
if not exist Guid.txt goto ERROR

GenTools\GenMsPayloadHeader.exe -o PayloadS1.bin --version 0x01000002 --lsv 0x01000000 -p SamplePayload\Payload.bin -v
GenTools\GenFmpImageAuth.py -o PayloadS2.bin -p PayloadS1.bin --pfxfile SAMPLE_DEVELOPMENT.pfx

set /p GUID=<Guid.txt
@echo %GUID%

GenTools\GenFmpCap.exe -o PayloadS3.bin -p PayloadS2.bin %GUID% 1 0 -v
GenTools\GenFv.exe -o 3rdPartyDevelopmentCapsule.cap -g 6dcbd5ed-e82d-4c44-bda1-7194199ad92a --capsule -v -f PayloadS3.bin

:CLEANUP
if exist PayloadS1.bin del PayloadS1.bin
if exist PayloadS2.bin del PayloadS2.bin
if exist PayloadS3.bin del PayloadS3.bin
goto END

:ERROR
@echo One or more files not present

:END