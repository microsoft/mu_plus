# MS WHEA Package

## About
This package contains drivers and infrastructure for reporting errors and telemetry through the CPER (Common Platform Error Record) HwErrRecord interface, specifically targeting systems that also leverage WHEA (Windows Hardware Error Architecture).

The MsWhea drivers provide the same functionality at different stages of UEFI by binding to the REPORT_STATUS_CODE interface. Together, they store hardware errors and corrected faults into non-volatile memory that is later picked up by Windows. In Windows, this can be emitted as telemetry which is then used to idenitfy errors and patterns for devices (as of time of writing any event besides EFI_GENERIC_ERROR_INFO will be sent).

This package also contains optional solutions for persisting error records in the pre-memory space. In early boot stages, drivers need to emit events as critical in order for them to be logged.

More information about CPER can be found in Appendix N of the UEFI spec.

## How to include this driver
This driver must be included via DSC by including the EarlyStorageLib: MsWheaEarlyStorageLib|MsWheaPkg/Library/MsWheaEarlyStorageLib/MsWheaEarlyStorageLib.inf
Then the PEI stage driver will be included in the DSC Components.IA32 or PEI section: MsWheaPkg/MsWheaReport/Pei/MsWheaReportPei.inf
Then the DXE stage driver will be included in the Components.X64 or DXE section: MsWheaPkg/MsWheaReport/Dxe/MsWheaReportDxe.inf

## Important Notes
The PCD value of __gMsWheaPkgTokenSpaceGuid.PcdDeviceIdentifierGuid__ must be overriden by *each* platform as this is later used in the CPER as the Platform ID (byte offset 32 in the record).

In the DXE phase, errors will be picked up by MsWhea for you. In early phases of boot, the errors must be explicitly logged. To do so, first add the library into your INF: MsWheaPkg/MsWheaPkg.dec

These headers must be included:
- MsWheaErrorStatus.h
- Library/BaseLib.h

You also must define the WheaHeader you will write to.

```
MS_WHEA_ERROR_HDR mMsWheaHdr; //create a header for the hardware error record we might create
SetMem(&mMsWheaHdr, sizeof(MS_WHEA_ERROR_HDR), 0);
mMsWheaHdr.Signature = MS_WHEA_ERROR_SIGNATURE;       // Fixed by design
mMsWheaHdr.Rev = MS_WHEA_REV_1;                       // Support extended CMOS Support
// mMsWheaHdr.Phase = 0;                              // Leave alone, filled by MsWhea RSC Handler
mMsWheaHdr.ErrorSeverity = EFI_GENERIC_ERROR_FATAL;   // So it can survie any immediate reboot
```

Once you're ready to log your error, you can set the CriticalInfo (UINT64) and ReporterID (UINT64) and report the StatusCode. The failure type is of type EFI_STATUS_CODE_VALUE.

```
ReportStatusCodeWithExtendedData( MS_WHEA_ERROR_STATUS_TYPE, 
                                  {{FAILURE_TYPE}},
                                  &mMsWheaHdr,
                                  sizeof(MS_WHEA_ERROR_HDR) );
```


## Testing
There is a UEFI shell application based unit test for WHEA reports.  This test attempts to verify basic functionality of public interfaces.  Check the **UnitTests** folder at the root of the package.  
There is also a feature flag that can inject reports on each boot during various uefi stages. This flag should be off in production.

## Copyright
Copyright (c) 2018, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
