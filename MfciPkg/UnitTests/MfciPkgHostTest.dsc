## @file
# Host Test DSC for the Manufacturer Firmware Configuration Interface (MFCI) Package
#
# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

################################################################################
[Defines]
  PLATFORM_NAME                  = MfciPkg
  PLATFORM_GUID                  = 24621B71-FD5A-4724-87D2-AE9044FB6BC2
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/MfciPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64
  SKUID_IDENTIFIER               = DEFAULT
  BUILD_TARGETS                  = NOOPT

  DEFINE  MFCI_POLICY_EKU_TEST   = "1.3.6.1.4.1.311.45.255.255"

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x3f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80080246
  gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel|0x80080246

  # the unit test uses the test certificate that will also be used for testing end-to-end scenarios
  !include MfciPkg/Private/Certs/CA-test.dsc.inc
  gMfciPkgTokenSpaceGuid.PcdMfciPkcs7RequiredLeafEKU  |$(MFCI_POLICY_EKU_TEST)   # use the test version

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################
[LibraryClasses]
  ArmTrngLib|MdePkg/Library/BaseArmTrngLibNull/BaseArmTrngLibNull.inf
  RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  SmmCpuRendezvousLib|MdePkg/Library/SmmCpuRendezvousLibNull/SmmCpuRendezvousLibNull.inf

[LibraryClasses.X64]
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  Tpm2CommandLib|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2DeviceLibDTpm.inf
  Tpm2DebugLib|SecurityPkg/Library/Tpm2DebugLib/Tpm2DebugLibNull.inf
  MfciRetrievePolicyLib|MfciPkg/Library/MfciRetrievePolicyLibNull/MfciRetrievePolicyLibNull.inf

[LibraryClasses]
  MfciPolicyParsingLib|MfciPkg/Private/Library/MfciPolicyParsingLibNull/MfciPolicyParsingLibNull.inf
  MfciDeviceIdSupportLib|MfciPkg/Library/MfciDeviceIdSupportLibNull/MfciDeviceIdSupportLibNull.inf
  VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MfciPkg/UnitTests/Library/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  FltUsedLib|MdePkg/Library/FltUsedLib/FltUsedLib.inf
  MuTelemetryHelperLib|MsWheaPkg/Library/MuTelemetryHelperLib/MuTelemetryHelperLib.inf

################################################################################
#
# Components section - list of all Components needed by this Platform.
#
################################################################################
[Components]
  MfciPkg/UnitTests/Library/MockResetUtilityLib/MockResetUtilityLib.inf
  MfciPkg/UnitTests/Library/MockBaseCryptLib/MockBaseCryptLib.inf
  MfciPkg/UnitTests/Library/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.inf
  MfciPkg/UnitTests/Library/MockMfciRetrieveTargetPolicyLib/MockMfciRetrieveTargetPolicyLib.inf

  MfciPkg/MfciDxe/Test/MfciTargetingHostTest.inf

  MfciPkg/MfciDxe/Test/MfciVerifyPolicyAndChangeHostTest.inf {
    <LibraryClasses>
      ResetUtilityLib|MfciPkg/UnitTests/Library/MockResetUtilityLib/MockResetUtilityLib.inf
      BaseCryptLib|MfciPkg/UnitTests/Library/MockBaseCryptLib/MockBaseCryptLib.inf
  }

  MfciPkg/MfciDxe/Test/MfciVerifyPolicyAndChangeRoTHostTest.inf {
    <LibraryClasses>
      ResetUtilityLib|MfciPkg/UnitTests/Library/MockResetUtilityLib/MockResetUtilityLib.inf
      MfciRetrieveTargetPolicyLib|MfciPkg/UnitTests/Library/MockMfciRetrieveTargetPolicyLib/MockMfciRetrieveTargetPolicyLib.inf
  }

  MfciPkg/MfciDxe/Test/MfciPublicInterfaceHostTest.inf

  MfciPkg/MfciDxe/Test/MfciMultipleCertsHostTest.inf

[BuildOptions]
  *_*_*_CC_FLAGS            = -D DISABLE_NEW_DEPRECATED_INTERFACES
