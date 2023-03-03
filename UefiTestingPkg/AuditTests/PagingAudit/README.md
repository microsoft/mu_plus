## SMM Paging Audit

SMM is a privileged mode of the IA32/X64 cpu architecture.  In this environment nearly all system state can
be inspected including that of the operating system, kernel, and hypervisor.  Due to it's
capabilities SMM has become an area of interest for those searching to exploit the system.
To help minimize the interest and impact of an exploit in SMM the SMI handlers should operate
in a least privileged model.  To do this standard paging can be leveraged to limit the SMI
handlers access.  Tianocore has a feature to enable paging within SMM and this tool helps confirm
the configuration being used.  This tool requires three parts to get a complete view.

### SMM Driver

The SMM driver must be included in your build and dispatched to SMM before the end of DXE.  It is
recommended that this driver should only be used on debug builds as it reports the entire
SMM memory environment to the caller.  The shell app will communicate to the SMM driver and
request critical memory information including IDT, GDT, page tables, and loaded images.

### SMM Shell App

The UEFI shell application collects system information from the DXE environment and then
communicates to the SMM driver/handler to collect necessary info from SMM.  It then
writes this data to files and then that content is used by the windows scripts.

## DXE Paging Audit

The DXE version of the paging audit and driver have overlapping purpose. Both are capable of
inspecting the page/translation tables to collect all leaf entries for parsing using
Windows\PagingReportGenerator.py and both are compatible with X64 and AARCH64 architectures.

### DXE Driver

The DXE Driver registers an event to be notified on Mu Pre Exit Boot Services (to change this,
replace gMuEventPreExitBootServicesGuid with a different event GUID), which will then trigger
the paging information collection. The driver will then save the collected information to
an available simple file system. The driver version of the DXE paging audit should be used
when the intent is to capture a snapshot of the page/translation table at a point in boot at
which the shell app cannot be run.

### DXE Version App

The DXE version of UEFI shell app has two modes of operation and **does not require the DXE driver**.
Calling the app without any parameters will run it as a unit test with the only current test being
to check every leaf entry in the page/translation table to ensure that no page is Read/Write/Execute.
Calling the app with the '-d' parameter will collect paging information and attempt to save it to the
same file system containing the app. If it cannot save to the app's file system, it will save to the
first available simple file system.

## Python Script

The Python script will parse the *.dat files into a human-readable HTML report file to inspect
the page table at the collection point. The Results tab of the HTML checks the data against our suggested
rules to act as a barometer for the security of the target system.

## Usage

### SMM Paging Audit Usage

```text
[PcdsFixedAtBuild.X64]
  # Optional: Virtual platforms that do not support SMRRs can add below change to skip the auditing related to SMRR
  gUefiTestingPkgTokenSpaceGuid.PcdPlatformSmrrUnsupported|TRUE

[Components.X64]
  UefiTestingPkg/AuditTests/PagingAudit/UEFI/SmmPagingAuditDriver.inf
  UefiTestingPkg/AuditTests/PagingAudit/UEFI/SmmPagingAuditTestApp.inf
```

The SMM driver will also need to be added to a firmware volume in the platform FDF so it can be dispatched:

```text
INF UefiTestingPkg/AuditTests/PagingAudit/UEFI/SmmPagingAuditDriver.inf
```

After compiling your new firmware, flash the image onto the target system and copy the built SmmPagingAuditTestApp.efi
to a USB key. Boot the target system running the new firmware to the shell and run the app. The tool will create
a set of *.dat files on an available simple file system on the target system. Run the Python script on the data
to create the HTML report.

### DXE Paging Audit Usage

#### DXE Paging Audit Driver

Add the following to the platform DSC file:

```text
[PcdsFixedAtBuild.X64]
    # Optional: Virtual platforms that do not support SMRRs can add below change to skip the auditing related to SMRR
    gUefiTestingPkgTokenSpaceGuid.PcdPlatformSmrrUnsupported|TRUE

[Components.X64]
    UefiTestingPkg/AuditTests/PagingAudit/UEFI/DxePagingAuditDriver.inf
```

Add the driver to a firmware volume in the platform FDF so it can be dispatched:

```text
INF UefiTestingPkg/AuditTests/PagingAudit/UEFI/DxePagingAuditDriver.inf
```

After compiling the new firmware, flash the firmware image on the system. and boot the system to the OS or UEFI shell
depending on at what point the paging snapshot is collected. Meaning, if the paging snapshot is collected at the end of
DXE, boot the system to the OS, then reboot back to the UEFI shell to collect the paging audit files.
If the paging snapshot is collected before the end of BDS, the system can be booted directly to the UEFI shell.
Use a USB or virtual disk to store the paging audit files by copying them from their deposited location (usually FS0).

Example if the paging audit files are on FS1 and the USB/virtual drive FS0:

```cmd
copy FS1:\*.dat FS0:\
```

#### DXE Paging Audit App

Add the following entry to platform dsc file and compile the new firmware image:

```text
[Components.X64]
    UefiTestingPkg/AuditTests/PagingAudit/UEFI/DxePagingAuditTestApp.inf
```

Copy the built DxePagingAuditTestApp.efi to a USB key or virtual drive and boot the system to the
UEFI shell with the drive attached. Run the app without any parameters to run it as a unit test or
with the '-d' parameter to collect the paging audit files.

Example the USB/virtual drive FS0:

```cmd
FS0:\DxePagingAuditTestApp.efi
```

the USB/virtual drive FS0

### Parsing the .dat files

Run the Python Windows\PagingReportGenerator.py script against the collected .dat files. Use the following
command for detailed script instruction:

```cmd
PagingReportGenerator.py -h
```

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
