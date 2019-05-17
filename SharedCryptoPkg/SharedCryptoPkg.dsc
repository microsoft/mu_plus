
## @file
# SharedCrypto Library and driver CI Build DSC
#
# This DSC is only for CI builds
#
# Copyright (c) 2019, Microsoft Corporation
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf

  # unit test dependencies
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
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
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

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

[Components.IA32, Components.ARM, Components.X64, Components.AARCH64]
  SharedCryptoPkg/Driver/SharedCryptoPeiShaOnly.inf {
    <LibraryClasses>
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
      RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  }

[Components.X64, Components.AARCH64, Components.IA32]
  SharedCryptoPkg/Driver/SharedCryptoDxe.inf {
    <LibraryClasses>
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf

      RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  }

  SharedCryptoPkg/Driver/SharedCryptoDxeMu.inf {
    <LibraryClasses>
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
      RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  }

[Components.X64.DXE_SMM_DRIVER]
  SharedCryptoPkg/Driver/SharedCryptoSmm.inf {
    <LibraryClasses>
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
      RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
      IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  }

  SharedCryptoPkg/Driver/SharedCryptoSmmMu.inf {
    <LibraryClasses>
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
      RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
      IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  }


[BuildOptions.X64]
  MSFT:*_*_*_CC_FLAGS  = /FAcs /X /GS
