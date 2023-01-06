/** @file -- DxePagingAuditTestsx86.c
    x86 implementations for DXE paging audit tests

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/CpuPageTableLib.h>
#include <Library/DxeMemoryProtectionHobLib.h>
#include <Protocol/MemoryProtectionSpecialRegionProtocol.h>

#include "../DxePagingAuditTestApp.h"

// TRUE if A interval subsumes B interval
#define CHECK_SUBSUMPTION(AStart, AEnd, BStart, BEnd) \
  ((AStart <= BStart) && (AEnd >= BEnd))

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
  IA32_MAP_ENTRY                             *Map                      = NULL;
  UINTN                                      MapCount                  = 0;
  UINTN                                      Index                     = 0;
  BOOLEAN                                    FoundRWXAddress           = FALSE;
  BOOLEAN                                    IgnoreRWXAddress          = FALSE;
  MEMORY_PROTECTION_DEBUG_PROTOCOL           *MemoryProtectionProtocol = NULL;
  MEMORY_PROTECTION_SPECIAL_REGION_PROTOCOL  *SpecialRegionProtocol    = NULL;
  MEMORY_PROTECTION_SPECIAL_REGION           *SpecialRegions           = NULL;
  UINTN                                      SpecialRegionCount        = 0;
  UINTN                                      SpecialRegionIndex        = 0;
  IMAGE_RANGE_DESCRIPTOR                     *NonProtectedImageList    = NULL;
  LIST_ENTRY                                 *NonProtectedImageLink    = NULL;
  IMAGE_RANGE_DESCRIPTOR                     *NonProtectedImage        = NULL;
  IA32_CR4                                   Cr4;
  PAGING_MODE                                PagingMode;
  UINTN                                      PagesAllocated = 0;
  EFI_STATUS                                 Status;

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

  UT_ASSERT_NOT_EFI_ERROR (
    gBS->LocateProtocol (
           &gMemoryProtectionDebugProtocolGuid,
           NULL,
           (VOID **)&MemoryProtectionProtocol
           )
    );

  UT_ASSERT_NOT_EFI_ERROR (
    MemoryProtectionProtocol->GetImageList (
                                &NonProtectedImageList,
                                NonProtected
                                )
    );

  UT_ASSERT_NOT_EFI_ERROR (
    gBS->LocateProtocol (
           &gMemoryProtectionSpecialRegionProtocolGuid,
           NULL,
           (VOID **)&SpecialRegionProtocol
           )
    );

  UT_ASSERT_NOT_EFI_ERROR (
    SpecialRegionProtocol->GetSpecialRegions (
                             &SpecialRegions,
                             &SpecialRegionCount
                             )
    );

  for ( ; Index < MapCount; Index++) {
    if ((Map[Index].Attribute.Bits.ReadWrite != 0) && (Map[Index].Attribute.Bits.Nx == 0)) {
      IgnoreRWXAddress = FALSE;
      if (NonProtectedImageList != NULL) {
        for (NonProtectedImageLink = NonProtectedImageList->Link.ForwardLink;
             NonProtectedImageLink != &NonProtectedImageList->Link;
             NonProtectedImageLink = NonProtectedImageLink->ForwardLink)
        {
          NonProtectedImage = CR (
                                NonProtectedImageLink,
                                IMAGE_RANGE_DESCRIPTOR,
                                Link,
                                IMAGE_RANGE_DESCRIPTOR_SIGNATURE
                                );
          if CHECK_SUBSUMPTION (
               NonProtectedImage->Base,
               NonProtectedImage->Base + NonProtectedImage->Length,
               Map[Index].LinearAddress,
               Map[Index].LinearAddress + Map[Index].Length
               ) {
            IgnoreRWXAddress = TRUE;
            break;
          }
        }
      }

      if ((SpecialRegionCount > 0) && !IgnoreRWXAddress) {
        for (SpecialRegionIndex = 0; SpecialRegionIndex < SpecialRegionCount; SpecialRegionIndex++) {
          if (CHECK_SUBSUMPTION (
                SpecialRegions[SpecialRegionIndex].Start,
                SpecialRegions[SpecialRegionIndex].Start + SpecialRegions[SpecialRegionIndex].Length,
                Map[Index].LinearAddress,
                Map[Index].LinearAddress + Map[Index].Length
                ) &&
              (SpecialRegions[SpecialRegionIndex].EfiAttributes == 0))
          {
            IgnoreRWXAddress = TRUE;
            break;
          }
        }
      }

      if (!IgnoreRWXAddress) {
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
