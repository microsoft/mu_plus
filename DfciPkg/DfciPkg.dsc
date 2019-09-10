## @file
# DfciPkg Package Localized Strings and Content
#
# All of the elements of the implementation of Dfci are documented here.
#
# Copyright (C) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PLATFORM_NAME                  = DfciPkg
  PLATFORM_GUID                  = 2df0c0cb-ce91-4605-bf3a-f7aaa26e8fed
  PLATFORM_VERSION               = .10
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/DfciPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

[PcdsFeatureFlag]


[PcdsFixedAtBuild]

[LibraryClasses.common]
  XmlTreeLib|XmlSupportPkg/Library/XmlTreeLib/XmlTreeLib.inf
  XmlTreeQueryLib|XmlSupportPkg/Library/XmlTreeQueryLib/XmlTreeQueryLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  HttpLib|NetworkPkg/Library/DxeHttpLib/DxeHttpLib.inf
  NetLib|NetworkPkg/Library/DxeNetLib/DxeNetLib.inf

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

  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf

  DfciXmlSettingSchemaSupportLib|DfciPkg/Library/DfciXmlSettingSchemaSupportLib/DfciXmlSettingSchemaSupportLib.inf
  DfciXmlPermissionSchemaSupportLib|DfciPkg/Library/DfciXmlPermissionSchemaSupportLib/DfciXmlPermissionSchemaSupportLib.inf
  DfciXmlDeviceIdSchemaSupportLib|DfciPkg/Library/DfciXmlDeviceIdSchemaSupportLib/DfciXmlDeviceIdSchemaSupportLib.inf
  DfciXmlIdentitySchemaSupportLib|DfciPkg/Library/DfciXmlIdentitySchemaSupportLib/DfciXmlIdentitySchemaSupportLib.inf
  DfciDeviceIdSupportLib|DfciPkg/Library/DfciDeviceIdSupportLibNull/DfciDeviceIdSupportLibNull.inf
  DfciRecoveryLib|DfciPkg/Library/DfciRecoveryLib/DfciRecoveryLib.inf
  DfciUiSupportLib|DfciPkg/Library/DfciUiSupportLibNull/DfciUiSupportLibNull.inf
  DfciV1SupportLib|DfciPkg/Library/DfciV1SupportLibNull/DfciV1SupportLibNull.inf
  DfciSettingsLib|DfciPkg/Library/DfciSettingsLib/DfciSettingsLib.inf
  DfciVirtualizationSettingsLib|DfciPkg/Library/DfciVirtualizationSettings/DfciVirtualizationSettings.inf
  DfciGroupLib|DfciPkg/Library/DfciGroupLibNull/DfciGroups.inf
  ZeroTouchSettingsLib|ZeroTouchPkg/Library/ZeroTouchSettings/ZeroTouchSettings.inf
  JsonLiteParserLib|MsCorePkg/Library/JsonLiteParser/JsonLiteParser.inf

  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[LibraryClasses.X64]
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  PasswordStoreLib|MsCorePkg/Library/PasswordStoreLibNull/PasswordStoreLibNull.inf

#!if $(TARGET) == DEBUG
  #if debug is enabled provide StackCookie support lib so that we can link to /GS exports
  NULL|MdePkg/Library/BaseBinSecurityLibRng/BaseBinSecurityLibRng.inf
  BaseBinSecurityLib|MdePkg/Library/BaseBinSecurityLibRng/BaseBinSecurityLibRng.inf
#!else
#  BaseBinSecurityLib|MdePkg/Library/BaseBinSecurityLibNull/BaseBinSecurityLibNull.inf
#!endif

[LibraryClasses.X64.UEFI_APPLICATION]
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf

  UnitTestLib|MsUnitTestPkg/Library/UnitTestLib/UnitTestLib.inf
  UnitTestLogLib|MsUnitTestPkg/Library/UnitTestLogLib/UnitTestLogLib.inf
  UnitTestAssertLib|MsUnitTestPkg/Library/UnitTestAssertLib/UnitTestAssertLib.inf
  UnitTestPersistenceLib|MsUnitTestPkg/Library/UnitTestPersistenceFileSystemLib/UnitTestPersistenceFileSystemLib.inf
  UnitTestBootUsbLib|MsUnitTestPkg/Library/UnitTestBootUsbClassLib/UnitTestBootUsbClassLib.inf
  UnitTestResultReportLib|XmlSupportPkg/Library/UnitTestResultReportJUnitFormatLib/UnitTestResultReportLib.inf

[LibraryClasses.common.PEIM]
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf

[LibraryClasses.common.DXE_CORE]
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf


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


[Components.X64]
  DfciPkg/Library/DfciDeviceIdSupportLibNull/DfciDeviceIdSupportLibNull.inf
  DfciPkg/Library/DfciRecoveryLib/DfciRecoveryLib.inf
  DfciPkg/Library/DfciPasswordProvider/DfciPasswordProvider.inf
  DfciPkg/Library/DfciSettingPermissionLib/DfciSettingPermissionLib.inf
  DfciPkg/Library/DfciSettingsLib/DfciSettingsLib.inf
  DfciPkg/Library/DfciUiSupportLibNull/DfciUiSupportLibNull.inf
  DfciPkg/Library/DfciV1SupportLibNull/DfciV1SupportLibNull.inf
  DfciPkg/Library/DfciXmlPermissionSchemaSupportLib/DfciXmlPermissionSchemaSupportLib.inf
  DfciPkg/Library/DfciXmlSettingSchemaSupportLib/DfciXmlSettingSchemaSupportLib.inf
  DfciPkg/Library/DfciXmlDeviceIdSchemaSupportLib/DfciXmlDeviceIdSchemaSupportLib.inf
  DfciPkg/Library/DfciXmlIdentitySchemaSupportLib/DfciXmlIdentitySchemaSupportLib.inf
  DfciPkg/Library/DfciVirtualizationSettings/DfciVirtualizationSettings.inf
  DfciPkg/Library/DfciGroupLibNull/DfciGroups.inf
  DfciPkg/SettingsManager/SettingsManagerDxe.inf {
        #Platform should add all it settings libs here
  <LibraryClasses>
        NULL|DfciPkg/Library/DfciPasswordProvider/DfciPasswordProvider.inf
        NULL|DfciPkg/Library/DfciSettingsLib/DfciSettingsLib.inf
        NULL|DfciPkg/Library/DfciVirtualizationSettings/DfciVirtualizationSettings.inf
        DfciSettingPermissionLib|DfciPkg/Library/DfciSettingPermissionLib/DfciSettingPermissionLib.inf
  <PcdsFeatureFlag>
     gDfciPkgTokenSpaceGuid.PcdSettingsManagerInstallProvider|TRUE
  }

  DfciPkg/AuthManagerNull/AuthManagerNull.inf
  DfciPkg/IdentityAndAuthManager/IdentityAndAuthManagerDxe.inf
  DfciPkg/DfciManager/DfciManager.inf

  # Device Firmware Configuration Interface (Menu) application
  DfciPkg/Application/DfciMenu/DfciMenu.inf

  #test tools
  DfciPkg/Application/DfciApply/DfciApply.inf

  DfciPkg/Application/EnrollInDfci/EnrollInDfci.inf
  DfciPkg/Application/SetRecovery/SetRecovery.inf

  DfciPkg/UnitTests/DevicdIdTest/DeviceIdTestApp.inf

[BuildOptions]
#force deprecated interfaces off
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
