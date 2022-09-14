# MS Core Package

## About

This package has shared drivers and libraries that are silicon and platform independent.

## Modules

| Modules                       | Link to Documentation |
| ---                           | --- |
| **AcpiRGRT:**                 | [Regulatory Graphics Resource Table](../AcpiRGRT/feature_acpi_rgrt.md) |
| **CheckHardwareConnected:**   | [Require Devices Connected](../CheckHardwareConnected/readme.md) |
| **DebugFileLoggerII:**        | [Uefi Log to File](../DebugFileLoggerII/README.md) |
| **GuidedSectionExtractPeim:** | [Pei version without decompression](../Core/GuidedSectionExtractPeim/ReadMe.md) |
| **IncompatiblePciDevices**    | [Incompatible Pci Devices](../IncompatiblePciDevices/ReadMe.md) |
| **No Option Roms Allowed**    | [NoOptionRomsAllowed](../IncompatiblePciDevices/NoOptionRomsAllowed/ReadMe.md) |
| **MuCryptoDxe**               | [MuCryptoDxe](../MuCryptoDxe/Readme.md) |
| **MuVarPolicyFoundationDxe**  | [MuVarPolicyFoundationDxe](../MuVarPolicyFoundationDxe/Feature_MuVarPolicyFoundationDxe_Readme.md) |

## Libraries

| Libraries                                 | Link to Documentation |
| ---                                       | --- |
| **DebugPortPei**                          | [DebugPortPei](../Library/DebugPortPei/ReadMe.md) |
| **DebugPortProtocolInstallLib**           | [DebugPortProtocolInstallLib](../Library/DebugPortProtocolInstallLib/ReadMe.md) |
| **DeviceBootManagerLibNull**              | [DeviceBootManagerLibNull](../Library/DeviceBootManagerLibNull/ReadMe.md) |
| **DeviceSpecificBusInfoLibNull**          | [DeviceSpecificBusInfoLibNull](../Library/DeviceSpecificBusInfoLibNull/ReadMe.md) |
| **DxeDebugLibRouter**                     | [DxeDebugLibRouter](../Library/DxeDebugLibRouter/ReadMe.md) |
| **JsonLiteParser**                        | [JsonLiteParser](../Library/JsonLiteParser/ReadMe.md) |
| **MathLib**                               | [MathLib](../MathLib/Library/ReadMe.md) |
| **MemoryTypeInformationChangeLib**        | [MemoryTypeInformationChangeLib](../Library/MemoryTypeInformationChangeLib/ReadMe.md) |
| **PasswordStoreLibNull**                  | [PasswordStoreLibNull](../Library/PasswordStoreLibNull/ReadMe.md) |
| **PeiDebugLib**                           | [PeiDebugLib](../Library/PeiDebugLib/ReadMe.md) |
| **PlatformBootManagerLib**                | [PlatformBootManagerLib](../Library/PlatformBootManagerLib/ReadMe.md) |
| **TpmSgNvIndexLib**                       | [TpmSgNvIndexLib](../Library/TpmSgNvIndexLib/ReadMe.md) |
| **ExceptionPersistenceLib**               | [ExceptionPersistenceLib](../Library/ExceptionPersistenceLibCmos/Readme.md) |
| **MemoryProtectionExceptionHandlerLib**   | [MemoryProtectionExceptionHandlerLib](../Library/MemoryProtectionExceptionHandlerLib/Readme.md) |

## Testing

There are UEFI shell application based unit tests for some libraries.
These tests attempt to verify basic functionality of public interfaces.
Check the **UntTests** folder at the root of the package.

| Library Tests          | Link to Documentation |
| ---                    | --- |
| **Json Test:**         | [Json Test](..\UnitTests/JsonTest/ReadMe.md) |
| **MathLib UnitTests:** | [MathLib UnitTests](..\UnitTests/MathLibUnitTest/ReadMe.md) |

---

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
