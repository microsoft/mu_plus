/** @file -- DxePagingAuditTestApp.c
This Shell App tests the page table or writes page table and
memory map information to SFS

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "../../PagingAuditCommon.h"

#include <Library/UnitTestLib.h>
#include <Library/CpuPageTableLib.h>
#include <Library/DxeMemoryProtectionHobLib.h>
#include <Protocol/MemoryProtectionSpecialRegionProtocol.h>
#include <Protocol/ShellParameters.h>
#include <Protocol/Shell.h>

#define UNIT_TEST_APP_NAME     "Paging Audit Test"
#define UNIT_TEST_APP_VERSION  "1"
#define MAX_CHARS_TO_READ      3

// TRUE if A interval subsumes B interval
#define CHECK_SUBSUMPTION(AStart, AEnd, BStart, BEnd) \
  ((AStart <= BStart) && (AEnd >= BEnd))

typedef struct _PAGING_AUDIT_TEST_CONTEXT {
  IA32_MAP_ENTRY    *Entries;
  UINTN             Count;
} PAGING_AUDIT_TEST_CONTEXT;

CHAR8  *mMemoryInfoDatabaseBuffer   = NULL;
UINTN  mMemoryInfoDatabaseSize      = 0;
UINTN  mMemoryInfoDatabaseAllocSize = 0;

UNIT_TEST_STATUS
EFIAPI
CheckPageGuards (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN                             MapKey;
  UINTN                             DescriptorSize;
  UINT32                            DescriptorVersion;
  EFI_STATUS                        Status;
  EFI_MEMORY_DESCRIPTOR             *MemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR             *MemoryMapEntry;
  UINTN                             MemoryMapSize             = 0;
  EFI_MEMORY_DESCRIPTOR             *MemoryMap                = NULL;
  BOOLEAN                           TestFailed                = FALSE;
  MEMORY_PROTECTION_DEBUG_PROTOCOL  *MemoryProtectionProtocol = NULL;

  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  MemoryMap,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );

  UT_ASSERT_EQUAL (Status, EFI_BUFFER_TOO_SMALL);
  do {
    MemoryMap = (EFI_MEMORY_DESCRIPTOR *)AllocatePool (MemoryMapSize);
    ASSERT (MemoryMap != NULL);
    Status = gBS->GetMemoryMap (
                    &MemoryMapSize,
                    MemoryMap,
                    &MapKey,
                    &DescriptorSize,
                    &DescriptorVersion
                    );
    if (EFI_ERROR (Status)) {
      FreePool (MemoryMap);
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);

  UT_ASSERT_NOT_EFI_ERROR (gBS->LocateProtocol (&gMemoryProtectionDebugProtocolGuid, NULL, (VOID **)&MemoryProtectionProtocol));

  MemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + MemoryMapSize);
  MemoryMapEntry = MemoryMap;
  while ((UINTN)MemoryMapEntry < (UINTN)MemoryMapEnd) {
    if (GetDxeMemoryTypeSettingFromBitfield (MemoryMapEntry->Type, gDxeMps.HeapGuardPageType)) {
      if (!(MemoryProtectionProtocol->IsGuardPage (MemoryMapEntry->PhysicalStart) ||
            !(MemoryProtectionProtocol->IsGuardPage (MemoryMapEntry->PhysicalStart + EFI_PAGES_TO_SIZE (MemoryMapEntry->NumberOfPages) - EFI_PAGE_SIZE))))
      {
        UT_LOG_ERROR (
          "Memory Range 0x%llx-0x%llx is not flanked by guard pages! Memory type: 0x%d\n",
          MemoryMapEntry->PhysicalStart,
          MemoryMapEntry->PhysicalStart + EFI_PAGES_TO_SIZE (MemoryMapEntry->NumberOfPages),
          MemoryMapEntry->Type
          );
        TestFailed = TRUE;
      }
    }

    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }

  UT_ASSERT_FALSE (TestFailed);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
NoReadWriteExcecute (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  IA32_MAP_ENTRY                             *Map                         = ((PAGING_AUDIT_TEST_CONTEXT *)Context)->Entries;
  UINTN                                      MapCount                     = ((PAGING_AUDIT_TEST_CONTEXT *)Context)->Count;
  UINTN                                      Index                        = 0;
  BOOLEAN                                    FoundReadWriteExecuteAddress = FALSE;
  MEMORY_PROTECTION_DEBUG_PROTOCOL           *MemoryProtectionProtocol    = NULL;
  MEMORY_PROTECTION_SPECIAL_REGION_PROTOCOL  *SpecialRegionProtocol       = NULL;
  MEMORY_PROTECTION_SPECIAL_REGION           *SpecialRegions              = NULL;
  UINTN                                      SpecialRegionCount           = 0;
  UINTN                                      SpecialRegionIndex           = 0;
  IMAGE_RANGE_DESCRIPTOR                     *NonProtectedImageList       = NULL;
  LIST_ENTRY                                 *NonProtectedImageLink       = NULL;
  IMAGE_RANGE_DESCRIPTOR                     *NonProtectedImage           = NULL;

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
            continue;
          }
        }
      }

      if (SpecialRegionCount > 0) {
        for (SpecialRegionIndex = 0; SpecialRegionIndex < SpecialRegionCount; SpecialRegionIndex++) {
          if CHECK_SUBSUMPTION (
               SpecialRegions[SpecialRegionIndex].Start,
               SpecialRegions[SpecialRegionIndex].Start + SpecialRegions[SpecialRegionIndex].Length,
               Map[Index].LinearAddress,
               Map[Index].LinearAddress + Map[Index].Length
               ) {
            continue;
          }
        }
      }

      UT_LOG_ERROR ("Memory Range 0x%llx-0x%llx is Read/Write/Execute\n", Map[Index].LinearAddress, Map[Index].LinearAddress + Map[Index].Length);
      FoundReadWriteExecuteAddress = TRUE;
    }
  }

  UT_ASSERT_FALSE (FoundReadWriteExecuteAddress);

  return UNIT_TEST_PASSED;
}

/**
  DxePagingAuditTestAppEntryPoint

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
DxePagingAuditTestAppEntryPoint (
  IN     EFI_HANDLE        ImageHandle,
  IN     EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                     Status;
  UNIT_TEST_FRAMEWORK_HANDLE     Fw   = NULL;
  UNIT_TEST_SUITE_HANDLE         Misc = NULL;
  PAGING_AUDIT_TEST_CONTEXT      *Context;
  IA32_CR4                       Cr4;
  PAGING_MODE                    PagingMode;
  IA32_MAP_ENTRY                 *Map           = NULL;
  UINTN                          MapCount       = 0;
  UINTN                          PagesAllocated = 0;
  EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParams;
  CHAR8                          Argument[MAX_CHARS_TO_READ + 1];
  BOOLEAN                        RunTests = TRUE;

  DEBUG ((DEBUG_ERROR, "%a()\n", __FUNCTION__));

  DEBUG ((DEBUG_ERROR, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  Argument[MAX_CHARS_TO_READ] = '\0';

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiShellParametersProtocolGuid,
                  (VOID **)&ShellParams
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a Could not retrieve command line args!\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  if (ShellParams->Argc > 1) {
    if (StrnCmp (ShellParams->Argv[1], L" -h", 4)) {
      DEBUG ((DEBUG_INFO, "-h : Print available flags\n"));
      DEBUG ((DEBUG_INFO, "-d : Dump the page table files to the EFI partition\n"));
      RunTests = FALSE;
    } else if (StrnCmp (ShellParams->Argv[1], L" -d", 4)) {
      DumpPagingInfo (NULL, NULL);
      RunTests = FALSE;
    }
  }

  if (RunTests) {
    Context = (PAGING_AUDIT_TEST_CONTEXT *)AllocateZeroPool (sizeof (PAGING_AUDIT_TEST_CONTEXT));

    //
    // Start setting up the test framework for running the tests.
    //
    Status = InitUnitTestFramework (&Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
      goto EXIT;
    }

    Cr4.UintN = AsmReadCr4 ();

    if (Cr4.Bits.LA57 != 0) {
      PagingMode = Paging5Level;
    } else {
      PagingMode = Paging4Level;
    }

    Status = PageTableParse (AsmReadCr3 (), PagingMode, NULL, &MapCount);

    while (Status == RETURN_BUFFER_TOO_SMALL) {
      if ((Map != NULL) && (PagesAllocated > 0)) {
        FreePages (Map, PagesAllocated);
      }

      PagesAllocated = EFI_SIZE_TO_PAGES (MapCount * sizeof (IA32_MAP_ENTRY));
      Map            = AllocatePages (PagesAllocated);
      Status         = PageTableParse (AsmReadCr3 (), PagingMode, Map, &MapCount);
    }

    Context->Entries = Map;
    Context->Count   = MapCount;

    //
    // Create test suite
    //
    CreateUnitTestSuite (&Misc, Fw, "Miscellaneous tests", "Security.Misc", NULL, NULL);

    if (Misc == NULL) {
      DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for TestSuite\n"));
      Status = EFI_OUT_OF_RESOURCES;
      goto EXIT;
    }

    AddTestCase (Misc, "No pages can be read,write,execute", "Security.Misc.NoReadWriteExecute", NoReadWriteExcecute, NULL, NULL, Context);
    AddTestCase (Misc, "Guarded heap ranges are flanked by page guards", "Security.Misc.CheckPageGuards", CheckPageGuards, NULL, NULL, Context);

    //
    // Execute the tests.
    //
    Status = RunAllTestSuites (Fw);
EXIT:

    if (Fw) {
      FreeUnitTestFramework (Fw);
    }

    if ((Map != NULL) && (PagesAllocated > 0)) {
      FreePages (Map, PagesAllocated);
    }
  }

  return EFI_SUCCESS;
} // DxePagingAuditTestAppEntryPoint()
