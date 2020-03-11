## @file
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = XmlSupportPkgHostTest
  PLATFORM_GUID           = f826100e-74e8-4dce-bb47-2c2d9afd45a0
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/XmlSupportPkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses]

  XmlTreeLib|XmlSupportPkg/Library/XmlTreeLib/XmlTreeLib.inf
  XmlTreeQueryLib|XmlSupportPkg/Library/XmlTreeQueryLib/XmlTreeQueryLib.inf

[Components]
  #
  # Build HOST_APPLICATION that tests the SampleUnitTest
  #
  XmlSupportPkg/Test/UnitTest/XmlTreeLib/XmlTreeLibUnitTestsHost.inf
  XmlSupportPkg/Test/UnitTest/XmlTreeQueryLib/XmlTreeQueryLibUnitTestsHost.inf {
    <PcdsFixedAtBuild>
    #Turn off Halt on Assert and Print Assert so that libraries can
    #be tested in more of a release mode environment
    gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0E
  }
