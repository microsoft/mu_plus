
# UEFI Testing Package
## &#x1F539; Copyright
Copyright (c) 2017, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## &#x1F539; About
This package adds tests.  

## &#x1F539; System Functional tests
Tests that invoke system functions and query system state for verification.

### MemmapAndMatTestApp
This test compares the UEFI memory map and Memory Attributes Table against known
requirements.  The MAT has strict requirements to allow OS usage and page protections. 

### MorLockTestApp
This test verifies the UEFI variable store handling of MorLock v1 and v2 behavior. 

## &#x1F539; System Audit tests 
UEFI applications that collect data from the system and then that data can be used to
compare against known good values.  

### UefiVarLockAudit
Audit collection tool that gathers information about UEFI variables.  This allows
auditing the variables within a system, checking attributes, and confirming
read/write status.  This information is put into an XML file that allows for
easy comparison and programatic auditing.  
#### UEFI
UEFI shell application that gets the current variable information from the UEFI 
shell and creates an XML file. 
#### Windows
Python 2.7 script that can be run from the Windows OS.  It takes the UEFI created
XML file as input and then queries all listed variables and updates the XML with 
access and status codes.  This gives additional verification for variables that 
may employ late locking or other protections from OS access.  

