## @file
#  !include DSC for the Firmware Hot Reset (FHR) support.
#  To be !include'd into your platform DSC
#
#  Copyright (c) Microsoft Corporation
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[LibraryClasses]
  FhrLib|FhrPkg/Library/FhrLib/FhrLib.inf
  FhrBmLib|FhrPkg/Library/FhrBm/FhrBm.inf
  MemoryBinOverrideLib|FhrPkg/Library/FhrMemoryBinOverrideLib/MemoryBinOverrideLib.inf
  FhrResetLib|FhrPkg/Library/FhrResetLibNull/FhrResetLibNull.inf

[Components.IA32,Components.AARCH64]
  FhrPkg/FhrPei/FhrPei.inf

[Components.X64,Components.AARCH64]
  FhrPkg/FhrDxe/FhrDxe.inf
  FhrPkg/FhrResume/FhrResume.inf
