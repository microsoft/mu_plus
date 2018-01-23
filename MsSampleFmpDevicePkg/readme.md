# Sample Drivers, Libraries and Tools for the Capsule Update Feature

## Copyright

Copyright (c) 2017, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## About

**MsSampleFmpDevicePkg** Package is an environment that provides:

1. Sample FMP wrapper driver
2. Sample FMP library instance
3. Tools for generating GUIDs, keys, certificates and sample capsules
4. Dummy capsule payload and tools for loading capsules from UEFI shell
5. Sample nt32 DSC and FDF files for the build containing capsule update feature

Note: this package provides tools and sample FMP driver and library.  No actual device firmware is updated or attempted to be updated by this implementation.

## Modules/Drivers

### SampleDeviceLibWrapperFMP.inf

Implementation: MsSampleFmpDevicePkg\SampleDeviceLibWrapperFmpDxeDriver

A sample FMP wrapper driver.

A device in the system may implement its own FMP wrapper driver, which installs Firmware Management Protocol (FMP) for this device.

In a scalable FMP architecture, there are one or more FMP wrapper drivers (e.g., for UEFI, ME, FW, etc.), each referring to its own FMP library instance, Device GUID, Device Name, Lowest Supported Version and update progress display color in the DSC file.
These drivers may be included in the FDF file.

## Libraries

### SampleFmpDeviceLib.inf

LIBRARY_CLASS: `FmpDeviceLib`

Definition: MsCapsuleUpdatePkg\Include\Library\FmpDeviceLib.h

Implementation: MsSampleFmpDevicePkg\Library\SampleFmpDeviceLib.inf

Implements a sample FMP instance included by the SampleDeviceLibWrapperFMP.inf driver above.

This library defines a set of functions called by MsCapsuleUpdatePkg's FmpWrapperDeviceLib.inf library, which in turn is included by FMP wrapper drivers.  Each FMP wrapper driver will include a unique FMP library instance.

Following is an example of relationship between an FMP driver and libraries:

```c
SampleDeviceLibWrapperFMP.inf   (FMP driver which is just a wrapper)
           |
           v
FmpWrapperDeviceLib.inf         (MsCapsuleUpdatePkg's common-code library included by the FMP driver)
           |
           v
SampleFmpDeviceLib.inf          (FMP instance unique to this FMP driver and implements all the required hooks)
```

### CapsuleKeyBaseLib.inf

LIBRARY_CLASS: `CapsuleKeyLib`

Definition: MsCapsuleUpdatePkg\Include\Library\CapsuleKeyLib.h

Implementation: MsSampleFmpDevicePkg\Library\CapsuleKeyBaseLib

Contains public key certificates used for authenticating development and/or production capsules.

## Nt32Pkg\Nt32PkgMsCapsule.dsc, Nt32Pkg/Nt32PkgMsCapsule.fdf, MsSampleFmpDevicePkg\MsSampleFmpDevicePkg.dsc & MsSampleFmpDevicePkg\MsSampleFmpDevicePkg.dec

Nt32PkgMsCapsule.dsc points to Nt32Pkg\Nt32PkgMsCapsule.fdf, links all the required libraries for `MsCapsuleUpdatePkg`, compiles SampleDeviceLibWrapperFMP.inf, EfiSystemResourceTableDxe.inf and BootGraphicsResourceTableDxe.inf drivers and sets `gEfiMdeModulePkgTokenSpaceGuid.PcdTestKeyUsed` accordingly.  Each FMP wrapper driver including SampleDeviceLibWrapperFMP.inf will refer to its own FMP library instance, Device GUID, Device Name, Lowest Supported Version, update progress display color and whether system reset is required after the device update (an example shown below).

Nt32PkgMsCapsule.fdf include SampleDeviceLibWrapperFMP.inf, EfiSystemResourceTableDxe.inf and BootGraphicsResourceTableDxe.inf drivers in the final firmware image.

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
4.	Copy Certs.h to MsSampleFmpDevicePkg\Library\CapsuleKeyBaseLib
5.	Run GenerateGuid.py to generate a GUID in Guid.txt
6.	Replace #s in Nt32Pkg\Nt32PkgMsCapsule.dsc and MsSampleFmpDevicePkg\MsSampleFmpDevicePkg.dsc by GUID in Guid.txt
7.	Run CapsuleGeneratorDevelopment.bat to create sample development capsule (*.cap)
8.	Run CapsuleGeneratorProduction.bat to create sample production capsule (*.cap)
9.	Configure environment variables as necessary
10.	Build with nt32 DSC file above (example: build -p Nt32PkgMsCapsule.dsc)

## Loading and Updating Capsules from UEFI Shell

After booting nt32 platform (Nt32PkgMsCapsule.dsc) to UEFI shell, the capsules generated above can be loaded and updated in UEFI shell by MsSampleFmpDevicePkg\Tools\IA32\FMPTestApp.efi.

Example: “fs0:>FMPTestApp.efi -l -c 3rdPartyProductionCapsule.cap”

Option -l above is required when using MsCapsuleUpdatePkg's `DisplayUpdateProgressGraphicsLib` library in the nt32 platform.  This option displays the logo first and then displays the update progress bar relative to that logo. 

In order for these capsules to work successfully in nt32 platform, the capsules generated above do not set `CAPSULE_FLAGS_PERSIST_ACROSS_RESET` or `CAPSULE_FLAGS_INITIATE_RESET` flags and thus do not require warm reset.  For production systems, these flags may need to be set (see the capsule generating batch files for more info).

## Creating Windows Capsules

Windows capsules can be created by running MsSampleFmpDevicePkg\Tools\CreateWindowsCapsule.py tool (which uses CreateWindowsInf.py).  Windows capsule files (*.inf, *.cat and *.cap/*.bin) are created in MsSampleFmpDevicePkg\Tools\WindowsCapsule folder.

## Limitations

Following are the limitations of this implementation:
1.	When using nt32 platform, the capsules cannot be tested on the next reboot (see above for more info).

## Feedback

**TBD**

