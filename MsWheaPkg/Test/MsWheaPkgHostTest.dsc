## @file
# MsWheaPkg DSC file used to build host-based unit tests.
#
# Copyright (C) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = MsWheaPkgHostTest
  PLATFORM_GUID           = 5DCE1750-D532-4180-B8D3-5EADA36BAD87
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/MsWheaPkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[Components]
  #
  # Build MdeModulePkg HOST_APPLICATION Tests
  #
  # MsWheaReport
  MsWheaPkg/MsWheaReport/Test/MsWheaReportCommonHostTest.inf {
    <PcdsFixedAtBuild>
      # Disable ASSERTs. We need to test return values.
      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x07
  }
  MsWheaPkg/MsWheaReport/Test/MsWheaReportHERHostTest.inf {
    <PcdsFixedAtBuild>
      gMsWheaPkgTokenSpaceGuid.PcdDeviceIdentifierGuid|{0x16, 0x33, 0x43, 0x92, 0xA2, 0x00, 0x43, 0xEE, 0xBF, 0x63, 0x7F, 0x41, 0xEA, 0x3C, 0xEA, 0xAB}
  }

  # MuTelemetryHelperLib
  MsWheaPkg/Test/UnitTests/Library/MuTelemetryHelperLib/MuTelemetryHelperLibHostTest.inf {
    <LibraryClasses>
      MuTelemetryHelperLib|MsWheaPkg/Library/MuTelemetryHelperLib/MuTelemetryHelperLib.inf
      # NOTE: This is a sloppy way to do this, but will clean it up.
      ReportStatusCodeLib|MsWheaPkg/Library/MuTelemetryHelperLib/MuTelemetryHelperLib.inf
  }