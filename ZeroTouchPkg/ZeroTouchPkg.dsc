## @file
# ZeroTouchPkg.dsc
# ZeroTouch Package Localized Strings and Content
#
# Copyright (C) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PLATFORM_NAME                  = ZeroTouch
  PLATFORM_GUID                  = 709fa506-3f3b-4ea3-9622-453c3881e8a3
  PLATFORM_VERSION               = .10
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/ZeroTouchPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

[PcdsFeatureFlag]

[PcdsFixedAtBuild]

[LibraryClasses.common]

[LibraryClasses.IA32]

[LibraryClasses.X64, LibraryClasses.AARCH64]
  ZeroTouchSettingsLib|ZeroTouchPkg/Library/ZeroTouchSettings/ZeroTouchSettings.inf

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

[Components.IA32]

[Components.X64, Components.AARCH64]
  ZeroTouchPkg/Library/ZeroTouchSettings/ZeroTouchSettings.inf

[BuildOptions]
#force deprecated interaces off
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
