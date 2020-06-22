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
    # TODO: add the production certificate
    # !include MfciPkg/Private/Certs/CA-production.inc
    gMfciPkgTokenSpaceGuid.PcdMfciPkcs7RequiredLeafEKU  |$(MFCI_POLICY_EKU_RETAIL) # we use the production version
  !endif

[LibraryClasses.Common]
  MfciPolicyParsingLib|MfciPkg/Private/Library/MfciPolicyParsingLib/MfciPolicyParsingLib.inf
  MuSecureBootLib|MfciPkg/Private/Library/MuSecureBootLib/MuSecureBootLib.inf

  !if $(OPT_INTO_MFCI_PRE_PRODUCTION) != TRUE
    BreakTheBuildLib|BreakTheBuildPkg/NotFound.inf # Break the build, MFCI is beta software and you need to define this var as TRUE to use it
  !endif


[LibraryClasses.IA32]
  MfciPkg|MfciPkg/MfciPei/MfciPei.inf

[LibraryClasses.X64, LibraryClasses.AARCH64]
  MfciPkg|MfciPkg/MfciDxe/MfciDxe.inf
  Tpm2CommandLib|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf

[Components.Common]
  MfciPkg/Private/Library/MfciPolicyParsingLib/MfciPolicyParsingLib.inf
  MfciPkg/Private/Library/MuSecureBootLib/MuSecureBootLib.inf

[Components.IA32]
  MfciPkg/MfciPei/MfciPei.inf

[Components.X64, Components.AARCH64]
  MfciPkg/MfciDxe/MfciDxe.inf