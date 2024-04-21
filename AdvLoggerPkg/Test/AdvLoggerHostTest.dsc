## @file
# Host Test DSC for the Advanced Logger Package
#
# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

################################################################################
[Defines]
  PLATFORM_NAME                  = AdvLoggerPkgHostTest
  PLATFORM_GUID                  = 1BF313CF-F093-42A9-8676-3445F2544295
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/AdvLoggerPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64
  SKUID_IDENTIFIER               = DEFAULT
  BUILD_TARGETS                  = NOOPT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################
[LibraryClasses.common.HOST_APPLICATION]
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf

  #
  # Mocked Libs
  #
  UefiRuntimeLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiRuntimeLib/MockUefiRuntimeLib.inf
  MemoryAllocationLib|MdePkg/Test/Mock/Library/GoogleTest/MockMemoryAllocationLib/MockMemoryAllocationLib.inf
  UefiBootServicesTableLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiBootServicesTableLib/MockUefiBootServicesTableLib.inf

################################################################################
#
# Components section - list of all Components needed by this Platform.
#
################################################################################
[Components]
  AdvLoggerPkg/AdvLoggerOsConnectorPrm/Library/AdvLoggerOsConnectorPrmConfigLib/GoogleTest/AdvLoggerPrmConfigLibGoogleTest.inf
  AdvLoggerPkg/AdvLoggerOsConnectorPrm/GoogleTest/AdvLoggerOsConnectorPrmGoogleTest.inf
