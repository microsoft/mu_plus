# Capsule Update Feature

## Copyright

Copyright (c) 2016, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## About

*MsCapsuleUpdatePkg* Package is an environment that implements:
1.	Processing of (signed) UEFI capsules at boot time and runtime
2.	Scalable FMP architecture
3.	Support for FMP and System Table capsules
4.	Support for Windows Firmware Update Display capsules
5.	Firmware update progress display (text or graphical)
6.	Capsule update and FMP policies
7.	Support for multiple capsule verification certificates
8.  ESRT support
9.	Sample FMP driver and tools for generating GUIDs, keys, certificates and sample capsules (*MsSampleFmpDevicePkg*)

## Modules/Drivers

### SampleDeviceLibWrapperFMP.inf

Implementation: MsSampleFmpDevicePkg\SampleDeviceLibWrapperFmpDxeDriver

A sample FMP wrapper driver.

A device in the system may implement its own FMP wrapper driver, which installs Firmware Management Protocol (FMP) for this device.

In a scalable FMP architecture, there are one or more FMP wrapper drivers (e.g., for UEFI, ME, FW, etc.), each referring to its own FMP library instance, Device GUID, Device Name, Lowest Supported Version and update progress display color in the DSC file.
These drivers may be included in the FDF file.

### EfiSystemResourceTableDxe.inf

Implementation: MsCapsuleUpdatePkg\Universal\EsrtDxe

This driver adds EFI System Resource Table (ESRT) to the System Configuration Tables.  It prints the entire ESRT table at debug output.  This driver is added to the DSC and FDF files.

This driver implementation differs from EDK2's implementation in that it simply cosumes all FMP instances (`gEfiFirmwareManagementProtocolGuid`) and creates an ESRT entry for each FMP instance.

## Libraries

### FmpWrapperDeviceLib.inf

LIBRARY_CLASS: `FmpWrapperDeviceLib`

Definition: MsCapsuleUpdatePkg\Include\Library\FmpWrapperDeviceLib.h

Implementation: MsCapsuleUpdatePkg\Library\FmpWrapperDeviceLib.inf

Implements common code included by all FMP wrapper drivers including SampleDeviceLibWrapperFMP.inf above.

### SampleFmpDeviceLib.inf

LIBRARY_CLASS: `FmpDeviceLib`

Definition: MsCapsuleUpdatePkg\Include\Library\FmpDeviceLib.h

Implementation: MsSampleFmpDevicePkg\Library\SampleFmpDeviceLib.inf

Implements a sample FMP instance included by the SampleDeviceLibWrapperFMP.inf driver above.

This library defines a set of functions called by FmpWrapperDeviceLib.inf library, which in turn is included by FMP wrapper drivers.  Each FMP wrapper driver will include a unique FMP library instance.

Following is an example of relationship between an FMP driver and libraries:

SampleDeviceLibWrapperFMP.inf   (FMP driver which is just a wrapper)
           |
           v
FmpWrapperDeviceLib.inf         (common code for all FMP driver)
           |
           v
SampleFmpDeviceLib.inf          (FMP instance unique to this FMP driver)

### DxeCapsuleLib.inf

LIBRARY_CLASS: `CapsuleLib`

Definition: MdeModulePkg\Include\Library\CapsuleLib.h

Implementation: MsCapsuleUpdatePkg\Library\DxeCapsuleLib

Implements entry points for processing capsules at boot time and runtime.

`ProcessCapsules()` is an entry point for processing FMP and Windows Firmware Update Display capsules at boot time.

`LocateAndProcessSystemTableCapsules()` is an entry point for processing System Table capsules at boot time.

`ProcessCapsuleImage()` is an entry point for processing FMP and Windows Firmware Update Display capsules at runtime.

This library implementation is similar to EDK2's implementation, but with following differences/additions: supporting system table capsules, not supporting drivers embedded in the capsule and only supporting one payload per capsule.

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

### CapsuleKeyBaseLib.inf

LIBRARY_CLASS: `CapsuleKeyLib`

Definition: MsCapsuleUpdatePkg\Include\Library\CapsuleKeyLib.h

Implementation: MsCapsuleUpdatePkg\Library\CapsuleKeyBaseLib

Contains public key certificates used for authenticating development and/or production capsules.

Certs.h must be replaced by the one generated by MsSampleFmpDevicePkg\Tools\MakeCerts.bat and then MsSampleFmpDevicePkg\Tools\ConvertCerToH.py.

### BaseBmpSupportLib.inf

LIBRARY_CLASS: `BmpSupportLib`

Definition: MdeModulePkg\Include\Library\BmpSupportLib.h

Implementation: MsCapsuleUpdatePkg\Library\BaseBmpSupportLib

Converts BMP graphics image to GOP blt.  It is called from DxeCapsuleLib.inf when processing Windows Firmware Update Display capsules.

### SafeBaseLib.inf

LIBRARY_CLASS: `MsSafeBaseLib`

Definition: MsCapsuleUpdatePkg\Include\Library\MsSafeBaseLib.h

Implementation: MsCapsuleUpdatePkg\Library\BaseMsSafeBaseLib

Implements helper functions used by BaseBmpSupportLib.inf.

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

## Nt32Pkg\Nt32PkgMsCapsule.dsc, Nt32Pkg/Nt32PkgMsCapsule.fdf, MsCapsuleUpdatePkg\MsCapsuleUpdatePkg.dsc, MsCapsuleUpdatePkg\MsCapsuleUpdatePkg.dec, MsSampleFmpDevicePkg\MsSampleFmpDevicePkg.dsc & MsSampleFmpDevicePkg\MsSampleFmpDevicePkg.dec

Nt32PkgMsCapsule.dsc points to Nt32Pkg\Nt32PkgMsCapsule.fdf, links all the required libraries for `MsCapsuleUpdatePkg`, compiles SampleDeviceLibWrapperFMP.inf, EfiSystemResourceTableDxe.inf and BootGraphicsResourceTableDxe.inf drivers and sets `gEfiMdeModulePkgTokenSpaceGuid.PcdTestKeyUsed` accordingly.  Each FMP wrapper driver including SampleDeviceLibWrapperFMP.inf will refer to its own FMP library instance, Device GUID, Device Name, Lowest Supported Version, update progress display color and whether system reset is required after the device update (an example shown below).

Nt32PkgMsCapsule.fdf include SampleDeviceLibWrapperFMP.inf, EfiSystemResourceTableDxe.inf and BootGraphicsResourceTableDxe.inf drivers in the final firmware image.

MsCapsuleUpdatePkg.dsc links all the required libraries for `MsCapsuleUpdatePkg` and compiles EfiSystemResourceTableDxe.inf driver.

MsCapsuleUpdatePkg.dec lists of the libraries, GUIDs and PCDs used in the `MsCapsuleUpdatePkg`.

MsSampleFmpDevicePkg.dsc links all the required libraries for `MsSampleFmpDevicePkg` and `MsCapsuleUpdatePkg` and compiles SampleDeviceLibWrapperFMP.inf driver.  Each FMP wrapper driver including SampleDeviceLibWrapperFMP.inf will refer to its own FMP library instance, Device GUID, Device Name, Lowest Supported Version, update progress display color and whether system reset is required after the device update (an example shown below).

MsSampleFmpDevicePkg.dec lists of the libraries, GUIDs and PCDs used in the `MsSampleFmpDevicePkg`.

An example of an FMP wrapper driver including SampleDeviceLibWrapperFMP.inf in the DSC file:

```c
MsSampleFmpDevicePkg/SampleDeviceLibWrapperFmpDxeDriver/SampleDeviceLibWrapperFMP.inf {
<LibraryClasses>
  FmpDeviceLib|MsSampleFmpDevicePkg/Library/SampleFmpDeviceLib/SampleFmpDeviceLib.inf
<PcdsFixedAtBuild>
  gMsCapsuleUpdatePkgTokenSpaceGuid.PcdDeviceLibWrapperDeviceImageName|L"Sample Device FMP"
  #set Lowest supported version
  gMsCapsuleUpdatePkgTokenSpaceGuid.PcdBuildTimeLowestSupportedVersion|0x0 #0.0.0
  #set to White (RGB) (255, 255, 255)
  gMsCapsuleUpdatePkgTokenSpaceGuid.PcdProgressColor|0xFFFFFFFF
  #Take note - GUIDs/UUIDs have different formatting.  To get them all align can be challenging.
  #When doing byte array the last 8 bytes are MSB while the previous bytes are in LSB
  #UUID HEX FMT:   11223344-5566-7788-99AA-BBCCDDEEFF00   for EV2
  #gMsCapsuleUpdatePkgTokenSpaceGuid.PcdDeviceLibWrapperDeviceGuid|{0x44, 0x33, 0x22, 0x11, 0x66, 0x55, 0x88, 0x77, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00}
  #Populate GUID (from MsSampleFmpDevicePkg\Tools\Guid.txt) below as per the example above
  #UUID HEX FMT:   ########-####-####-####-############   for >=EV2.5
  gMsCapsuleUpdatePkgTokenSpaceGuid.PcdDeviceLibWrapperDeviceGuid|{0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##}
<PcdsFeatureFlag>
  gMsCapsuleUpdatePkgTokenSpaceGuid.PcdDeviceLibWrapperSystemResetRequired|TRUE
}
```

## Steps to Build Successfully

Note: all sample tools provided in MsSampleFmpDevicePkg\Tools should not be used in production environments.

1.	Change directory to MsSampleFmpDevicePkg\Tools
2.	Run MakeCert.bat to create private keys (*.pfx) and public key certificates (*.cer)
3.	Run ConvertCerToH.py to create Certs.h
4.	Copy Certs.h to MsCapsuleUpdatePkg\Library\CapsuleKeyBaseLib
5.	Run GenerateGuid.py to generate a GUID in Guid.txt
6.	Replace #s in Nt32Pkg\Nt32PkgMsCapsule.dsc and MsSampleFmpDevicePkg\MsSampleFmpDevicePkg.dsc by GUID in Guid.txt
7.	Run CapsuleGeneratorDevelopment.bat to create sample development capsule (*.cap)
8.	Run CapsuleGeneratorProduction.bat to create sample production capsule (*.cap)
9.	Configure environment variables as necessary
10.	Build with one of the DSC files above (example: build -p Nt32PkgMsCapsule.dsc)

## Loading and Updating Capsules from UEFI Shell

After booting nt32 platform (Nt32PkgMsCapsule.dsc) to UEFI shell, the capsules generated above can be loaded and updated in UEFI shell by MsSampleFmpDevicePkg\Tools\XXX\FMPTestApp.efi.

Example: “fs0:>FMPTestApp.efi -c 3rdPartyProductionCapsule.cap”

In order for these capsules to work successfully in nt32 platform, the capsules generated above do not set `CAPSULE_FLAGS_PERSIST_ACROSS_RESET` or `CAPSULE_FLAGS_INITIATE_RESET` flags and thus do not require warm reset.  For production systems, these flags may need to be set (see the capsule generating batch files for more info).

When using `DisplayUpdateProgressGraphicsLib` library in the nt32 platform, firmware update progress bar is displayed successfully only when the system is booted to UEFI shell without going into the Setup or Frontpage.  This should not be a limitation in a non-nt32 platform.

## Creating Windows Capsules

Windows capsules can be created by running MsSampleFmpDevicePkg\Tools\CreateWindowsCapsule.py tool (which uses CreateWindowsInf.py).  Windows capsule files (*.inf, *.cat and *.cap/*.bin) are created in MsSampleFmpDevicePkg\Tools\WindowsCapsule folder.

## Limitations

Following are the limitations of this implementation:
1.	Drivers embedded in the capsule are not supported/run.  The reason for not supporting it is that this design assumes closed-system architecture where these drivers would run within trusted boundaries and the firmware is unprotected.  So, any such driver approved by secure boot (or when secure boot is off), if allowed to run, can jeopardize the security of the system.
2.	Only one payload per capsule is supported.
3.	When using nt32 platform, the capsules cannot be tested on the next reboot (see above for more info).

## Feedback

**TBD**
