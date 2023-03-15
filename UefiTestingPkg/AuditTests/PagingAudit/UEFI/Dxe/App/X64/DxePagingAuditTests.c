/** @file -- DxePagingAuditTests.c
    X64 implementations for DXE paging audit tests

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/CpuPageTableLib.h>
#include <Library/DxeMemoryProtectionHobLib.h>

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
  IA32_MAP_ENTRY                             *Map;
  UINTN                                      MapCount;
  UINTN                                      Index;
  BOOLEAN                                    FoundRWXAddress;
  IA32_CR4                                   Cr4;
  PAGING_MODE                                PagingMode;
  UINTN                                      PagesAllocated = 0;
  EFI_STATUS                                 Status;

  Map                      = NULL;
  MapCount                 = 0;
  Index                    = 0;
  FoundRWXAddress          = FALSE;

  // Poll CR4 to deterimine the page table depth
  Cr4.UintN = AsmReadCr4 ();

  if (Cr4.Bits.LA57 != 0) {
    PagingMode = Paging5Level;
  } else {
    PagingMode = Paging4Level;
  }

  // CR3 is the page table pointer
  Status = PageTableParse (AsmReadCr3 (), PagingMode, NULL, &MapCount);

  while (Status == RETURN_BUFFER_TOO_SMALL) {
    if ((Map != NULL) && (PagesAllocated > 0)) {
      FreePages (Map, PagesAllocated);
    }

    PagesAllocated = EFI_SIZE_TO_PAGES (MapCount * sizeof (IA32_MAP_ENTRY));
    Map            = AllocatePages (PagesAllocated);

    UT_ASSERT_NOT_NULL (Map);
    Status = PageTableParse (AsmReadCr3 (), PagingMode, Map, &MapCount);
  }

  for ( ; Index < MapCount; Index++) {
    if ((Map[Index].Attribute.Bits.ReadWrite != 0) && (Map[Index].Attribute.Bits.Nx == 0)) {
      if (!CanRegionBeRWX (Map[Index].LinearAddress, Map[Index].Length)) {
        UT_LOG_ERROR ("Memory Range 0x%llx-0x%llx is Read/Write/Execute\n", Map[Index].LinearAddress, Map[Index].LinearAddress + Map[Index].Length);
        FoundRWXAddress = TRUE;
      } else {
        UT_LOG_WARNING ("Memory Range 0x%llx-0x%llx is Read/Write/Execute. This range is excepted from the test.\n", Map[Index].LinearAddress, Map[Index].LinearAddress + Map[Index].Length);
      }
    }
  }

  UT_ASSERT_FALSE (FoundRWXAddress);

  return UNIT_TEST_PASSED;
}
