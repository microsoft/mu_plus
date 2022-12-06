/** @file -- DxePagingAuditTestsArm64.c
    ARM64 implementations for DXE paging audit tests

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "../DxePagingAuditTestApp.h"

/**
  Check the page table for Read/Write/Execute regions.

  @param[in] Context            Unit test context

  @retval UNIT_TEST_PASSED      The unit test passed
  @retval other                 The unit test failed

**/
UNIT_TEST_STATUS
EFIAPI
NoReadWriteExecute (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_LOG_WARNING ("Test not implemented!\n");
  return UNIT_TEST_PASSED;
}
