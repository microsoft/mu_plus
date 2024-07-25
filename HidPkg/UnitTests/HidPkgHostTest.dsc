## @file
# Host Unit Test DSC for the HID Package
#
# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

################################################################################
[Defines]
  PLATFORM_NAME                  = HidPkg
  PLATFORM_GUID                  = ed24dec6-ab1b-4788-8014-f81f7087e019
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/HidPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64
  SKUID_IDENTIFIER               = DEFAULT
  BUILD_TARGETS                  = NOOPT

  
!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc 

################################################################################
#
# Components section - list of all Components needed by this Platform.
#
################################################################################
[Components]
  HidPkg/HidMouseAbsolutePointerDxe/UnitTest/HidMouseAbsolutePointerHostTest.inf {
    <LibraryClasses>
      UefiLib|MdePkg/Test/Mock/Library/Stub/StubUefiLib/StubUefiLib.inf
      UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
    <PcdsFixedAtBuild>
      #Turn off Halt on Assert and Print Assert so that libraries can
      #be tested in more of a release mode environment
      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0E
  }



[BuildOptions]
  *_*_*_CC_FLAGS            = -D DISABLE_NEW_DEPRECATED_INTERFACES
