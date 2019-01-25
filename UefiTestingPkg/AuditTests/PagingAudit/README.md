# Paging Audit

## SmmPagingAudit

### Code Basics

SMM is a privileged mode of the ia32/x64 cpu architecture.  In this environment nearly all system state can 
be inspected including that of the operating system, kernel, and hypervisor.  Due to it's 
capabilities SMM has become an area of interest for those searching to exploit the system. 
To help minimize the interest and impact of an exploit in SMM the SMI handlers should operate
in a least privileged model.  To do this standard paging can be leveraged to limit the SMI
handlers access.  Tianocore has a feature to enable paging within SMM and this tool helps confirm
the configuration being used.  This tool requires three parts to get a complete view.  
 
### SMM

The SMM driver must be included in your build and dispatched to SMM before the End Of Dxe.  It is
recommended that this driver should only be used on debug builds as it reports the entire
SMM memory environment to the caller.  The shell app will communicate to the SMM driver and 
request critical memory information including IDT, GDT, page tables, and loaded images.

### SMM Version App

The UEFI shell application collects system information from the DXE environment and then
communicates to the SMM driver/handler to collect necessary info from SMM.  It then 
writes this data to files and then that content is used by the windows scripts.

## DxePagingAudit

### Code Basics

The Dxe version of paging audit drvier/shell app intends to inspect all 4 levels of page
tables and their corresponding Read/Write/Executable permissions. The driver/shell app will
collect necessary memrory information from platform enviroment, then iterate through each
page entries and log them on to avaiable SimpleFileSystem. The collected *.dat files can be
parsed using Windows\PagingReportGenerator.py.

### DXE Driver

The DXE Driver registers an event to be notified on Exit Boot Services (to change this,
replace gEfiEventExitBootServicesGuid with a different event GUID), which will then trigger
the paging information collection.

### DXE Version App

The DXE version of UEFI shell application collects necessary system and memory information
from DXE when invoked from Shell environment.

## Windows

The Windows script will look at the *.DAT files, parse their content, check for errors
and then insert the formatted data into the Html report file.  This report file is then double-clickable
by the end user/developer to review the posture of the SMM environment.  The Results tab applies 
our suggested rules for SMM to show if the environment passes or fails.  
If it fails the filters on the data tab can be configured to show where the problem exists.  


## Usage / Enabling on EDK2 based system

First, for the SMM driver and app you need to add them to your DSC file for your project so they get compiled.

### SMM Paging Audit

```
[Components.X64]  
  UefiTestingPkg\AuditTests\PagingAudit\SmmPagingAudit\Smm\SmmPagingAuditSmm.inf  
  UefiTestingPkg\AuditTests\PagingAudit\SmmPagingAudit\App\SmmPagingAuditApp.inf
```

Next, you must add the SMM driver to a firmware volume in your FDF that can dispatch SMM modules.    
```
INF UefiTestingPkg\AuditTests\PagingAudit\SmmPagingAudit\Smm\SmmPagingAuditSmm.inf
```

Third, after compiling your new firmware you must:
1. flash that image on the system.  
2. Copy the SmmPagingAuditApp.efi to a USB key

Then, boot your system running the new firmware to the shell and run the app. The tool will create a set of *.dat files on the same USB key. 

On a Windows PC, run the Python script on the data found on your USB key.  

Finally, double-click the HTML output file and check your results.   

### DXE Paging Audit

#### DxePagingAuditDxe

1. Add the following entry to platform dsc file;
```
[Components.X64] 
    UefiTestingPkg\AuditTests\PagingAudit\DxePagingAudit\Dxe\DxePagingAuditDxe.inf
```
2. Add the driver to a firmware volume in your FDF that can dispatch it;
```
INF UefiTestingPkg\AuditTests\PagingAudit\DxePagingAudit\Dxe\DxePagingAuditDxe.inf
```
3. After compiling your new firmware you must:<BR>
    a. flash that image on the system.
4. Boot your system running the new firmware to the shell with a USB plugged in. If the USB
disk is FS0:\\, the files should be in FS1:\\. Copy them to the flash drive:
```
copy FS1:\*.dat FS0:\
```
5. On a Windows PC, run Windows\PagingReportGenerator.py script with the data found on your
USB key. Please use the following command for detailed script instruction:
```
PagingReportGenerator.py -h
```
6. Double-click the HTML output file and check your results;

#### DxePagingAuditApp

1. Add the following entry to platform dsc file;
```
[Components.X64] 
    UefiTestingPkg\AuditTests\PagingAudit\DxePagingAudit\App\DxePagingAuditApp.inf
```
2. Compile the newly added application and copy DxePagingAuditApp.efi to a USB key;
3. Boot your system to the shell with the USB plugged in. If the USB disk is FS0:\, the files
should be in FS1:\\. Copy them to the flash drive:
```
FS0:\
DxePagingAuditApp.efi
copy FS1:\*.dat FS0:\
```
4. Follow step 5 - 6 from DxePagingAuditDxe section;

## Copyright

Copyright (c) 2018 - 2019, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
