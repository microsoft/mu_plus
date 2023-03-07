/** @file PageSplitTest.c
TCBZ3519
Functionality to support MemoryAttributeProtocolFuncTestApp.c

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
***/

#include "MemoryAttributeProtocolFuncTestApp.h"

#define PAGING_PAE_INDEX_MASK    0x1FF
#define PAGE_TABLE_PRESENT_BIT   0x1
#define PAGE_TABLE_BASE_ADDRESS  0xFFFFFFFFFF000
#define PAGE_TABLE_IS_LEAF       0x80
#define PAGE_TABLE_NX            0x8000000000000000

/**
  Get an un-split page table entry and allocate entire region so the page
  doesn't need to be split on allocation

  @param[out]  Address  Address of allocated 2MB page region
**/
EFI_STATUS
EFIAPI
GetUnsplitPageTableEntry (
  OUT EFI_PHYSICAL_ADDRESS  *Address
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  BaseAddress;
  UINT64                *L4Table;
  UINT64                *L3Table;
  UINT64                *L2Table;
  UINTN                 Index2;
  UINTN                 Index3;
  UINTN                 Index4;

  L4Table = (UINT64 *)AsmReadCr3 ();

  for (Index4 = 0x0; Index4 < 0x200; Index4++) {
    if (!L4Table[Index4] & PAGE_TABLE_PRESENT_BIT) {
      continue;
    }

    L3Table = (UINT64 *)(L4Table[Index4] & PAGE_TABLE_BASE_ADDRESS);

    for (Index3 = 0x0; Index3 < 0x200; Index3++ ) {
      if (!L3Table[Index3] & PAGE_TABLE_PRESENT_BIT) {
        continue;
      }

      if (!(L3Table[Index3] & PAGE_TABLE_IS_LEAF)) {
        L2Table = (UINT64 *)(L3Table[Index3] & PAGE_TABLE_BASE_ADDRESS);

        for (Index2 = 0x0; Index2 < 0x200; Index2++ ) {
          if (L2Table[Index2] & PAGE_TABLE_IS_LEAF) {
            BaseAddress = (Index4 * PTE512GB) + (Index3 * PTE1GB) + (Index2 * PTE2MB);
            Status      = gBS->AllocatePages (AllocateAddress, EfiLoaderCode, EFI_SIZE_TO_PAGES (PTE2MB), &BaseAddress);
            if (!EFI_ERROR (Status)) {
              *Address = BaseAddress;
              return EFI_SUCCESS;
            }
          }
        }
      }
    }
  }

  return EFI_OUT_OF_RESOURCES;
}

/**
  Check if the 2MB page entry correlating with the input address
  is set to no-execute

  @param[in]  Address  Address of the page table entry
**/
UINT64
EFIAPI
GetSpitPageTableEntryNoExecute (
  IN  PHYSICAL_ADDRESS  Address
  )
{
  UINT64  *L4Table;
  UINT64  *L3Table;
  UINT64  *L2Table;
  UINTN   Index4;
  UINTN   Index3;
  UINTN   Index2;

  Index4 = ((UINTN)RShiftU64 (Address, 39)) & PAGING_PAE_INDEX_MASK;
  Index3 = ((UINTN)Address >> 30) & PAGING_PAE_INDEX_MASK;
  Index2 = ((UINTN)Address >> 21) & PAGING_PAE_INDEX_MASK;

  L4Table = (UINT64 *)AsmReadCr3 ();
  L3Table = (UINT64 *)(L4Table[Index4] & PAGE_TABLE_BASE_ADDRESS);
  L2Table = (UINT64 *)(L3Table[Index3] & PAGE_TABLE_BASE_ADDRESS);

  return L2Table[Index2] & PAGE_TABLE_NX;
}
