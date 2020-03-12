
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

[LibraryClasses]
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf

  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf

  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf

  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/BaseMemoryAllocationLibNull/BaseMemoryAllocationLibNull.inf
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
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  BaseCryptLib|SharedCryptoPkg/Library/CryptLibSharedDriver/PeiCryptLibSharedDriver.inf

[LibraryClasses.common.DXE_CORE, LibraryClasses.common.UEFI_APPLICATION, LibraryClasses.common.DXE_DRIVER]
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  BaseCryptLib|SharedCryptoPkg/Library/CryptLibSharedDriver/DxeCryptLibSharedDriver.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  BaseCryptLib|SharedCryptoPkg/Library/CryptLibSharedDriver/SmmCryptLibSharedDriver.inf
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf

[LibraryClasses.IA32, LibraryClasses.X64]
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[LibraryClasses.ARM.DXE_DRIVER, LibraryClasses.AARCH64.DXE_DRIVER, LibraryClasses.ARM.UEFI_APPLICATION, LibraryClasses.AARCH64.UEFI_APPLICATION]
  RngLib|SecurityPkg/RandomNumberGenerator/RngDxeLib/RngDxeLib.inf

[LibraryClasses.IA32]
  NULL|MdePkg/Library/VsIntrinsicLib/VsIntrinsicLib.inf

[LibraryClasses.X64]
  NULL|MdePkg/Library/BaseBinSecurityLibRng/BaseBinSecurityLibRng.inf
  BaseBinSecurityLib|MdePkg/Library/BaseBinSecurityLibRng/BaseBinSecurityLibRng.inf

[LibraryClasses.X64.UEFI_APPLICATION]
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf

[Components]
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

[Components.IA32, Components.X64]
  SharedCryptoPkg/Driver/SharedCryptoPeiShaOnly.inf
  SharedCryptoPkg/Driver/SharedCryptoPeiShaRsa.inf

[Components.X64.DXE_SMM_DRIVER, Components.AARCH64.DXE_SMM_DRIVER, Components.IA32.DXE_SMM_DRIVER]

  SharedCryptoPkg/Driver/SharedCryptoDxe.inf
  SharedCryptoPkg/Driver/SharedCryptoDxeMu.inf
  SharedCryptoPkg/Driver/SharedCryptoDxeShaOnly.inf

[Components.X64.DXE_SMM_DRIVER]
  SharedCryptoPkg/Driver/SharedCryptoSmm.inf
  SharedCryptoPkg/Driver/SharedCryptoSmmMu.inf

[BuildOptions.X64]
  MSFT:*_*_*_CC_FLAGS  = /FAcs /X /GS
