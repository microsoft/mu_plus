## @file
# MsGraphics Package Package build file
#
# Copyright (C) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PLATFORM_NAME                  = MsGraphicsPkg
  PLATFORM_GUID                  = 87407FA2-5B88-4D94-B187-96E8644F9112
  PLATFORM_VERSION               = .10
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/MsGraphicsPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

[PcdsFeatureFlag]


[PcdsFixedAtBuild]

!include MdePkg/MdeLibs.dsc.inc


[LibraryClasses.common]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf

[LibraryClasses.common]

  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  MathLib|MsCorePkg/Library/MathLib/MathLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf

  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  FrameBufferBltLib|MdeModulePkg/Library/FrameBufferBltLib/FrameBufferBltLib.inf
  BmpSupportLib|MdeModulePkg/Library/BaseBmpSupportLib/BaseBmpSupportLib.inf

  MsUiThemeCopyLib|MsGraphicsPkg/Library/MsUiThemeCopyLib/MsUiThemeCopyLib.inf
  PlatformThemeLib|MsGraphicsPkg/Library/SamplePlatformThemeLib/PlatformThemeLib.inf
  UiProgressCircleLib|MsGraphicsPkg/Library/BaseUiProgressCircleLib/UiProgressCircleLib.inf
  UiRectangleLib|MsGraphicsPkg/Library/BaseUiRectangleLib/BaseUiRectangleLib.inf
  MsUiThemeLib|MsGraphicsPkg/Library/MsUiThemeLib/Dxe/MsUiThemeLib.inf
  MsPlatformEarlyGraphicsLib|MsGraphicsPkg/Library/MsEarlyGraphicsLibNull/Dxe/MsEarlyGraphicsLibNull.inf
  DisplayDeviceStateLib|MsGraphicsPkg/Library/DisplayDeviceStateLibNull/DisplayDeviceStateLibNull.inf
  FrameBufferMemDrawLib|MsGraphicsPkg/Library/FrameBufferMemDrawLibNull/FrameBufferMemDrawLibNull.inf
  MsColorTableLib|MsGraphicsPkg/Library/MsColorTableLib/MsColorTableLib.inf
  SwmDialogsLib|MsGraphicsPkg/Library/SwmDialogsLib/SwmDialogs.inf
  BootGraphicsLib|MsGraphicsPkg/Library/BootGraphicsLibNull/BootGraphicsLib.inf
  BootGraphicsProviderLib|MsGraphicsPkg/Library/BootGraphicsProviderLibNull/BootGraphicsProviderLib.inf
  QrEncoderLib|MsGraphicsPkg/Library/QrEncoderLib/QrEncoderLib.inf


  #ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  #FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  FltUsedLib|MdePkg/Library/FltUsedLib/FltUsedLib.inf

[LibraryClasses.common.PEIM]
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf

  MsUiThemeLib|MsGraphicsPkg/Library/MsUiThemeLib/Pei/MsUiThemeLib.inf
  MsPlatformEarlyGraphicsLib|MsGraphicsPkg/Library/MsEarlyGraphicsLibNull/Pei/MsEarlyGraphicsLibNull.inf

[LibraryClasses.common.UEFI_APPLICATION]
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf

[LibraryClasses.IA32]

[LibraryClasses.X64, LibraryClasses.AARCH64]
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  MsUiThemeLib|MsGraphicsPkg/Library/MsUiThemeLib/Dxe/MsUiThemeLib.inf
  UIToolKitLib|MsGraphicsPkg/Library/SimpleUIToolKit/SimpleUIToolKit.inf

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

[Components]
  MsGraphicsPkg/Library/MsUiThemeCopyLib/MsUiThemeCopyLib.inf
  MsGraphicsPkg/Library/SamplePlatformThemeLib/PlatformThemeLib.inf
  MsGraphicsPkg/Library/BaseUiProgressCircleLib/UiProgressCircleLib.inf
  MsGraphicsPkg/Library/MsUiThemeLib/Dxe/MsUiThemeLib.inf
  MsGraphicsPkg/Library/MsUiThemeLib/Pei/MsUiThemeLib.inf
  MsGraphicsPkg/Library/MsColorTableLib/MsColorTableLib.inf
  MsGraphicsPkg/Library/FrameBufferMemDrawLib/FrameBufferMemDrawLib.inf
  MsGraphicsPkg/Library/SwmDialogsLib/SwmDialogs.inf
  MsGraphicsPkg/Library/BootGraphicsLib/BootGraphicsLib.inf
  MsGraphicsPkg/Library/BootGraphicsLibNull/BootGraphicsLib.inf
  MsGraphicsPkg/Library/BootGraphicsProviderLibNull/BootGraphicsProviderLib.inf
  MsGraphicsPkg/Library/QrEncoderLib/QrEncoderLib.inf
  MsGraphicsPkg/Application/BmpDisplay/BmpDisplay.inf
  MsGraphicsPkg/Universal/TimeoutSpinner/TimeoutSpinner.inf
  MsGraphicsPkg/UnitTests/SpinnerTest/SpinnerTest.inf
  # Null Version of FrameBufferMemDrawLib
  MsGraphicsPkg/Library/FrameBufferMemDrawLibNull/FrameBufferMemDrawLibNull.inf
[Components.IA32]
  # Early Graphics driver.
  MsGraphicsPkg/MsEarlyGraphics/Pei/MsEarlyGraphics.inf
  MsGraphicsPkg/MsUiTheme/Pei/MsUiThemePpi.inf
  MsGraphicsPkg/Library/MsEarlyGraphicsLibNull/Pei/MsEarlyGraphicsLibNull.inf

[Components.X64, Components.AARCH64]
  MsGraphicsPkg/Library/MsEarlyGraphicsLibNull/Dxe/MsEarlyGraphicsLibNull.inf
  MsGraphicsPkg/Library/SimpleUIToolKit/SimpleUIToolKit.inf

  # Graphics (GOP) Override driver.
  MsGraphicsPkg/GopOverrideDxe/GopOverrideDxe.inf

  # MsUiThemeProtocol driver.
  MsGraphicsPkg/MsUiTheme/Dxe/MsUiThemeProtocol.inf

  # OnScreenKeyboard driver.
  MsGraphicsPkg/OnScreenKeyboardDxe/OnScreenKeyboardDxe.inf

  # Rendering Engine (RE) driver.
  MsGraphicsPkg/RenderingEngineDxe/RenderingEngineDxe.inf

  # Simple Window Manager (SWM) driver.
  MsGraphicsPkg/SimpleWindowManagerDxe/SimpleWindowManagerDxe.inf

  # Display Engine driver.
  MsGraphicsPkg/DisplayEngineDxe/DisplayEngineDxe.inf

  # Early Graphics driver.
  MsGraphicsPkg/MsEarlyGraphics/Dxe/MsEarlyGraphics.inf

  # Display On Screen Notifications Driver
  MsGraphicsPkg/Library/ColorBarDisplayDeviceStateLib/ColorBarDisplayDeviceStateLib.inf

  # Null Display On Screen Notifications Driver
  MsGraphicsPkg/Library/DisplayDeviceStateLibNull/DisplayDeviceStateLibNull.inf

  # Rectangle Library
  MsGraphicsPkg/Library/BaseUiRectangleLib/BaseUiRectangleLib.inf

  # Print screen module
  MsGraphicsPkg/PrintScreenLogger/PrintScreenLogger.inf

  # NVidia Support Driver
  MsGraphicsPkg/NvidiaSupportDxe/NvidiaSupportDxe.inf

[BuildOptions]
#force deprecated interfaces off
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
