## @file
# ZeroTouchPkg.dsc
# ZeroTouch Package Localized Strings and Content
#
# Copyright (C) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PLATFORM_NAME                  = ZeroTouch
  PLATFORM_GUID                  = 3376b35e-073c-489d-98f5-b764a7ababf1
  PLATFORM_VERSION               = .10
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/ZeroTouchPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64|AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

[PcdsFeatureFlag]

[PcdsFixedAtBuild]

[LibraryClasses.common]

[LibraryClasses.IA32]

[LibraryClasses.X64, LibraryClasses.AARCH64]

[Components]
  MdeModulePkg/Test/ShellTest/FhrFunctionalTestApp/FhrFunctionalTestApp.inf {
    <LibraryClasses>
        UnitTestLib|UnitTestFrameworkPkg/Library/UnitTestLib/UnitTestLib.inf
        UnitTestPersistenceLib|UnitTestFrameworkPkg/Library/UnitTestPersistenceLibNull/UnitTestPersistenceLibNull.inf
        UnitTestResultReportLib|UnitTestFrameworkPkg/Library/UnitTestResultReportLib/UnitTestResultReportLibDebugLib.inf
        UnitTestBootLib|UnitTestFrameworkPkg/Library/UnitTestBootLibNull/UnitTestBootLibNull.inf
  }

[Components.IA32]

[Components.X64, Components.AARCH64]

[BuildOptions]
#force deprecated interfaces off
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
