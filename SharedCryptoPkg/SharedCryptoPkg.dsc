
## @file
# SharedCrypto Library and driver CI Build DSC
#
# This DSC is only for CI builds
#
# Copyright (C) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##


[Defines]
  PLATFORM_NAME                  = SharedCrypto
  PLATFORM_GUID                  = A8692B37-52B7-4188-B75E-360E32D1EFB4
  PLATFORM_VERSION               = .10
  DSC_SPECIFICATION              = 0x0001001A
  OUTPUT_DIRECTORY               = Build/SharedCryptoPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64|AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

!ifndef $(IS_CI)
  DEFINE IS_CI = TRUE
!endif

[LibraryClasses]
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
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf

  # unit test dependencies
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  MemoryTypeInformationChangeLib|MdeModulePkg/Library/MemoryTypeInformationChangeLibNull/MemoryTypeInformationChangeLibNull.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf


[LibraryClasses.common.PEIM]
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/SecPeiDebugAgentLib.inf
  DebugLib|MsCorePkg/Library/PeiDebugLib/PeiDebugLib.inf
  BaseCryptLib|SharedCryptoPkg/Library/CryptLibSharedDriver/PeiCryptLibSharedDriver.inf

[LibraryClasses.common.DXE_CORE]
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibOptDxe/BaseMemoryLibOptDxe.inf
  BaseCryptLib|SharedCryptoPkg/Library/CryptLibSharedDriver/DxeCryptLibSharedDriver.inf
  CryptLibSharedDriver|SharedCryptoPkg/Library/CryptLibSharedDriver/DxeCryptLibSharedDriver.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  BaseCryptLib|SharedCryptoPkg/Library/CryptLibSharedDriver/SmmCryptLibSharedDriver.inf
  CryptLibSharedDriver|SharedCryptoPkg/Library/CryptLibSharedDriver/SmmCryptLibSharedDriver.inf


[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.DXE_RUNTIME_DRIVER,LibraryClasses.common.DXE_SMM_DRIVER]
  BaseBinSecurityLibRng|MdePkg/Library/BaseBinSecurityLibRng/BaseBinSecurityLibRng.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibOptDxe/BaseMemoryLibOptDxe.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DebugLib|MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf


[LibraryClasses.IA32]
 NULL|MdePkg/Library/VsIntrinsicLib/VsIntrinsicLib.inf

# build the test app if needed
[LibraryClasses.X64]
  NULL|MdePkg/Library/BaseBinSecurityLibRng/BaseBinSecurityLibRng.inf
  BaseBinSecurityLib|MdePkg/Library/BaseBinSecurityLibRng/BaseBinSecurityLibRng.inf

[LibraryClasses.X64.UEFI_APPLICATION]
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf

  UnitTestLib|MsUnitTestPkg/Library/UnitTestLib/UnitTestLib.inf
  UnitTestLogLib|MsUnitTestPkg/Library/UnitTestLogLib/UnitTestLogLib.inf
  UnitTestAssertLib|MsUnitTestPkg/Library/UnitTestAssertLib/UnitTestAssertLib.inf
  UnitTestPersistenceLib|MsUnitTestPkg/Library/UnitTestPersistenceFileSystemLib/UnitTestPersistenceFileSystemLib.inf
  UnitTestBootUsbLib|MsUnitTestPkg/Library/UnitTestBootUsbClassLib/UnitTestBootUsbClassLib.inf
  UnitTestResultReportLib|MsUnitTestPkg/Library/UnitTestResultReportPlainTextOutputLib/UnitTestResultReportLib.inf



[Components.X64, Components.AARCH64, Components.ARM, Components.AARCH64]
  #SharedCryptoPkg/UnitTests/PkcsUnitTestApp/PkcsUnitTestApp.inf
  #SharedCryptoPkg/UnitTests/HmacUnitTestApp/UnitTestApp.inf
  #SharedCryptoPkg/UnitTests/RandomUnitTestApp/UnitTestApp.inf
  #SharedCryptoPkg/UnitTests/ShaUnitTestApp/UnitTestApp.inf
  #SharedCryptoPkg/UnitTests/RsaUnitTestApp/UnitTestApp.inf
  #SharedCryptoPkg/UnitTests/X509UnitTestApp/UnitTestApp.inf
  # make sure we can build our own images
  SharedCryptoPkg/Library/CryptLibSharedDriver/DxeCryptLibSharedDriver.inf
  SharedCryptoPkg/Library/CryptLibSharedDriver/PeiCryptLibSharedDriver.inf
  SharedCryptoPkg/Library/CryptLibSharedDriver/SmmCryptLibSharedDriver.inf

[Components.IA32, Components.ARM, Components.X64, Components.AARCH64]
  SharedCryptoPkg/Driver/SharedCryptoPeiShaOnly.inf {
    <LibraryClasses>
!if $(IS_CI)  == TRUE
      BaseCryptLib|CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
!else
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
!endif
      RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  }

[Components.X64, Components.AARCH64, Components.IA32]

  SharedCryptoPkg/Driver/SharedCryptoDxe.inf {
    <LibraryClasses>
!if $(IS_CI)  == TRUE
      BaseCryptLib|CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
!else
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
!endif

      RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  }

  SharedCryptoPkg/Driver/SharedCryptoDxeMu.inf {
    <LibraryClasses>
!if $(IS_CI)  == TRUE
      BaseCryptLib|CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
!else
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
!endif
      RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  }

[Components.X64.DXE_SMM_DRIVER]
  SharedCryptoPkg/Driver/SharedCryptoSmm.inf {
    <LibraryClasses>
!if $(IS_CI)  == TRUE
      BaseCryptLib|CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
!else
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
!endif
      RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
      IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  }

  SharedCryptoPkg/Driver/SharedCryptoSmmMu.inf {
    <LibraryClasses>
!if $(IS_CI)  == TRUE
      BaseCryptLib|CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
!else
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
!endif
      RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
      IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  }


[BuildOptions.X64]
  MSFT:*_*_*_CC_FLAGS  = /FAcs /X /GS
