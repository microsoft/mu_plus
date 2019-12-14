
## @file
# SharedCrypto Driver Build DSC
#
# This DSC is only for building the shared crypto binary
#
# Copyright (C) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##


[Defines]
  PLATFORM_NAME                  = SharedCrypto
  PLATFORM_GUID                  = E6A82DBC-2F61-4BCD-8D60-720C7610B741
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

  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf

  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf

  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf

  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  SharedCryptoLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  BaseBinSecurityLibRng|MdePkg/Library/BaseBinSecurityLibNull/BaseBinSecurityLibNull.inf

  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf

[LibraryClasses.common.PEIM]
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/SecPeiDebugAgentLib.inf
  DebugLib|MsCorePkg/Library/PeiDebugLib/PeiDebugLib.inf
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibOptPei/BaseMemoryLibOptPei.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf

[LibraryClasses.common.DXE_CORE, LibraryClasses.common.UEFI_DRIVER]
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibOptDxe/BaseMemoryLibOptDxe.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf

[LibraryClasses.X64, LibraryClasses.IA32]
  NULL|MdePkg/Library/BaseBinSecurityLibRng/BaseBinSecurityLibRng.inf
  BaseBinSecurityLib|MdePkg/Library/BaseBinSecurityLibRng/BaseBinSecurityLibRng.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibOptDxe/BaseMemoryLibOptDxe.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DebugLib|MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf

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

  UnitTestLib|MsUnitTestPkg/Library/UnitTestLib/UnitTestLib.inf
  UnitTestLogLib|MsUnitTestPkg/Library/UnitTestLogLib/UnitTestLogLib.inf
  UnitTestAssertLib|MsUnitTestPkg/Library/UnitTestAssertLib/UnitTestAssertLib.inf
  UnitTestPersistenceLib|MsUnitTestPkg/Library/UnitTestPersistenceFileSystemLib/UnitTestPersistenceFileSystemLib.inf
  UnitTestBootUsbLib|MsUnitTestPkg/Library/UnitTestBootUsbClassLib/UnitTestBootUsbClassLib.inf
  UnitTestResultReportLib|MsUnitTestPkg/Library/UnitTestResultReportPlainTextOutputLib/UnitTestResultReportLib.inf


!if $(TARGET) == DEBUG
[LibraryClasses]
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
[LibraryClasses.common.DXE_CORE]
  DebugLib|MdePkg/Library/UefiDebugLibDebugPortProtocol/UefiDebugLibDebugPortProtocol.inf
[LibraryClasses.common.PEIM]
  DebugLib|MsCorePkg/Library/PeiDebugLib/PeiDebugLib.inf

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x3F
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x7
  gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel|0x800802C6
!endif

###################################
# Components to build
###################################

[Components.IA32, Components.X64]
  SharedCryptoPkg/Driver/SharedCryptoPeiShaOnly.inf
  SharedCryptoPkg/Driver/SharedCryptoPeiShaRsa.inf

[Components.X64, Components.AARCH64, Components.IA32]
  SharedCryptoPkg/Driver/SharedCryptoDxe.inf
  SharedCryptoPkg/Driver/SharedCryptoDxeShaOnly.inf
  SharedCryptoPkg/Driver/SharedCryptoDxeMu.inf

[Components.X64.DXE_SMM_DRIVER]
  SharedCryptoPkg/Driver/SharedCryptoSmm.inf
  SharedCryptoPkg/Driver/SharedCryptoSmmMu.inf

[BuildOptions.X64.DXE_SMM_DRIVER]
  # MSFT:*_*_*_CC_FLAGS  = /FAcs /X /GS

[BuildOptions.common.EDKII.DXE_RUNTIME_DRIVER, BuildOptions.common.EDKII.DXE_SMM_DRIVER, BuildOptions.common.EDKII.SMM_CORE, BuildOptions.common.EDKII.DXE_DRIVER]
  MSFT:*_*_*_DLINK_FLAGS = /ALIGN:4096 # enable 4k alignment for MAT and other protections.
