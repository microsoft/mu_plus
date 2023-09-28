/** @file -- DxePagingAuditTestApp.c
This Shell App tests the page table or writes page table and
memory map information to SFS

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "../../PagingAuditCommon.h"

#include <Protocol/ShellParameters.h>
#include <Protocol/Shell.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/MemoryProtectionSpecialRegionProtocol.h>
#include <Protocol/MemoryProtectionDebug.h>

#include <Library/FileHandleLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/FlatPageTableLib.h>
#include <Library/UnitTestLib.h>

#define UNIT_TEST_APP_NAME     "Paging Audit Test"
#define UNIT_TEST_APP_VERSION  "1"
#define MAX_CHARS_TO_READ      3

// TRUE if A interval subsumes B interval
#define CHECK_SUBSUMPTION(AStart, AEnd, BStart, BEnd) \
  ((AStart <= BStart) && (AEnd >= BEnd))

CHAR8                             *mMemoryInfoDatabaseBuffer   = NULL;
UINTN                             mMemoryInfoDatabaseSize      = 0;
UINTN                             mMemoryInfoDatabaseAllocSize = 0;
MEMORY_PROTECTION_SPECIAL_REGION  *mSpecialRegions             = NULL;
IMAGE_RANGE_DESCRIPTOR            *mNonProtectedImageList      = NULL;
UINTN                             mSpecialRegionCount          = 0;
EFI_GCD_MEMORY_SPACE_DESCRIPTOR   *mMemorySpaceMap             = NULL;
UINTN                             mMemorySpaceMapCount         = 0;
PAGE_MAP                          mMap                         = { 0 };

/**
  Frees the entries in the mMap global.

  @param[in] Context  Unit test context.
**/
STATIC
VOID
EFIAPI
FreePageTableMap (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  if (mMap.Entries != NULL) {
    FreePages (mMap.Entries, mMap.EntryPagesAllocated);
  }
}

/**
  Populate the global flat page table map.

  @param[in] Context            Unit test context.

  @retval EFI_SUCCESS           The page table is parsed successfully.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for the page table map.
  @retval EFI_INVALID_PARAMETER An error occurred while parsing the page table.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
PopulatePageTableMap (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  Status = CreateFlatPageTable (&mMap);

  while (Status == RETURN_BUFFER_TOO_SMALL) {
    if ((mMap.Entries != NULL) && (mMap.EntryPagesAllocated > 0)) {
      FreePages (mMap.Entries, mMap.EntryPagesAllocated);
    }

    mMap.EntryPagesAllocated = EFI_SIZE_TO_PAGES (mMap.EntryCount * sizeof (PAGE_MAP_ENTRY));
    mMap.Entries             = AllocatePages (mMap.EntryPagesAllocated);

    if (mMap.Entries == NULL) {
      UT_LOG_ERROR ("Failed to allocate %d pages for page table map!\n", mMap.EntryPagesAllocated);
      return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
    }

    Status = CreateFlatPageTable (&mMap);
  }

  if ((Status != EFI_SUCCESS) && (mMap.Entries != NULL)) {
    FreePageTableMap (Context);
  }

  return Status == EFI_SUCCESS ? UNIT_TEST_PASSED : UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
}

/**
  Populates the non protected image list global
**/
VOID
GetNonProtectedImageList (
  VOID
  )
{
  EFI_STATUS                        Status;
  MEMORY_PROTECTION_DEBUG_PROTOCOL  *MemoryProtectionProtocol;

  MemoryProtectionProtocol = NULL;

  if (mNonProtectedImageList != NULL) {
    return;
  }

  Status = gBS->LocateProtocol (
                  &gMemoryProtectionDebugProtocolGuid,
                  NULL,
                  (VOID **)&MemoryProtectionProtocol
                  );

  if (!EFI_ERROR (Status)) {
    Status = MemoryProtectionProtocol->GetImageList (
                                         &mNonProtectedImageList,
                                         NonProtected
                                         );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%d - Unable to fetch non-protected image list\n", __FUNCTION__, __LINE__));
    mNonProtectedImageList = NULL;
  }
}

/**
  Populates the special region array global
**/
VOID
GetSpecialRegions (
  VOID
  )
{
  EFI_STATUS                                 Status;
  MEMORY_PROTECTION_SPECIAL_REGION_PROTOCOL  *SpecialRegionProtocol;

  SpecialRegionProtocol = NULL;

  if (mSpecialRegions != NULL) {
    return;
  }

  Status = gBS->LocateProtocol (
                  &gMemoryProtectionSpecialRegionProtocolGuid,
                  NULL,
                  (VOID **)&SpecialRegionProtocol
                  );

  if (!EFI_ERROR (Status)) {
    Status = SpecialRegionProtocol->GetSpecialRegions (
                                      &mSpecialRegions,
                                      &mSpecialRegionCount
                                      );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%d - Unable to fetch special region list\n", __FUNCTION__, __LINE__));
    mNonProtectedImageList = NULL;
  }
}

/**
  Checks if a region is allowed to be read/write/execute based on the special region array
  and non protected image list

  @param[in] Address            Start address of the region
  @param[in] Length             Length of the region

  @retval TRUE                  The region is allowed to be read/write/execute
  @retval FALSE                 The region is not allowed to be read/write/execute
**/
STATIC
BOOLEAN
CanRegionBeRWX (
  IN UINT64  Address,
  IN UINT64  Length
  )
{
  LIST_ENTRY              *NonProtectedImageLink;
  IMAGE_RANGE_DESCRIPTOR  *NonProtectedImage;
  UINTN                   SpecialRegionIndex, MemorySpaceMapIndex;

  if ((mNonProtectedImageList == NULL) && (mSpecialRegions == NULL)) {
    return FALSE;
  }

  if (mSpecialRegions != NULL) {
    for (SpecialRegionIndex = 0; SpecialRegionIndex < mSpecialRegionCount; SpecialRegionIndex++) {
      if (CHECK_SUBSUMPTION (
            mSpecialRegions[SpecialRegionIndex].Start,
            mSpecialRegions[SpecialRegionIndex].Start + mSpecialRegions[SpecialRegionIndex].Length,
            Address,
            Address + Length
            ) &&
          (mSpecialRegions[SpecialRegionIndex].EfiAttributes == 0))
      {
        return TRUE;
      }
    }
  }

  if (mNonProtectedImageList != NULL) {
    for (NonProtectedImageLink = mNonProtectedImageList->Link.ForwardLink;
         NonProtectedImageLink != &mNonProtectedImageList->Link;
         NonProtectedImageLink = NonProtectedImageLink->ForwardLink)
    {
      NonProtectedImage = CR (NonProtectedImageLink, IMAGE_RANGE_DESCRIPTOR, Link, IMAGE_RANGE_DESCRIPTOR_SIGNATURE);
      if (CHECK_SUBSUMPTION (
            NonProtectedImage->Base,
            NonProtectedImage->Base + NonProtectedImage->Length,
            Address,
            Address + Length
            ))
      {
        return TRUE;
      }
    }
  }

  if (mMemorySpaceMap != NULL) {
    for (MemorySpaceMapIndex = 0; MemorySpaceMapIndex < mMemorySpaceMapCount; MemorySpaceMapIndex++) {
      if (CHECK_SUBSUMPTION (
            mMemorySpaceMap[MemorySpaceMapIndex].BaseAddress,
            mMemorySpaceMap[MemorySpaceMapIndex].BaseAddress + mMemorySpaceMap[MemorySpaceMapIndex].Length,
            Address,
            Address + Length
            ) &&
          (mMemorySpaceMap[MemorySpaceMapIndex].GcdMemoryType == EfiGcdMemoryTypeNonExistent))
      {
        return TRUE;
      }
    }
  }

  return FALSE;
}

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
  UINTN    Index;
  BOOLEAN  FoundRWXAddress;

  UT_ASSERT_NOT_NULL (mMap.Entries);
  UT_ASSERT_NOT_EQUAL (mMap.EntryCount, 0);

  Index           = 0;
  FoundRWXAddress = FALSE;

  for ( ; Index < mMap.EntryCount; Index++) {
    if (IsPageExecutable (mMap.Entries[Index].PageEntry) &&
        IsPageReadable (mMap.Entries[Index].PageEntry) &&
        IsPageWritable (mMap.Entries[Index].PageEntry))
    {
      if (!CanRegionBeRWX (mMap.Entries[Index].LinearAddress, mMap.Entries[Index].Length)) {
        UT_LOG_ERROR (
          "Memory Range 0x%llx-0x%llx is Read/Write/Execute\n",
          mMap.Entries[Index].LinearAddress,
          mMap.Entries[Index].LinearAddress + mMap.Entries[Index].Length
          );
        FoundRWXAddress = TRUE;
      }
    }
  }

  UT_ASSERT_FALSE (FoundRWXAddress);

  return UNIT_TEST_PASSED;
}

/**

 Locates and opens the SFS volume containing the application and, if successful, returns an
 FS handle to the opened volume.

  @param    mFs_Handle       Handle to the opened volume.

  @retval   EFI_SUCCESS     The FS volume was opened successfully.
  @retval   Others          The operation failed.

**/
STATIC
EFI_STATUS
OpenAppSFS (
  OUT EFI_FILE  **Fs_Handle
  )
{
  EFI_DEVICE_PATH_PROTOCOL         *DevicePath;
  BOOLEAN                          Found;
  EFI_HANDLE                       Handle;
  EFI_HANDLE                       *HandleBuffer;
  UINTN                            Index;
  UINTN                            NumHandles;
  EFI_STRING                       PathNameStr;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SfProtocol;
  EFI_STATUS                       Status;
  EFI_FILE_PROTOCOL                *FileHandle;
  EFI_FILE_PROTOCOL                *FileHandle2;

  Status       = EFI_SUCCESS;
  SfProtocol   = NULL;
  NumHandles   = 0;
  HandleBuffer = NULL;

  //
  // Locate all handles that are using the SFS protocol.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &NumHandles,
                  &HandleBuffer
                  );

  if (EFI_ERROR (Status) != FALSE) {
    DEBUG ((DEBUG_ERROR, "%a: failed to locate all handles using the Simple FS protocol (%r)\n", __FUNCTION__, Status));
    goto CleanUp;
  }

  //
  // Search the handles to find one that is on a GPT partition on a hard drive.
  //
  Found = FALSE;
  for (Index = 0; (Index < NumHandles) && (Found == FALSE); Index += 1) {
    DevicePath = DevicePathFromHandle (HandleBuffer[Index]);
    if (DevicePath == NULL) {
      continue;
    }

    //
    // Convert the device path to a string to print it.
    //
    PathNameStr = ConvertDevicePathToText (DevicePath, TRUE, TRUE);
    DEBUG ((DEBUG_ERROR, "%a: device path %d -> %s\n", __FUNCTION__, Index, PathNameStr));

    //
    // Check if this is a block IO device path. If it is not, keep searching.
    // This changes our locate device path variable, so we'll have to restore
    // it afterwards.
    //
    Status = gBS->LocateDevicePath (
                    &gEfiBlockIoProtocolGuid,
                    &DevicePath,
                    &Handle
                    );

    if (EFI_ERROR (Status) != FALSE) {
      DEBUG ((DEBUG_ERROR, "%a: not a block IO device path\n", __FUNCTION__));
      continue;
    }

    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID **)&SfProtocol
                    );

    if (EFI_ERROR (Status) != FALSE) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to locate Simple FS protocol using the handle to fs0: %r \n", __FUNCTION__, Status));
      goto CleanUp;
    }

    //
    // Open the volume/partition.
    //
    Status = SfProtocol->OpenVolume (SfProtocol, &FileHandle);
    if (EFI_ERROR (Status) != FALSE) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to open Simple FS volume fs0: %r \n", __FUNCTION__, Status));
      goto CleanUp;
    }

    //
    // Ensure the PktName file is present
    //
    Status = FileHandle->Open (FileHandle, &FileHandle2, L"DxePagingAuditTestApp.efi", EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "%a: Unable to locate %s. Status: %r\n", __FUNCTION__, L"DxePagingAuditTestApp.efi", Status));
      Status = FileHandleClose (FileHandle);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Error closing Vol Handle. Code = %r\n", __FUNCTION__, Status));
      }

      Status = EFI_NOT_FOUND;
      continue;
    } else {
      DEBUG ((DEBUG_ERROR, "%a: Located app device path\n", __FUNCTION__));
      Status     = FileHandleClose (FileHandle2);
      *Fs_Handle = (EFI_FILE *)FileHandle;
      break;
    }
  }

CleanUp:
  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  return Status;
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
  UNIT_TEST_FRAMEWORK_HANDLE     Fw       = NULL;
  UNIT_TEST_SUITE_HANDLE         Misc     = NULL;
  BOOLEAN                        RunTests = TRUE;
  EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParams;
  EFI_FILE                       *Fs_Handle;

  DEBUG ((DEBUG_ERROR, "%a()\n", __FUNCTION__));

  DEBUG ((DEBUG_ERROR, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

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
    RunTests = FALSE;
    if (StrnCmp (ShellParams->Argv[1], L"-r", 4) == 0) {
      RunTests = TRUE;
    } else if (StrnCmp (ShellParams->Argv[1], L"-d", 4) == 0) {
      Status = OpenAppSFS (&Fs_Handle);

      if (!EFI_ERROR ((Status))) {
        DumpPagingInfo (Fs_Handle);
      } else {
        DumpPagingInfo (NULL);
      }
    } else {
      if (StrnCmp (ShellParams->Argv[1], L"-h", 4) != 0) {
        DEBUG ((DEBUG_INFO, "Invalid argument!\n"));
      }

      DEBUG ((DEBUG_INFO, "-h : Print available flags\n"));
      DEBUG ((DEBUG_INFO, "-d : Dump the page table files to the EFI partition\n"));
      DEBUG ((DEBUG_INFO, "-r : Run the application tests\n"));
      DEBUG ((DEBUG_INFO, "NOTE: Combined flags (i.e. -rd) is not supported\n"));
    }
  }

  if (RunTests) {
    //
    // Start setting up the test framework for running the tests.
    //
    Status = InitUnitTestFramework (&Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
      goto EXIT;
    }

    //
    // Create test suite
    //
    CreateUnitTestSuite (&Misc, Fw, "Miscellaneous tests", "Security.Misc", NULL, NULL);

    if (Misc == NULL) {
      DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for TestSuite\n"));
      goto EXIT;
    }

    GetSpecialRegions ();
    GetNonProtectedImageList ();
    Status = gDS->GetMemorySpaceMap (&mMemorySpaceMapCount, &mMemorySpaceMap);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Unable to fetch the GCD memory map. Test results may be inaccurate. Status: %r\n", __FUNCTION__, Status));
    }

    AddTestCase (Misc, "No pages can be read,write,execute", "Security.Misc.NoReadWriteExecute", NoReadWriteExecute, PopulatePageTableMap, FreePageTableMap, NULL);

    //
    // Execute the tests.
    //
    Status = RunAllTestSuites (Fw);
EXIT:

    if (Fw) {
      FreeUnitTestFramework (Fw);
    }
  }

  return EFI_SUCCESS;
}   // DxePagingAuditTestAppEntryPoint()
