## @file
# XmlSupportPackage Package Firmware Environment CI dsc file
#
# Copyright (C) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PLATFORM_NAME                  = XmlSupportPkg
  PLATFORM_GUID                  = 6266634F-D749-4571-BBE7-CE99080D3D6A
  PLATFORM_VERSION               = 1.0
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/XmlSupportPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgTarget.dsc.inc

[LibraryClasses]
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  XmlTreeLib|XmlSupportPkg/Library/XmlTreeLib/XmlTreeLib.inf
  XmlTreeQueryLib|XmlSupportPkg/Library/XmlTreeQueryLib/XmlTreeQueryLib.inf

[Components]
  XmlSupportPkg/Library/XmlTreeLib/XmlTreeLib.inf
  XmlSupportPkg/Library/XmlTreeQueryLib/XmlTreeQueryLib.inf

  ##
  # Uefi environment unit tests
  ##
  XmlSupportPkg/Test/UnitTest/XmlTreeLib/XmlTreeLibUnitTestsUefi.inf
  XmlSupportPkg/Test/UnitTest/XmlTreeQueryLib/XmlTreeQueryLibUnitTestsUefi.inf {
    <PcdsFixedAtBuild>
    #Turn off Halt on Assert and Print Assert so that libraries can
    #be tested in more of a release mode environment
    gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0E
  }

[BuildOptions]
#force deprecated interfaces off
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
