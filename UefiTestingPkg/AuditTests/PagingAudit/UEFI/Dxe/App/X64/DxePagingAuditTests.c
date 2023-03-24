/** @file -- DxePagingAuditTests.c
    X64 implementations for DXE paging audit tests

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "../../../X64/PagingAuditX64.h"
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
  UINT64   *Pml4;
  UINT64   *Pte1G;
  UINT64   *Pte2M;
  UINT64   *Pte4K;
  UINTN    Index1;
  UINTN    Index2;
  UINTN    Index3;
  UINTN    Index4;
  UINT64   Address;
  BOOLEAN  FoundRWXAddress;

  FoundRWXAddress = FALSE;
  Pml4            = (UINT64 *)AsmReadCr3 ();

  for (Index1 = 0; Index1 < 0x200; Index1++) {
    Index2 = 0;
    Index3 = 0;
    Index4 = 0;
    if ((Pml4[Index1] & X64_PAGE_TABLE_PRESENT) == 0) {
      continue;
    }

    Pte1G = (UINT64 *)(UINTN)(Pml4[Index1] & X64_PAGE_TABLE_ADDRESS_MASK);

    for (Index2 = 0; Index2 < 0x200; Index2++) {
      Index3 = 0;
      Index4 = 0;
      if (!X64_IS_PRESENT (Pte1G[Index2])) {
        continue;
      }

      if (X64_IS_LEAF (Pte1G[Index2])) {
        Address = IndexToAddress (Index1, Index2, Index3, Index4);
        if (X64_IS_READ_WRITE (Pte1G[Index2]) && X64_IS_EXECUTABLE (Pte1G[Index2])) {
          if (!CanRegionBeRWX (Address, SIZE_1GB)) {
            UT_LOG_ERROR ("Memory Range 0x%016lx-0x%016lx is Read/Write/Execute\n", Address, Address + SIZE_1GB);
            DEBUG ((DEBUG_INFO, "Memory Range 0x%016lx-0x%016lx is Read/Write/Execute\n", Address, Address + SIZE_1GB));
            FoundRWXAddress = TRUE;
          }
        }
      } else {
        Pte2M = (UINT64 *)(UINTN)(Pte1G[Index2] & X64_PAGE_TABLE_ADDRESS_MASK);

        for (Index3 = 0; Index3 < 0x200; Index3++) {
          Index4 = 0;
          if (!X64_IS_PRESENT (Pte2M[Index3])) {
            continue;
          }

          if (X64_IS_LEAF (Pte2M[Index3])) {
            Address = IndexToAddress (Index1, Index2, Index3, Index4);
            if (X64_IS_READ_WRITE (Pte2M[Index3]) && X64_IS_EXECUTABLE (Pte2M[Index3])) {
              if (!CanRegionBeRWX (Address, SIZE_2MB)) {
                UT_LOG_ERROR ("Memory Range 0x%016lx-0x%016lx is Read/Write/Execute\n", Address, Address + SIZE_2MB);
                DEBUG ((DEBUG_INFO, "Memory Range 0x%016lx-0x%016lx is Read/Write/Execute\n", Address, Address + SIZE_2MB));
                FoundRWXAddress = TRUE;
              }
            }
          } else {
            Pte4K = (UINT64 *)(UINTN)(Pte2M[Index3] & X64_PAGE_TABLE_ADDRESS_MASK);

            for (Index4 = 0; Index4 < 0x200; Index4++) {
              if (!X64_IS_PRESENT (Pte4K[Index4])) {
                continue;
              }

              Address = IndexToAddress (Index1, Index2, Index3, Index4);
              if (X64_IS_READ_WRITE (Pte4K[Index4]) && X64_IS_EXECUTABLE (Pte4K[Index4])) {
                if (!CanRegionBeRWX (Address, SIZE_4KB)) {
                  UT_LOG_ERROR ("Memory Range 0x%016lx-0x%016lx is Read/Write/Execute\n", Address, Address + SIZE_4KB);
                  DEBUG ((DEBUG_INFO, "Memory Range 0x%016lx-0x%016lx is Read/Write/Execute\n", Address, Address + SIZE_4KB));
                  FoundRWXAddress = TRUE;
                }
              }
            }
          }
        }
      }
    }
  }

  UT_ASSERT_FALSE (FoundRWXAddress);

  return UNIT_TEST_PASSED;
}
