## @file HidPkg.dsc
# HidPkg DSC file
#
# Copyright (C) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PLATFORM_NAME                  = HidPkg
  PLATFORM_GUID                  = D0C06044-28C7-4525-AA59-A6CE9001F571
  PLATFORM_VERSION               = 1.0
  DSC_SPECIFICATION              = 1.28
  OUTPUT_DIRECTORY               = Build/HidPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64|AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

[PcdsFeatureFlag]

[PcdsFixedAtBuild]

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses.common]
  BaseLib                     |MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib               |MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  DebugLib                    |MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DevicePathLib               |MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  MemoryAllocationLib         |MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib                      |MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PrintLib                    |MdePkg/Library/BasePrintLib/BasePrintLib.inf
  ReportStatusCodeLib         |MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  UefiBootServicesTableLib    |MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiDriverEntryPoint        |MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib                     |MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib |MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiUsbLib                  |MdePkg/Library/UefiUsbLib/UefiUsbLib.inf

  HiiLib                      |MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib          |MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  SafeIntLib                  |MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf # MU_CHANGE - CodeQL change

[Components.X64]
  HidPkg/HidKeyboardDxe/HidKeyboardDxe.inf
  HidPkg/HidMouseAbsolutePointerDxe/HidMouseAbsolutePointerDxe.inf
  HidPkg/UsbKbHidDxe/UsbKbHidDxe.inf
  HidPkg/UsbMouseHidDxe/UsbMouseHidDxe.inf
  HidPkg/UsbHidDxe/UsbHidDxe.inf
  HidPkg/UefiHidDxe/UefiHidDxe.inf

[BuildOptions]
#force deprecated interfaces off
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
