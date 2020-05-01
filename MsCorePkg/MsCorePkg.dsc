## @file
# MsCore Package Localized Strings and Content
#
# Copyright (C) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PLATFORM_NAME                  = MsCore
  PLATFORM_GUID                  = 25CFE603-202B-4297-A63A-00B534A8CC75
  PLATFORM_VERSION               = .10
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/MsCorePkg
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

[PcdsFeatureFlag]


[PcdsFixedAtBuild]

[LibraryClasses.common]
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  MemoryTypeInformationChangeLib|MdeModulePkg/Library/MemoryTypeInformationChangeLibNull/MemoryTypeInformationChangeLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  MuVariablePolicyHelperLib|MdeModulePkg/Library/MuVariablePolicyHelperLib/MuVariablePolicyHelperLib.inf
  MuTelemetryHelperLib|MsWheaPkg/Library/MuTelemetryHelperLib/MuTelemetryHelperLib.inf
  DeviceSpecificBusInfoLib|MsCorePkg/Library/DeviceSpecificBusInfoLibNull/DeviceSpecificBusInfoLibNull.inf

  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf

  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  GuidedSectionExtract|MsCorePkg/Core/GuidedSectionExtractPeim/GuidedSectionExtract.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf

  DeviceBootManagerLib|MsCorePkg/Library/DeviceBootManagerLibNull/DeviceBootManagerLibNull.inf
  PlatformBootManagerLib|MsCorePkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  MathLib|MsCorePkg/Library/MathLib/MathLib.inf

  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf

  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  JsonLiteParserLib|MsCorePkg/Library/JsonLiteParser/JsonLiteParser.inf
  FltUsedLib|MdePkg/Library/FltUsedLib/FltUsedLib.inf
  MuTelemetryHelperLib|MsWheaPkg/Library/MuTelemetryHelperLib/MuTelemetryHelperLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf

  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  XmlTreeLib|XmlSupportPkg/Library/XmlTreeLib/XmlTreeLib.inf
  UnitTestLib|UnitTestFrameworkPkg/Library/UnitTestLib/UnitTestLib.inf

  Tpm2CommandLib|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2DeviceLibDTpm.inf
  Tpm2DebugLib|SecurityPkg/Library/Tpm2DebugLib/Tpm2DebugLibNull.inf

[LibraryClasses.common.UEFI_APPLICATION]
  UnitTestPersistenceLib|UnitTestFrameworkPkg/Library/UnitTestPersistenceLibSimpleFileSystem/UnitTestPersistenceLibSimpleFileSystem.inf
  UnitTestResultReportLib|XmlSupportPkg/Library/UnitTestResultReportJUnitFormatLib/UnitTestResultReportLib.inf
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf

[LibraryClasses.X64]
!if $(TARGET) == DEBUG
!if $(TOOL_CHAIN_TAG) == VS2017 or $(TOOL_CHAIN_TAG) == VS2015 or $(TOOL_CHAIN_TAG) == VS2019
  #if debug is enabled provide StackCookie support lib so that we can link to /GS exports
  NULL|MdePkg/Library/BaseBinSecurityLibRng/BaseBinSecurityLibRng.inf
  BaseBinSecurityLib|MdePkg/Library/BaseBinSecurityLibRng/BaseBinSecurityLibRng.inf
!else
  # otherwise use the null version for GCC and CLANG
  BaseBinSecurityLib|MdePkg/Library/BaseBinSecurityLibNull/BaseBinSecurityLibNull.inf
!endif
!endif

[LibraryClasses.common.PEIM]
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf

[LibraryClasses.IA32]
  NULL|MdePkg/Library/VsIntrinsicLib/VsIntrinsicLib.inf

[LibraryClasses.common.DXE_CORE]
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf


###################################################################################################
#
# Components Section - list of the modules and components that will be processed by compilation
#                      tools and the EDK II tools to generate PE32/PE32+/Coff image files.
#
# Note: The EDK II DSC file is not used to specify how compiled binary images get placed
#       into firmware volume images. This section is just a list of modules to compile from
#       source into UEFI-compliant binaries.
#       It is the FDF file that contains information on combining binary files into firmware
#       volume images, whose concept is beyond UEFI and is described in PI specification.
#       Binary modules do not need to be listed in this section, as they should be
#       specified in the FDF file. For example: Shell binary (Shell_Full.efi), FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
###################################################################################################

[Components]
  MsCorePkg/Library/MathLib/MathLib.inf
  MsCorePkg/Library/MemoryTypeInformationChangeLib/MemoryTypeInformationChangeLib.inf
  MsCorePkg/Library/TpmSgNvIndexLib/TpmSgNvIndexLib.inf
  MsCorePkg/MuCryptoDxe/MuCryptoDxe.inf
  MsCorePkg/MuVarPolicyFoundationDxe/MuVarPolicyFoundationDxe.inf
  MsCorePkg/AcpiRGRT/AcpiRgrt.inf

[Components.IA32]
  MsCorePkg/Core/GuidedSectionExtractPeim/GuidedSectionExtract.inf
  MsCorePkg/Universal/StatusCodeHandler/Serial/Pei/SerialStatusCodeHandlerPei.inf
  MsCorePkg/Library/DebugPortPei/DebugPortPei.inf
  MsCorePkg/Library/PeiDebugLib/PeiDebugLib.inf
  MsCorePkg/DebugFileLoggerII/Pei/DebugFileLoggerPei.inf

[Components.X64]
  MsCorePkg/Library/DeviceBootManagerLibNull/DeviceBootManagerLibNull.inf
  MsCorePkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  MsCorePkg/Library/DxeDebugLibRouter/DxeDebugLibRouter.inf
  MsCorePkg/Library/DebugPortProtocolInstallLib/DebugPortProtocolInstallLib.inf
  MsCorePkg/Universal/StatusCodeHandler/Serial/Dxe/SerialStatusCodeHandlerDxe.inf
  MsCorePkg/Universal/StatusCodeHandler/Serial/Smm/SerialStatusCodeHandlerSmm.inf
  MsCorePkg/IncompatiblePciDevices/NoOptionRomsAllowed/NoOptionRomsAllowed.inf
  MsCorePkg/UnitTests/MathLibUnitTest/MathLibUnitTest.inf
  MsCorePkg/Library/JsonLiteParser/JsonLiteParser.inf
  MsCorePkg/UnitTests/JsonTest/JsonTestApp.inf
  MsCorePkg/Library/DeviceSpecificBusInfoLibNull/DeviceSpecificBusInfoLibNull.inf
  MsCorePkg/CheckHardwareConnected/CheckHardwareConnected.inf
  MsCorePkg/Library/PasswordStoreLibNull/PasswordStoreLibNull.inf
  MsCorePkg/DebugFileLoggerII/Dxe/DebugFileLogger.inf

[BuildOptions]
#force deprecated interfaces off
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
