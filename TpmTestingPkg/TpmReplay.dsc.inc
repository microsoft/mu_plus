## @file
# DSC Include File for TpmTestingPkg
#
# This file contains recommended settings when using this feature. A platform
# may directly include this file or use it as a reference when integrating the
# TPM Replay feature.
#
# Copyright (C) Microsoft Corporation.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[PcdsFixedAtBuild]
  # Always skip TCG ACPI measurements when this feature is enabled
  gEfiSecurityPkgTokenSpaceGuid.PcdSkipTcgSmmAcpiMeasurements|TRUE

  # Do not measure the default S-CRTM version in PEI
  gEfiSecurityPkgTokenSpaceGuid.PcdTpm2ScrtmPolicy|0
