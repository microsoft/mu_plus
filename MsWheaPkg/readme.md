# MS WHEA Package

## About
This package contains drivers and infrastructure for reporting errors and telemetry through the CPER (Common Platform Error Record) HwErrRecord interface, specifically targeting systems that also leverage WHEA (Windows Hardware Error Architecture).

The MsWhea drivers provide the same functionality at different stages of UEFI by binding to the REPORT_STATUS_CODE interface. Together, they store hardware errors and corrected faults into non-volatile memory that is later picked up by Windows. In Windows, this can be emitted as telemetry which is then used to identify errors and patterns for devices (as of time of writing any event besides EFI_GENERIC_ERROR_INFO will be sent).

Project MU has updated design which defines a specific section type under `gMuTelemetrySectionTypeGuid` to be used in CPER header for WHEA telemetry. All reported data will be formatted to this section. Please refer to `MuTelemetryCperSection.h` for field definitions.

This package also contains optional solutions for persisting error records in the pre-memory space. In early boot stages, drivers need to emit events as critical in order for them to be logged.

Detailed information about CPER can be found in Appendix N of the UEFI spec.

## How to include this driver
This driver must be included via DSC by including the EarlyStorageLib: MsWheaEarlyStorageLib|MsWheaPkg/Library/MsWheaEarlyStorageLib/MsWheaEarlyStorageLib.inf
Then the PEI stage driver will be included in the DSC Components.IA32 or PEI section: MsWheaPkg/MsWheaReport/Pei/MsWheaReportPei.inf
Then the DXE stage driver will be included in the Components.X64 or DXE section: MsWheaPkg/MsWheaReport/Dxe/MsWheaReportDxe.inf
Finally the SMM stage driver will be included in the Components.X64 or DXE_SMM_DRIVER section: MsWheaPkg/MsWheaReport/Smm/MsWheaReportSmm.inf

## Important Notes
The PCD value of __gMsWheaPkgTokenSpaceGuid.PcdDeviceIdentifierGuid__ must be overriden by *each* platform as this is later used in the CPER as the Platform ID (byte offset 32 in the record).

In the DXE phase, errors will be picked up by MsWhea for you. In early phases of boot, the errors must be explicitly logged. To do so, first add the library into your INF: MsWheaPkg/MsWheaPkg.dec

These headers must be included:
- MsWheaErrorStatus.h
- MuTelemetryCperSection.h
- Library/BaseLib.h

Once you're ready to log your error, you can fill the MU_TELEMETRY_CPER_SECTION_DATA and report the StatusCode. The failure type is of type EFI_STATUS_CODE_VALUE. Note that MsWhea drivers will only listen to report status code calls with (EFI_ERROR_MINOR | EFI_ERROR_CODE) or (EFI_ERROR_MAJOR | EFI_ERROR_CODE) at EFI_STATUS_CODE_TYPE.

```
ReportStatusCode( MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                  {{FAILURE_TYPE}});
```
for error under EFI_GENERIC_ERROR_FATAL severity. __Or:__
```
ReportStatusCode( MS_WHEA_ERROR_STATUS_TYPE_INFO,
                  {{FAILURE_TYPE}});
```
will report errors under EFI_GENERIC_ERROR_INFO severity.


## Testing
There is a UEFI shell application based unit test for WHEA reports.  This test attempts to verify basic functionality of public interfaces.  Check the **UnitTests** folder at the root of the package.
There is also a feature flag that can inject reports on each boot during various uefi stages. This flag should be off in production.

## Helper Lib

A helper lib to help integrate the MsWhea package has been provided. It is entirely optional and can be easily dropped in. It provides a few macros that are detailed here.

 - LOG_INFO_TELEMETRY ( ClassId, LibraryId, IhvId, ExtraData1, ExtraData )
 - LOG_INFO_EVENT ( ClassId )
 - LOG_CRITICAL_TELEMETRY ( ClassId, LibraryId, IhvId, ExtraData1, ExtraData )
 - LOG_CRITICAL_EVENT ( ClassId )
 - LOG_FATAL_TELEMETRY ( ClassId, LibraryId, IhvId, ExtraData1, ExtraData )
 - LOG_FATAL_EVENT ( ClassId )

Currently FATAL and CRITICAL map to the same level in the WHEA log but has been implemented to provide future functionality. The parameters are as follows.

 - ClassId - An EFI_STATUS_CODE_VALUE representing the event that has occurred. This should be unique enough to identify a module or region of code.
 - LibraryId - An optional EFI_GUID that should identify the library emitting this event
 - IhvId - An optional EFI_GUID that should identify the Ihv that is most applicable to this. This will often be NULL
 - ExtraData1 - A UINT64 that can be used to provide contextual or runtime data. It will be persisted and can be useful for debugging purposes.
 - ExtraData2 - Another UINT64 that is also used for contextual and runtime data similar to ExtraData1.

By default, gEfiCallerIdGuid is used as the module ID when using the macros. If you need complete control over the WHEA entry, you can use the LogTelemetry function to log a telemetry event. This is the function that the Macros use. More information on this function is in the public header for MuTelemetryHelperLib.

### Including the Helper Lib

The helper lib can easily be included by including it in your DSC.

```
[LibraryClasses]
  MuTelemetryHelperLib|MsWheaPkg/Library/MuTelemetryHelperLib/MuTelemetryHelperLib.inf
```

Since it is a BASE library, it is available for all architectures.


## Copyright
Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
