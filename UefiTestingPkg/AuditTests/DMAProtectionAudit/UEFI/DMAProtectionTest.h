/** @file -- DMAProtectionTest.h

This is a header file defined helpfer data structures and all test function prototypes:
1) BME Breakdown on ExitBootServices()
2) Check the Status Registers of hardware definitions to verify IOMMU is enabled
3) Check excluded memory ranges are set as reserved

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DMA_PROTECTION_TEST_H_
#define _DMA_PROTECTION_TEST_H_

///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================

UNIT_TEST_STATUS
EFIAPI
CheckBMETeardown (
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
CheckExcludedRegions (
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
CheckIOMMUEnabled (
  IN UNIT_TEST_CONTEXT           Context
  );

#endif // _DMA_PROTECTION_TEST_H_
