##
# Copyright (c) 2016, Microsoft Corporation

# All rights reserved.
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##


[Defines]
  PLATFORM_NAME                  = MsSampleFmpDevicePkg
  PLATFORM_GUID                  = AE077D0A-C4E4-4395-B6CF-25001BD6A517
  PLATFORM_VERSION               = 0.96
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/MsSampleFmpDevicePkg
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

[LibraryClasses]
  #
  # Entry point
  #
  PeiCoreEntryPoint|MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf

  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf 
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf 
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  FmpAuthenticationLib|SecurityPkg/Library/FmpAuthenticationLibPkcs7/FmpAuthenticationLibPkcs7.inf

  CapsuleUpdatePolicyLib|MsCapsuleUpdatePkg/Library/CapsuleUpdatePolicyLibNull/CapsuleUpdatePolicyLibNull.inf
  MsFmpPayloadHeaderLib|MsCapsuleUpdatePkg/Library/MsFmpPayloadHeaderV1DxeLib/MsFmpPayloadHeaderV1DxeLib.inf
  BmpSupportLib|MsCapsuleUpdatePkg/Library/BaseBmpSupportLib/BaseBmpSupportLib.inf
  IntSafeLib|MdePkg/Library/IntSafeLib/IntSafeLib.inf
  CapsuleLib|MsCapsuleUpdatePkg/Library/DxeCapsuleLib/DxeCapsuleLib.inf
  FmpHelperLib|MsCapsuleUpdatePkg/Library/FmpHelperLib/FmpHelperDxeLib.inf
  FmpPolicyLib|MsCapsuleUpdatePkg/Library/FmpPolicyLib/FmpPolicyDxeLib.inf
  # For update progress text display:
  #DisplayUpdateProgressLib|MsCapsuleUpdatePkg/Library/DisplayUpdateProgressTextLib/DisplayUpdateProgressTextLib.inf
  # For update progress bar (graphics) display:
  DisplayUpdateProgressLib|MsCapsuleUpdatePkg/Library/DisplayUpdateProgressGraphicsLib/DisplayUpdateProgressGraphicsLib.inf
  FmpWrapperDeviceLib|MsCapsuleUpdatePkg/Library/FmpWrapperDeviceLib/FmpWrapperDeviceLib.inf
  CapsuleKeyLib|MsSampleFmpDevicePkg/Library/CapsuleKeyBaseLib/CapsuleKeyBaseLib.inf


[LibraryClasses.X64]
  #
  # DXE phase common
  #
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf


[PcdsFeatureFlag]
#  <SET FEATURE PCDs FLAGS FOR THIS BUILD HERE>


[PcdsFixedAtBuild]
#  <SET FIXED AT BUILD PCDs FOR THIS BUILD HERE>


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
  MsSampleFmpDevicePkg/Library/SampleFmpDeviceLib/SampleFmpDeviceLib.inf

  MsSampleFmpDevicePkg/SampleDeviceLibWrapperFmpDxeDriver/SampleDeviceLibWrapperFMP.inf {
  <LibraryClasses>
    FmpDeviceLib|MsSampleFmpDevicePkg/Library/SampleFmpDeviceLib/SampleFmpDeviceLib.inf
  <PcdsFixedAtBuild>
    gMsCapsuleUpdatePkgTokenSpaceGuid.PcdDeviceLibWrapperDeviceImageName|L"Sample Device FMP"
    #set Lowest supported version
    gMsCapsuleUpdatePkgTokenSpaceGuid.PcdBuildTimeLowestSupportedVersion|0x0 #0.0.0
    #set to White (RGB) (255, 255, 255)
    gMsCapsuleUpdatePkgTokenSpaceGuid.PcdProgressColor|0xFFFFFFFF
    #Take note - GUIDs/UUIDs have different formatting.  To get them all align can be challenging.
    #When doing byte array the last 8 bytes are MSB while the previous bytes are in LSB
    #UUID HEX FMT:   11223344-5566-7788-99AA-BBCCDDEEFF00
    #gMsCapsuleUpdatePkgTokenSpaceGuid.PcdDeviceLibWrapperDeviceGuid|{0x44, 0x33, 0x22, 0x11, 0x66, 0x55, 0x88, 0x77, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00}
    #Populate GUID (from MsSampleFmpDevicePkg\Tools\Guid.txt) below as per the example above
    #UUID HEX FMT:   ########-####-####-####-############
    gMsCapsuleUpdatePkgTokenSpaceGuid.PcdDeviceLibWrapperDeviceGuid|{0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##, 0x##}
  <PcdsFeatureFlag>
    gMsCapsuleUpdatePkgTokenSpaceGuid.PcdDeviceLibWrapperSystemResetRequired|TRUE
  }

[BuildOptions]
  #Turn off optimization to ease in debugging. 
  DEBUG_VS2015x86_*_CC_FLAGS     = /Od

