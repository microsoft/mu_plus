# Paging Audit

## Contents

- [Paging Audit](#paging-audit)
  - [Contents](#contents)
  - [DXE Paging Audit](#dxe-paging-audit)
    - [DXE Driver](#dxe-driver)
    - [DXE Shell App](#dxe-shell-app)
      - [Mode 1: Shell-Based Unit Test](#mode-1-shell-based-unit-test)
      - [Mode 2: Paging Audit Collection Tool](#mode-2-paging-audit-collection-tool)
  - [SMM Paging Audit](#smm-paging-audit)
    - [SMM Driver](#smm-driver)
    - [SMM Shell App](#smm-shell-app)
  - [Parsing the Paging Data](#parsing-the-paging-data)
  - [Platform Configuration](#platform-configuration)
    - [SMM Paging Audit Configuration](#smm-paging-audit-configuration)
    - [DXE Paging Audit Driver Configuration](#dxe-paging-audit-driver-configuration)
    - [DXE Paging Audit App Configuration](#dxe-paging-audit-app-configuration)
  - [Copyright](#copyright)

## DXE Paging Audit

The DXE paging audit is a tool to analyze the page/translation tables of the platform to check
compliance with
[Enhanced Memory Protection](https://microsoft.github.io/mu/WhatAndWhy/enhancedmemoryprotection/),
debug paging related issues, and understand the memory layout of the platform. The DXE version
of the audit operates only on the page/translation tables of the DXE environment and not the
tables used in SMM.

### DXE Driver

The DXE Driver registers an event to be notified on Mu Pre Exit Boot Services (to change this,
replace gEfiEventBeforeExitBootServicesGuid with a different event GUID), which will then trigger
the paging information collection. The driver will then save the collected information to
an available simple file system. The driver version of the DXE paging audit should be used
when the intent is to capture a snapshot of the page/translation table data at an arbitrary
point in boot.

### DXE Shell App

The DXE version of UEFI shell app has two modes of operation and **does not require**
**the DXE driver**.

#### Mode 1: Shell-Based Unit Test

Calling the app in the following way:

`DxePagingAuditTestApp.efi`  
OR  
`DxePagingAuditTestApp.efi -r`

Will run the app as a shell-based unit test. The following tests will be run and the
results will be saved to an XML file in the same file system as the app (or the next
available writable simple file system):

- **NoReadWriteExecute:** Checks if the page/translation table has any readable, writable,
and executable regions.  
- **UnallocatedMemoryIsRP:** Checks that all EfiConventionalMemory is EFI_MEMORY_RP or
is not mapped.  
- **IsMemoryAttributeProtocolPresent:** Checks if the EFI Memory Attribute Protocol
is installed.  
- **NullPageIsRp:** Checks if page 0 is EFI_MEMORY_RP or is not mapped.  
- **MmioIsXp:** Checks that MMIO regions in the EFI memory map are EFI_MEMORY_XP.  
- **ImageCodeSectionsRoDataSectionsXp:** Checks that loaded image sections containing
code are EFI_MEMORY_RO and sections containing data are EFI_MEMORY_XP.  
- **BspStackIsXpAndHasGuardPage:** Checks that the stack is EFI_MEMORY_XP and has an
EFI_MEMORY_RP page at the base to catch overflow.  

#### Mode 2: Paging Audit Collection Tool

Calling the app in the following way:

`DxePagingAuditTestApp.efi -d`

Will collect paging information and save the data in the same file system as
the app (or the next available writable simple file system). See the
[Parsing the Paging Data](#parsing-the-paging-data) for instructions on parsing
the collected data.

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
writes this data to files for parsing by Python scripts.

## Parsing the Paging Data

The Python Windows\PagingReportGenerator.py script will parse the *.dat files into
a human-readable HTML report file to inspect
the page table at the collection point. The Results tab of the HTML shows the results of testing
the parsed data against the tests run in the shell app, and each failed test presents a filter
button to show the regions of memory which do not pass the test.

Run the following command for help with the script:

`PagingReportGenerator.py -h`

The script is run with the following command:

```cmd
PagingReportGenerator.py -i <input directory> -o <output report> [-p <platform name>] [--PlatformVersion <platform version>] [-l <filename>] [--debug]
```

**Required Input Parameters:**  
  `-i <input directory>`: The directory containing the *.dat files to be parsed.  
  `-o <output report>`: The name of the HTML report file to be generated.

**Optional Input Parameters:**  
  `-p <platform name>`: The name of the platform the data was collected on.  
  `--PlatformVersion <platform version>`: The version of the platform the data was collected on.  
  `-l <filename>`: The name of the log file to be generated.  
  `--debug`: Set logging level to DEBUG (will be INFO by default).

## Platform Configuration

### SMM Paging Audit Configuration

```text
[PcdsFixedAtBuild]
  # Optional: Virtual platforms that do not support SMRRs can add below change to skip the auditing related to SMRR
  gUefiTestingPkgTokenSpaceGuid.PcdPlatformSmrrUnsupported|TRUE

[Components]
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

### DXE Paging Audit Driver Configuration

Add the following to the platform DSC file:

```text
[PcdsFixedAtBuild]
    # Optional: Virtual platforms that do not support SMRRs can add below change to skip the auditing related to SMRR
    gUefiTestingPkgTokenSpaceGuid.PcdPlatformSmrrUnsupported|TRUE

[Components]
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

### DXE Paging Audit App Configuration

Add the following entry to platform dsc file and compile the new firmware image:

```text
[Components]
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

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
