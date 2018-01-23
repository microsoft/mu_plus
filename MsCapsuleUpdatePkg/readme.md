# Capsule Update Feature

## Copyright

Copyright (c) 2016, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## About

**MsCapsuleUpdatePkg** Package is an environment that implements:
1.	Processing of (signed) UEFI capsules at boot time and runtime
2.	Scalable FMP architecture
3.	Support for FMP capsules
4.	Support for Windows Firmware Update Display capsules
5.	Firmware update progress display (text or graphical)
6.	Capsule update and FMP policies
7.	Support for multiple capsule verification certificates
8.  ESRT support

Note: this package provides the entire framework for updating device firmwares via capsules.  No actual device firmware is updated or attempted to be updated by this implementation.

## Modules/Drivers

### EfiSystemResourceTableDxe.inf

Implementation: MsCapsuleUpdatePkg\Universal\EsrtDxe

This driver adds EFI System Resource Table (ESRT) to the System Configuration Tables.  It prints the entire ESRT table at debug output.  This driver is added to the DSC and FDF files.

This driver implementation differs from EDK2's implementation in that it simply cosumes all FMP instances (`gEfiFirmwareManagementProtocolGuid`) and creates an ESRT entry for each FMP instance.

## Libraries

### FmpWrapperDeviceLib.inf

LIBRARY_CLASS: `FmpWrapperDeviceLib`

Definition: MsCapsuleUpdatePkg\Include\Library\FmpWrapperDeviceLib.h

Implementation: MsCapsuleUpdatePkg\Library\FmpWrapperDeviceLib.inf

Implements common code included by all FMP wrapper drivers.

In a scalable FMP architecture, there are one or more FMP wrapper drivers (e.g., for UEFI, ME, FW, etc.), each referring to its own FMP library instance, Device GUID, Device Name, Lowest Supported Version and update progress display color in the DSC file.

Following is an example of relationship between an FMP driver and libraries:

```c
SampleDeviceLibWrapperFMP.inf   (FMP driver which is just a wrapper)
           |
           v
FmpWrapperDeviceLib.inf         (this common-code library included by the FMP driver)
           |
           v
SampleFmpDeviceLib.inf          (FMP instance unique to this FMP driver and implements all the required hooks)
```

### DxeCapsuleLib.inf

LIBRARY_CLASS: `CapsuleLib`

Definition: MdeModulePkg\Include\Library\CapsuleLib.h

Implementation: MsCapsuleUpdatePkg\Library\DxeCapsuleLib

Implements entry points for processing capsules at boot time and runtime.

`ProcessCapsules()` is an entry point for processing FMP and Windows Firmware Update Display capsules at boot time.

`ProcessCapsuleImage()` is an entry point for processing FMP and Windows Firmware Update Display capsules at runtime.

This library implementation is similar to EDK2's implementation, but with following differences/additions: not supporting drivers embedded in the capsule and only supporting one payload per capsule.

### DisplayUpdateProgressTextLib.inf

LIBRARY_CLASS: `DisplayUpdateProgressLib`

Definition: MsCapsuleUpdatePkg\Include\Library\DisplayUpdateProgressLib.h

Implementation: MsCapsuleUpdatePkg\Library\DisplayUpdateProgressTextLib

Implements `DisplayUpdateProgress()` to display firmware update progress in text at console out.  It is called from DxeCapsuleLib.inf.

### DisplayUpdateProgressGraphicsLib.inf

LIBRARY_CLASS: `DisplayUpdateProgressLib`

Definition: MsCapsuleUpdatePkg\Include\Library\DisplayUpdateProgressLib.h

Implementation: MsCapsuleUpdatePkg\Library\DisplayUpdateProgressGraphicsLib

Implements `DisplayUpdateProgress()` to display firmware update progress in graphics (progress bar) at console out.  It is called from DxeCapsuleLib.inf.

This library requires a new protocol `gEfiBootLogoProtocol2Guid`, which is defined by this implementation in MdeModulePkg\Include\Protocol\BootLogo2.h.  MdeModulePkg\Universal\Acpi\BootGraphicsResourceTableDxe\BootGraphicsResourceTableDxe.inf driver has been changed to publish this new protocol and consequently is added to the DSC and FDF file.  MdeModulePkg\Library\BootLogoLib\BootLogoLib.inf driver has been changed to set boot logo data, which is then consumed by this library.

### CapsuleUpdatePolicyLibNull.inf

LIBRARY_CLASS: `CapsuleUpdatePolicyLib`

Definition: MsCapsuleUpdatePkg\Include\Library\CapsuleUpdatePolicyLib.h

Implementation: MsCapsuleUpdatePkg\Library\CapsuleUpdatePolicyLibNull

Implements (null) interface for checking update policies such as power, thermal and system environment while updating device firmware.  It is called from FmpWrapperDeviceLib.inf library.

### FmpPolicyDxeLib.inf

LIBRARY_CLASS: `FmpPolicyLib`

Definition: MsCapsuleUpdatePkg\Include\Library\FmpPolicyLib.h

Implementation: MsCapsuleUpdatePkg\Library\FmpPolicyLib

Implements (null) interface for FMP policies such as whether to check for Lowest Supported Version (LSV) or to lock FMP device at ReadyToBoot event.  This sample implementation will be replaced by platform specific implementation.

### BaseBmpSupportLib.inf

LIBRARY_CLASS: `BmpSupportLib`

Definition: MdeModulePkg\Include\Library\BmpSupportLib.h

Implementation: MsCapsuleUpdatePkg\Library\BaseBmpSupportLib

Converts BMP graphics image to GOP blt.  It is called from DxeCapsuleLib.inf when processing Windows Firmware Update Display capsules.

### FmpHelperDxeLib.inf

LIBRARY_CLASS: `FmpHelperLib`

Definition: MsCapsuleUpdatePkg\Include\Library\FmpHelperLib.h

Implementation: MsCapsuleUpdatePkg\Library\FmpHelperLib

Implements helper functions.

### MsFmpPayloadHeaderV1DxeLib.inf

LIBRARY_CLASS: `MsFmpPayloadHeaderLib`

Definition: MsCapsuleUpdatePkg\Include\Library\MsFmpPayloadHeaderLib.h

Implementation: MsCapsuleUpdatePkg\Library\MsFmpPayloadHeaderV1DxeLib

Implements helper functions used by FmpWrapperDeviceLib library.  It implements a (platform specific) header `MS_FMP_PAYLOAD_HEADER` above FMP payload containing version, LSV, etc.

## MsCapsuleUpdatePkg\MsCapsuleUpdatePkg.dsc & MsCapsuleUpdatePkg\MsCapsuleUpdatePkg.dec

MsCapsuleUpdatePkg.dsc links all the required libraries for `MsCapsuleUpdatePkg` and compiles EfiSystemResourceTableDxe.inf driver.

MsCapsuleUpdatePkg.dec lists of the libraries, GUIDs and PCDs used in the `MsCapsuleUpdatePkg`.

## Limitations

Following are the limitations of this implementation:
1.	Drivers embedded in the capsule are not supported/run.  The reason for not supporting it is that this design assumes closed-system architecture where these drivers would run within trusted boundaries and the firmware is unprotected.  So, any such driver approved by secure boot (or when secure boot is off), if allowed to run, can jeopardize the security of the system.
2.	Only one payload per capsule is supported.

## Feedback

**TBD**
