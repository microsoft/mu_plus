# Paging Audit

## SmmPagingAudit

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

The Dxe version of paging audit driver/shell app intends to inspect all 4 levels of page
tables and their corresponding Read/Write/Execute/Present attributes. The driver/shell app will
collect necessary memory information from platform environment, then iterate through each
page entries and log them on to available SimpleFileSystem. The collected *.dat files can be
parsed using Windows\PagingReportGenerator.py.

### DXE Driver

The DXE Driver registers an event to be notified on Mu Pre Exit Boot Services (to change this,
replace gMuEventPreExitBootServicesGuid with a different event GUID), which will then trigger
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

```text
[PcdsFixedAtBuild.X64]
  # Optional: Virtual platforms that do not support SMRRs can add below change to skip the auditing related to SMRR
  gUefiTestingPkgTokenSpaceGuid.PcdPlatformSmrrUnsupported|TRUE

[Components.X64]
  UefiTestingPkg/AuditTests/PagingAudit/UEFI/SmmPagingAuditDriver.inf
  UefiTestingPkg/AuditTests/PagingAudit/UEFI/SmmPagingAuditTestApp.inf
```

Next, you must add the SMM driver to a firmware volume in your FDF that can dispatch SMM modules.

```text
INF UefiTestingPkg/AuditTests/PagingAudit/UEFI/SmmPagingAuditDriver.inf
```

Third, after compiling your new firmware you must:

1. Flash that image on the system.
2. Copy the SmmPagingAuditTestApp.efi to a USB key

Then, boot your system running the new firmware to the shell and run the app. The tool will create a set of *.dat files on
the same USB key.

On a Windows PC, run the Python script on the data found on your USB key.

Finally, double-click the HTML output file and check your results.

### DXE Paging Audit

#### DxePagingAuditDxe

1. Add the following entry to platform dsc file;

    ```text
    [PcdsFixedAtBuild.X64]
      # Optional: Virtual platforms that do not support SMRRs can add below change to skip the auditing related to SMRR
      gUefiTestingPkgTokenSpaceGuid.PcdPlatformSmrrUnsupported|TRUE

    [Components.X64]
        UefiTestingPkg/AuditTests/PagingAudit/UEFI/DxePagingAuditDriver.inf
    ```

2. Add the driver to a firmware volume in your FDF that can dispatch it;

    ```text
    INF UefiTestingPkg/AuditTests/PagingAudit/UEFI/DxePagingAuditDriver.inf
    ```

3. After compiling your new firmware you must flash that image on the system.
4. Boot your system running the new firmware to the OS then reboot to UEFI shell with a USB plugged in. If the USB disk is
    `FS0:\\`, the files should be in `FS1:\\`. Copy them to the flash drive:

    ```cmd
    copy FS1:\*.dat FS0:\
    ```

5. On a Windows PC, run Windows\PagingReportGenerator.py script with the data found on your USB key. Please use the following
command for detailed script instruction:

    ```cmd
    PagingReportGenerator.py -h
    ```

6. Double-click the HTML output file and check your results.

#### DxePagingAuditTestApp

1. Add the following entry to platform dsc file:

    ```text
    [Components.X64]
        UefiTestingPkg/AuditTests/PagingAudit/UEFI/DxePagingAuditTestApp.inf
    ```

2. Compile the newly added application and copy DxePagingAuditTestApp.efi to a USB key.
3. Boot your system to the shell with the USB plugged in. If the USB disk is `FS0:\`, the files
should be in `FS1:\\`. Copy them to the flash drive:

    ```cmd
    FS0:\
    DxePagingAuditTestApp.efi
    copy FS1:\*.dat FS0:\
    ```

4. Follow step 5 - 6 from DxePagingAuditDxe section.

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
