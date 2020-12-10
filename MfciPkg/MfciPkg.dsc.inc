## @file
#  !include DSC for the Manufacturer Firmware Configuration Interface (MFCI) Package
#  To be !include'd into your platform DSC
#
#  Copyright (c) Microsoft Corporation
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##


[Defines]
  DEFINE  MFCI_POLICY_EKU_TEST     = "1.3.6.1.4.1.311.45.255.255"
  DEFINE  MFCI_POLICY_EKU_RETAIL   = "1.3.6.1.4.1.311.79.8.1"

[PcdsFixedAtBuild]
  !if $(SHIP_MODE) == FALSE
    !include MfciPkg/Private/Certs/CA-test.dsc.inc
    gMfciPkgTokenSpaceGuid.PcdMfciPkcs7RequiredLeafEKU  |$(MFCI_POLICY_EKU_TEST)   # use the test version
  !else
    !include MfciPkg/Private/Certs/SharedMfciTrustAnchor.dsc.inc
    gMfciPkgTokenSpaceGuid.PcdMfciPkcs7RequiredLeafEKU  |$(MFCI_POLICY_EKU_RETAIL) # we use the production version
  !endif

[LibraryClasses.Common]
  MfciPolicyParsingLib|MfciPkg/Private/Library/MfciPolicyParsingLib/MfciPolicyParsingLib.inf
  MuSecureBootLib|MfciPkg/Private/Library/MuSecureBootLib/MuSecureBootLib.inf

  !if $(OPT_INTO_MFCI_PRE_PRODUCTION) != TRUE
    BreakTheBuildLib|BreakTheBuildPkg/NotFound.inf # Break the build, MFCI is beta software and you need to define this var as TRUE to use it
  !endif


[LibraryClasses.IA32]

[LibraryClasses.X64]
  Tpm2CommandLib|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
  MfciRetrievePolicyLib|MfciPkg/Library/MfciRetrievePolicyLibViaHob/MfciRetrievePolicyLibViaHob.inf

[LibraryClasses.AARCH64]
  Tpm2CommandLib|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
  MfciRetrievePolicyLib|MfciPkg/Library/MfciRetrievePolicyLibViaVariable/MfciRetrievePolicyLibViaVariable.inf

[Components.Common]

[Components.IA32]
  MfciPkg/MfciPei/MfciPei.inf

[Components.X64]
  MfciPkg/MfciDxe/MfciDxe.inf

[Components.AARCH64]
  MfciPkg/MfciDxe/MfciDxe.inf


##
#  Example: to enable population of targeting variables via Project Mu OEM Sample Package's SMBIOS example
#
# [PcdsFeatureFlag]
#   gMfciPkgTokenSpaceGuid.PcdMfciPopulateTargetFromDeviceIdSupportLib|TRUE
#
# [LibraryClasses]
#  # From https://github.com/microsoft/mu_oem_sample.git
#  MfciDeviceIdSupportLib|OemPkg/Library/MfciDeviceIdSupportLibSmbios/MfciDeviceIdSupportLibSmbios.inf
#
##