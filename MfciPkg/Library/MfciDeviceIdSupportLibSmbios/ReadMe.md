# MFCI Device ID Support Library: SMBIOS instance example

## Background

MFCI-compatible platforms MUST publish MFCI targeting information prior to EndOfDxe. They may do so by either setting
all of the Per-Device Targeting Variables declared in `MfciVariables.h`.  Alternatively, then can implement an instance
of MFCI Device ID Support Library and set ```PcdMfciPopulateTargetFromDeviceIdSupportLib``` to ```TRUE```.

Refer to ```MfciDeviceIdSupportLib.h``` for this library's interface.

## About

This example instance of MfciDeviceIdSupportLib leverages ```EFI_SMBIOS_PROTOCOL``` to implement the library interface.
This library is invoked by MfciDxe in an event that triggers immediately prior to EndOfDxe, and expects all SMBIOS
values leveraged to be populated with correct MFCI targeting values at that time.  A platform might accomplish this
by populating SMBIOS values in another driver's entry point, constructor, or in an event upon arrival of
```EFI_SMBIOS_PROTOCOL```.

To leverage this library, add the following to your platform DSC:

```ini
[PcdsFeatureFlag]
gMfciPkgTokenSpaceGuid.PcdMfciPopulateTargetFromDeviceIdSupportLib|TRUE

[LibraryClasses]
MfciDeviceIdSupportLib|MfciPkg/Library/MfciDeviceIdSupportLibSmbios/MfciDeviceIdSupportLibSmbios.inf
```

## Copyright

Copyright (c) Microsoft Corporation
SPDX-License-Identifier: BSD-2-Clause-Patent
