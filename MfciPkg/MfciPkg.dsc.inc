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
  !ifdef $(SHIP_MODE)
    !if $(SHIP_MODE) == FALSE
      # to trust the test keys, for development and testing, SHIP_MODE
      # must be explicitly defined to FALSE
      # prevents new users from accidentally shipping test keys
      !include MfciPkg/Private/Certs/CA-test.dsc.inc
      gMfciPkgTokenSpaceGuid.PcdMfciPkcs7RequiredLeafEKU  |$(MFCI_POLICY_EKU_TEST)   # use the test version
    !else
      !include MfciPkg/Private/Certs/SharedMfciTrustAnchor.dsc.inc
      gMfciPkgTokenSpaceGuid.PcdMfciPkcs7RequiredLeafEKU  |$(MFCI_POLICY_EKU_RETAIL) # we use the production version
    !endif
  !else
    !include MfciPkg/Private/Certs/SharedMfciTrustAnchor.dsc.inc
    gMfciPkgTokenSpaceGuid.PcdMfciPkcs7RequiredLeafEKU  |$(MFCI_POLICY_EKU_RETAIL) # we use the production version
  !endif

[LibraryClasses]
  # A platform MUST supply an instance of Tpm2CommandLib. MFCI has been validated against the EDK2's SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf

  # The following NULL library MUST be overridden by a platform.  Scroll down to [Components] for more details
  MfciRetrievePolicyLib|MfciPkg/Library/MfciRetrievePolicyLibNull/MfciRetrievePolicyLibNull.inf

  # The following NULL library MAY be overridden by a platform.  Scroll down to [Components] for more details
  MfciDeviceIdSupportLib|MfciPkg/Library/MfciDeviceIdSupportLibNull/MfciDeviceIdSupportLibNull.inf

  # The following 2 private libraries MUST NOT be overridden by a platform
  MfciPolicyParsingLib|MfciPkg/Private/Library/MfciPolicyParsingLib/MfciPolicyParsingLib.inf

  !if $(OPT_INTO_MFCI_PRE_PRODUCTION) != TRUE
    BreakTheBuildLib|BreakTheBuildPkg/NotFound.inf # Break the build, MFCI is beta software and you need to define this var as TRUE to use it
  !endif

# [Components]

  # =======
  # MfciPei
  # =======
  # For platforms with a PEI phase, MfciPkg/MfciPei/MfciPei.inf provides PEI access to MFCI policy

  # =======
  # MfciDxe
  # =======
  # All MFCI platforms MUST include MfciPkg/MfciDxe/MfciDxe.inf
  #
  # MfciDxe Overrides
  # =================
  #
  # - MfciRetrievePolicyLib - REQUIRED
  # A non-NULL instance of this Lib MUST be supplied by a platform. If using this DSC.INC, it can be overriden in a platform's Components section.
  # MfciPkg supplies 2 implementations below to choose from. For platforms that include MfciPei, they are expected to select ViaHob. For systems where MFCI begins at DXE, the policy is expected to be retrieved ViaVariable.
  # MfciRetrievePolicyLib|MfciPkg/Library/MfciRetrievePolicyLibViaHob/MfciRetrievePolicyLibViaHob.inf
  # MfciRetrievePolicyLib|MfciPkg/Library/MfciRetrievePolicyLibViaVariable/MfciRetrievePolicyLibViaVariable.inf

  # - MfciDeviceIdSupportLib - OPTIONAL
  # A platform MUST provide targeting information to MfciDxe by _either_ populating the targeting UEFI variables prior to EndOfDxe, OR by overriding and implementing MfciDeviceIdSupportLib. A sample instance that may work for SMBIOS-compatible platforms is provided below. If using this DSC.INC, it can be overriden in a platform's Components section.
  # MfciDeviceIdSupportLib|MfciPkg/Library/MfciDeviceIdSupportLibSmbios/MfciDeviceIdSupportLibSmbios.inf

