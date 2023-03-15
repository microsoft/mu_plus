/** @file -- DxePagingAuditTests.c
    ARM64 implementations for DXE paging audit tests

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/ArmLib.h>
#include <Chipset/AArch64Mmu.h>
#include "../DxePagingAuditTestApp.h"

#define TT_ADDRESS_MASK  (0xFFFFFFFFFULL << 12)
#define IS_TABLE(page, level)  ((level == 3) ? FALSE : (((page) & TT_TYPE_MASK) == TT_TYPE_TABLE_ENTRY))
#define IS_BLOCK(page, level)  ((level == 3) ? (((page) & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY_LEVEL3) : ((page & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY))
#define ROOT_TABLE_LEN(T0SZ)   (TT_ENTRY_COUNT >> ((T0SZ) - 16) % 9)
#define IS_VALID  0x1
#define IS_READ_WRITE(page)  (((page & TT_AP_RW_RW) != 0) || ((page & TT_AP_MASK) == 0))
#define IS_EXECUTABLE(page)  ((page & TT_UXN_MASK) == 0)
#define IS_ACCESSIBLE(page)  ((page & TT_AF) != 0)

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
  UINT64   *Pml0;
  UINT64   *Pte1G;
  UINT64   *Pte2M;
  UINT64   *Pte4K;
  BOOLEAN  FoundRWXAddress;
  UINT64   Index3;
  UINT64   Index2;
  UINT64   Index1;
  UINT64   Index0;
  UINT64   RootEntryCount;
  UINT64   Address;

  FoundRWXAddress = FALSE;

  Pml0           = (UINT64 *)ArmGetTTBR0BaseAddress ();
  RootEntryCount = ROOT_TABLE_LEN (ArmGetTCR () & TCR_T0SZ_MASK);

  for (Index0 = 0x0; Index0 < RootEntryCount; Index0++) {
    Index1 = 0;
    Index2 = 0;
    Index3 = 0;

    if (!IS_TABLE (Pml0[Index0], 0)) {
      continue;
    }

    // Level 0 must always be table entries
    Pte1G = (UINT64 *)(Pml0[Index0] & TT_ADDRESS_MASK);

    for (Index1 = 0x0; Index1 < TT_ENTRY_COUNT; Index1++ ) {
      Index2 = 0;
      Index3 = 0;

      // If the entry is not valid, skip it
      if ((Pte1G[Index1] & IS_VALID) == 0) {
        continue;
      }

      if (!IS_BLOCK (Pte1G[Index1], 1)) {
        // This is a table
        Pte2M = (UINT64 *)(Pte1G[Index1] & TT_ADDRESS_MASK);

        for (Index2 = 0x0; Index2 < TT_ENTRY_COUNT; Index2++ ) {
          Index3 = 0;

          // If the entry is not valid, skip it
          if ((Pte2M[Index2] & IS_VALID) == 0) {
            continue;
          }

          if (!IS_BLOCK (Pte2M[Index2], 2)) {
            // This is a table
            Pte4K = (UINT64 *)(Pte2M[Index2] & TT_ADDRESS_MASK);

            for (Index3 = 0x0; Index3 < TT_ENTRY_COUNT; Index3++ ) {
              // If the entry is not valid, skip it
              if ((Pte4K[Index3] & IS_VALID) == 0) {
                continue;
              }

              // This is a block
              if (IS_READ_WRITE (Pte4K[Index3]) &&     // Read/Write
                  IS_EXECUTABLE (Pte4K[Index3]) &&     // Execute
                  IS_ACCESSIBLE (Pte4K[Index3]))       // Access Flag (0 for guard pages)
              {
                Address = IndexToAddress (Index0, Index1, Index2, Index3);

                if (!CanRegionBeRWX (Address, SIZE_4KB)) {
                  UT_LOG_ERROR ("Memory Range 0x%llx-0x%llx is Read/Write/Execute\n", Address, Address + SIZE_4KB);
                  FoundRWXAddress = TRUE;
                } else {
                  UT_LOG_WARNING ("Memory Range 0x%llx-0x%llx is Read/Write/Execute. This range is excepted from the test.\n", Address, Address + SIZE_4KB);
                }
              }
            }
          } else {
            // This is an block
            if (IS_READ_WRITE (Pte2M[Index2]) &&   // Read/Write
                IS_EXECUTABLE (Pte2M[Index2]) &&   // Execute
                IS_ACCESSIBLE (Pte2M[Index2]))     // Access Flag (0 for guard pages)
            {
              Address = IndexToAddress (Index0, Index1, Index2, Index3);

              if (!CanRegionBeRWX (Address, SIZE_2MB)) {
                UT_LOG_ERROR ("Memory Range 0x%llx-0x%llx is Read/Write/Execute\n", Address, Address + SIZE_2MB);
                FoundRWXAddress = TRUE;
              } else {
                UT_LOG_WARNING ("Memory Range 0x%llx-0x%llx is Read/Write/Execute. This range is excepted from the test.\n", Address, Address + SIZE_2MB);
              }
            }
          }
        }
      } else {
        // This is an block
        if (IS_READ_WRITE (Pte1G[Index1]) &&     // Read/Write
            IS_EXECUTABLE (Pte1G[Index1]) &&     // Execute
            IS_ACCESSIBLE (Pte1G[Index1]))       // Access Flag (0 for guard pages)
        {
          Address = IndexToAddress (Index0, Index1, Index2, Index3);

          if (!CanRegionBeRWX (Address, SIZE_1GB)) {
            UT_LOG_ERROR ("Memory Range 0x%llx-0x%llx is Read/Write/Execute\n", Address, Address + SIZE_1GB);
            FoundRWXAddress = TRUE;
          } else {
            UT_LOG_WARNING ("Memory Range 0x%llx-0x%llx is Read/Write/Execute. This range is excepted from the test.\n", Address, Address + SIZE_1GB);
          }
        }
      }
    }
  }

  UT_ASSERT_FALSE (FoundRWXAddress);

  return UNIT_TEST_PASSED;
}
