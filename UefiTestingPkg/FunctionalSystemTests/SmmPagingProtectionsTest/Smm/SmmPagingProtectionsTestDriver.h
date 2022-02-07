/** @file -- SmmPagingProtectionsTestDriver.h
This internal header file defines interfaces for SMM portion of the SmmPagingProtectionsTest
shared by both traditional and standalone instances.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

// MS_CHANGE - Entire file created.

#ifndef SMM_PAGING_PROTECTION_TEST_DRIVER_H_
#define SMM_PAGING_PROTECTION_TEST_DRIVER_H_

/**
  Common initialization routine of MM Paging Protection driver.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
SmmPagingProtectionsTestInitialization (
  VOID
  );

/**
  This function check if the buffer is valid per processor architecture and not overlap with SMRAM.

  @param Buffer  The buffer start address to be checked.
  @param Length  The buffer length to be checked.

  @retval TRUE  This buffer is valid per processor architecture and not overlap with SMRAM.
  @retval FALSE This buffer is not valid per processor architecture or overlap with SMRAM.
**/
BOOLEAN
EFIAPI
IsBufferOutsideMmValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  );

#endif // SMM_PAGING_PROTECTION_TEST_DRIVER_H_
