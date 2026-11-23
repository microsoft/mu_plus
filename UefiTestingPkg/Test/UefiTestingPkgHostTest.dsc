## @file
# Host-based tests for UefiTestingPkg.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PLATFORM_NAME           = UefiTestingPkgHostTest
  PLATFORM_GUID           = 5B708A19-93CD-4A66-8EDE-4C597A6603B7
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/UefiTestingPkg/HostTest
  SUPPORTED_ARCHITECTURES = X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[Components]
  UefiTestingPkg/AuditTests/SecureBootCryptoAudit/Host/EcitTableTest.inf
