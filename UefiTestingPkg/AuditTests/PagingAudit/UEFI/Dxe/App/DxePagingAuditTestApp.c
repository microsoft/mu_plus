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
#include <Protocol/MemoryAttribute.h>

#include <Library/FileHandleLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/FlatPageTableLib.h>
#include <Library/UnitTestLib.h>
#include <Library/HobLib.h>
#include <Library/SafeIntLib.h>

#define UNIT_TEST_APP_NAME     "Paging Audit Test"
#define UNIT_TEST_APP_VERSION  "2"
#define MAX_CHARS_TO_READ      4

// TRUE if A interval subsumes B interval
#define CHECK_SUBSUMPTION(AStart, AEnd, BStart, BEnd) \
  ((AStart <= BStart) && (AEnd >= BEnd))

// TRUE if A and B have overlapping intervals
#define CHECK_OVERLAP(AStart, AEnd, BStart, BEnd)   \
  ((AEnd > AStart) && (BEnd > BStart) &&            \
  ((AStart <= BStart && AEnd > BStart) ||           \
  (BStart <= AStart && BEnd > AStart)))

// Aligns the input address down to the nearest page boundary
#define ALIGN_ADDRESS(Address)  ((Address / EFI_PAGE_SIZE) * EFI_PAGE_SIZE)

// Globals required to create the memory info database
CHAR8  *mMemoryInfoDatabaseBuffer   = NULL;
UINTN  mMemoryInfoDatabaseSize      = 0;
UINTN  mMemoryInfoDatabaseAllocSize = 0;

// Globals for memory protection special regions
MEMORY_PROTECTION_SPECIAL_REGION  *mSpecialRegions    = NULL;
UINTN                             mSpecialRegionCount = 0;

// Global for the non-protected image list
IMAGE_RANGE_DESCRIPTOR  *mNonProtectedImageList = NULL;

// Globals for the memory space map
EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *mMemorySpaceMap     = NULL;
UINTN                            mMemorySpaceMapCount = 0;

// Globals for the EFI memory map
UINTN                  mEfiMemoryMapSize           = 0;
EFI_MEMORY_DESCRIPTOR  *mEfiMemoryMap              = NULL;
UINTN                  mEfiMemoryMapDescriptorSize = 0;

// Global for the flat page table
PAGE_MAP  mMap = { 0 };

// -------------------------------------------------
//    GLOBALS SUPPORT FUNCTIONS
// -------------------------------------------------

/**
  Return if the PE image section is aligned. This function must
  only be called using a loaded image's code type or EfiReservedMemoryType.
  Calling with a different type will ASSERT.

  @param[in]  SectionAlignment    PE/COFF section alignment
  @param[in]  MemoryType          PE/COFF image memory type

  @retval TRUE  The PE image section is aligned.
  @retval FALSE The PE image section is not aligned.
**/
BOOLEAN
IsLoadedImageSectionAligned (
  IN UINT32           SectionAlignment,
  IN EFI_MEMORY_TYPE  MemoryType
  )
{
  UINT32  PageAlignment;

  switch (MemoryType) {
    case EfiRuntimeServicesCode:
    case EfiACPIMemoryNVS:
      PageAlignment = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
      break;
    case EfiRuntimeServicesData:
    case EfiACPIReclaimMemory:
      ASSERT (FALSE);
      PageAlignment = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
      break;
    case EfiBootServicesCode:
    case EfiLoaderCode:
    case EfiReservedMemoryType:
      PageAlignment = EFI_PAGE_SIZE;
      break;
    default:
      ASSERT (FALSE);
      PageAlignment = EFI_PAGE_SIZE;
      break;
  }

  if ((SectionAlignment & (PageAlignment - 1)) != 0) {
    return FALSE;
  } else {
    return TRUE;
  }
}

/**
  Frees the entries in the mMap global.
**/
STATIC
VOID
FreePageTableMap (
  VOID
  )
{
  if (mMap.Entries != NULL) {
    FreePages (mMap.Entries, mMap.EntryPagesAllocated);
    mMap.Entries = NULL;
  }

  mMap.ArchSignature       = 0;
  mMap.EntryCount          = 0;
  mMap.EntryPagesAllocated = 0;
}

/**
  Populate the global flat page table map.

  @retval EFI_SUCCESS           The page table is parsed successfully.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for the page table map.
  @retval EFI_INVALID_PARAMETER An error occurred while parsing the page table.
**/
STATIC
EFI_STATUS
PopulatePageTableMap (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mMap.Entries != NULL) {
    return EFI_SUCCESS;
  }

  Status = CreateFlatPageTable (&mMap);

  while (Status == EFI_BUFFER_TOO_SMALL) {
    if ((mMap.Entries != NULL) && (mMap.EntryPagesAllocated > 0)) {
      FreePages (mMap.Entries, mMap.EntryPagesAllocated);
      mMap.Entries = NULL;
    }

    mMap.EntryPagesAllocated = EFI_SIZE_TO_PAGES (mMap.EntryCount * sizeof (PAGE_MAP_ENTRY));
    mMap.Entries             = AllocatePages (mMap.EntryPagesAllocated);

    if (mMap.Entries == NULL) {
      UT_LOG_ERROR ("Failed to allocate %d pages for page table map!\n", mMap.EntryPagesAllocated);
      return EFI_OUT_OF_RESOURCES;
    }

    Status = CreateFlatPageTable (&mMap);
  }

  if ((Status != EFI_SUCCESS) && (mMap.Entries != NULL)) {
    FreePageTableMap ();
  }

  return Status;
}

/**
  Frees the mNonProtectedImageList global
**/
STATIC
VOID
FreeNonProtectedImageList (
  VOID
  )
{
  LIST_ENTRY              *ImageRecordLink;
  IMAGE_RANGE_DESCRIPTOR  *CurrentImageRangeDescriptor;

  if (mNonProtectedImageList != NULL) {
    ImageRecordLink = &mNonProtectedImageList->Link;
    while (!IsListEmpty (ImageRecordLink)) {
      CurrentImageRangeDescriptor = CR (
                                      ImageRecordLink->ForwardLink,
                                      IMAGE_RANGE_DESCRIPTOR,
                                      Link,
                                      IMAGE_RANGE_DESCRIPTOR_SIGNATURE
                                      );

      RemoveEntryList (&CurrentImageRangeDescriptor->Link);
      FreePool (CurrentImageRangeDescriptor);
    }

    FreePool (mNonProtectedImageList);
    mNonProtectedImageList = NULL;
  }
}

/**
  Populates the mNonProtectedImageList global

  @retval EFI_SUCCESS   The non-protected image list is populated successfully.
  @retval other         An error occurred while populating the non-protected image list.
**/
STATIC
EFI_STATUS
PopulateNonProtectedImageList (
  VOID
  )
{
  EFI_STATUS                        Status;
  MEMORY_PROTECTION_DEBUG_PROTOCOL  *MemoryProtectionProtocol;

  MemoryProtectionProtocol = NULL;

  if (mNonProtectedImageList != NULL) {
    return EFI_SUCCESS;
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

  return Status;
}

/**
  Frees the mSpecialRegions global
**/
STATIC
VOID
FreeSpecialRegions (
  VOID
  )
{
  if (mSpecialRegions != NULL) {
    FreePool (mSpecialRegions);
    mSpecialRegions = NULL;
  }

  mSpecialRegionCount = 0;
}

/**
  Populates the special region array global

  @retval EFI_SUCCESS   The special region array is populated successfully.
  @retval other         An error occurred while populating the special region array.
**/
STATIC
EFI_STATUS
PopulateSpecialRegions (
  VOID
  )
{
  EFI_STATUS                                 Status;
  MEMORY_PROTECTION_SPECIAL_REGION_PROTOCOL  *SpecialRegionProtocol;

  SpecialRegionProtocol = NULL;

  if (mSpecialRegions != NULL) {
    return EFI_SUCCESS;
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
    mSpecialRegions = NULL;
  }

  return Status;
}

/**
  Frees the mMemorySpaceMap global
**/
STATIC
VOID
FreeMemorySpaceMap (
  VOID
  )
{
  if (mMemorySpaceMap != NULL) {
    FreePool (mMemorySpaceMap);
    mMemorySpaceMap = NULL;
  }

  mMemorySpaceMapCount = 0;
}

/**
  Populates the memory space map global

  @retval EFI_SUCCESS   The memory space map is populated successfully.
  @retval other         An error occurred while populating the memory space map.
**/
STATIC
EFI_STATUS
PopulateMemorySpaceMap (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mMemorySpaceMap != NULL) {
    return EFI_SUCCESS;
  }

  Status = gDS->GetMemorySpaceMap (&mMemorySpaceMapCount, &mMemorySpaceMap);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%d - Unable to fetch memory space map\n", __FUNCTION__, __LINE__));
    mMemorySpaceMap = NULL;
  }

  SortMemorySpaceMap (mMemorySpaceMap, mMemorySpaceMapCount, sizeof (EFI_GCD_MEMORY_SPACE_DESCRIPTOR));

  return Status;
}

/**
  Frees the memory space map global
**/
STATIC
VOID
FreeEfiMemoryMap (
  VOID
  )
{
  if (mEfiMemoryMap != NULL) {
    FreePool (mEfiMemoryMap);
    mEfiMemoryMap = NULL;
  }

  mEfiMemoryMapSize           = 0;
  mEfiMemoryMapDescriptorSize = 0;
}

/**
  Populates the memory space map global

  @retval EFI_SUCCESS   The memory space map is populated successfully.
  @retval other         An error occurred while populating the memory space map.
**/
STATIC
EFI_STATUS
PopulateEfiMemoryMap (
  VOID
  )
{
  UINTN       EfiMapKey;
  UINT32      EfiDescriptorVersion;
  EFI_STATUS  Status;

  // Get the EFI memory map.
  mEfiMemoryMapSize           = 0;
  mEfiMemoryMap               = NULL;
  mEfiMemoryMapDescriptorSize = 0;

  Status = gBS->GetMemoryMap (
                  &mEfiMemoryMapSize,
                  mEfiMemoryMap,
                  &EfiMapKey,
                  &mEfiMemoryMapDescriptorSize,
                  &EfiDescriptorVersion
                  );

  // Loop to allocate space for the memory map and then copy it in.
  do {
    mEfiMemoryMap = (EFI_MEMORY_DESCRIPTOR *)AllocateZeroPool (mEfiMemoryMapSize);
    if (mEfiMemoryMap == NULL) {
      ASSERT (mEfiMemoryMap != NULL);
      DEBUG ((DEBUG_ERROR, "%a - Unable to allocate memory for the EFI memory map.\n", __FUNCTION__));
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gBS->GetMemoryMap (
                    &mEfiMemoryMapSize,
                    mEfiMemoryMap,
                    &EfiMapKey,
                    &mEfiMemoryMapDescriptorSize,
                    &EfiDescriptorVersion
                    );
    if (EFI_ERROR (Status)) {
      FreePool (mEfiMemoryMap);
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);

  SortMemoryMap (mEfiMemoryMap, mEfiMemoryMapSize, mEfiMemoryMapDescriptorSize);

  return Status;
}

/**
  Checks the input flat page/translation table for the input region and converts the associated
  table entries to EFI access attributes.

  @param[in]  Map                 Pointer to the PAGE_MAP struct to be parsed
  @param[in]  Address             Start address of the region
  @param[in]  Length              Length of the region
  @param[out] Attributes          EFI Attributes of the region

  @retval EFI_SUCCESS             The output Attributes is valid
  @retval EFI_INVALID_PARAMETER   The flat translation table has not been built or
                                  Attributes was NULL or Length was 0
  @retval EFI_NOT_FOUND           The input region could not be found.
**/
EFI_STATUS
EFIAPI
GetRegionCommonAccessAttributes (
  IN PAGE_MAP  *Map,
  IN UINT64    Address,
  IN UINT64    Length,
  OUT UINT64   *Attributes
  )
{
  UINTN    Index;
  UINT64   EntryStartAddress;
  UINT64   EntryEndAddress;
  UINT64   InputEndAddress;
  BOOLEAN  FoundRange;

  if ((Map->Entries == NULL) || (Map->EntryCount == 0) || (Attributes == NULL) || (Length == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  FoundRange      = FALSE;
  Index           = 0;
  InputEndAddress = 0;

  if (EFI_ERROR (SafeUint64Add (Address, Length - 1, &InputEndAddress))) {
    return EFI_INVALID_PARAMETER;
  }

  do {
    EntryStartAddress = Map->Entries[Index].LinearAddress;
    if (EFI_ERROR (
          SafeUint64Add (
            Map->Entries[Index].LinearAddress,
            Map->Entries[Index].Length - 1,
            &EntryEndAddress
            )
          ))
    {
      return EFI_ABORTED;
    }

    if (CHECK_OVERLAP (Address, InputEndAddress, EntryStartAddress, EntryEndAddress)) {
      if (!FoundRange) {
        *Attributes = EFI_MEMORY_ACCESS_MASK;
        FoundRange  = TRUE;
      }

      if (IsPageExecutable (Map->Entries[Index].PageEntry)) {
        *Attributes &= ~EFI_MEMORY_XP;
      }

      if (IsPageWritable (Map->Entries[Index].PageEntry)) {
        *Attributes &= ~EFI_MEMORY_RO;
      }

      if (IsPageReadable (Map->Entries[Index].PageEntry)) {
        *Attributes &= ~EFI_MEMORY_RP;
      }

      Address = EntryEndAddress + 1;
    }

    if (EntryEndAddress >= InputEndAddress) {
      break;
    }
  } while (++Index < Map->EntryCount);

  return FoundRange ? EFI_SUCCESS : EFI_NOT_FOUND;
}

// ----------------------
//    CLEANUP FUNCTION
// ----------------------

/**
  Cleanup function for the unit test.

  @param[in] Context        Unit test context

  @retval UNIT_TEST_PASSED  Always passes
**/
STATIC
VOID
EFIAPI
GeneralTestCleanup (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  if (mMap.Entries != NULL) {
    FreePageTableMap ();
  }

  if (mSpecialRegions != NULL) {
    FreeSpecialRegions ();
  }

  if (mNonProtectedImageList != NULL) {
    FreeNonProtectedImageList ();
  }

  if (mMemorySpaceMap != NULL) {
    FreeMemorySpaceMap ();
  }

  if (mEfiMemoryMap != NULL) {
    FreeEfiMemoryMap ();
  }
}

// ---------------------------------
//    UNIT TEST SUPPORT FUNCTIONS
// ---------------------------------

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

// -------------------------
//    UNIT TEST FUNCTIONS
// -------------------------

/**
  Checks if the page/translation table has any read/write/execute
  regions.

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
  BOOLEAN  TestFailure;

  DEBUG ((DEBUG_INFO, "%a Enter...\n", __FUNCTION__));

  PopulateSpecialRegions ();
  PopulateNonProtectedImageList ();
  UT_ASSERT_NOT_EFI_ERROR (PopulateMemorySpaceMap ());
  UT_ASSERT_NOT_NULL (mMemorySpaceMap);
  UT_ASSERT_NOT_EFI_ERROR (PopulatePageTableMap ());
  UT_ASSERT_NOT_NULL (mMap.Entries);

  Index       = 0;
  TestFailure = FALSE;

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
        TestFailure = TRUE;
      }
    }
  }

  UT_ASSERT_FALSE (TestFailure);

  return UNIT_TEST_PASSED;
}

/**
  Checks that EfiConventionalMemory is EFI_MEMORY_RP or
  is not mapped.

  @param[in] Context            Unit test context

  @retval UNIT_TEST_PASSED      The unit test passed
  @retval other                 The unit test failed
**/
UNIT_TEST_STATUS
EFIAPI
UnallocatedMemoryIsRP (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN                TestFailure;
  EFI_MEMORY_DESCRIPTOR  *EfiMemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *EfiMemoryMapEnd;
  UINTN                  Attributes;
  EFI_STATUS             Status;

  DEBUG ((DEBUG_INFO, "%a Enter...\n", __FUNCTION__));

  UT_ASSERT_NOT_EFI_ERROR (PopulateEfiMemoryMap ());
  UT_ASSERT_NOT_NULL (mEfiMemoryMap);
  UT_ASSERT_NOT_EFI_ERROR (PopulatePageTableMap ());
  UT_ASSERT_NOT_NULL (mMap.Entries);

  TestFailure = FALSE;

  EfiMemoryMapEntry = mEfiMemoryMap;
  EfiMemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mEfiMemoryMap + mEfiMemoryMapSize);

  while (EfiMemoryMapEntry < EfiMemoryMapEnd) {
    if (EfiMemoryMapEntry->Type == EfiConventionalMemory) {
      Attributes = 0;
      Status     = GetRegionCommonAccessAttributes (
                     &mMap,
                     EfiMemoryMapEntry->PhysicalStart,
                     EfiMemoryMapEntry->NumberOfPages * EFI_PAGE_SIZE,
                     &Attributes
                     );
      if (Status != EFI_NOT_FOUND) {
        if (EFI_ERROR (Status)) {
          UT_LOG_ERROR (
            "Failed to get attributes for range 0x%llx - 0x%llx\n",
            EfiMemoryMapEntry->PhysicalStart,
            EfiMemoryMapEntry->PhysicalStart + (EfiMemoryMapEntry->NumberOfPages * EFI_PAGE_SIZE)
            );
          TestFailure = TRUE;
        } else if ((Attributes & EFI_MEMORY_RP) == 0) {
          UT_LOG_ERROR (
            "Memory Range 0x%llx-0x%llx is not EFI_MEMORY_RP\n",
            EfiMemoryMapEntry->PhysicalStart,
            EfiMemoryMapEntry->PhysicalStart + (EfiMemoryMapEntry->NumberOfPages * EFI_PAGE_SIZE)
            );
          TestFailure = TRUE;
        }
      }
    }

    EfiMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (EfiMemoryMapEntry, mEfiMemoryMapDescriptorSize);
  }

  UT_ASSERT_FALSE (TestFailure);

  return UNIT_TEST_PASSED;
}

/**
  Checks if the EFI Memory Attribute Protocol is Present.

  @param[in] Context            Unit test context

  @retval UNIT_TEST_PASSED      The unit test passed
  @retval other                 The unit test failed
**/
UNIT_TEST_STATUS
EFIAPI
IsMemoryAttributeProtocolPresent (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                     Status;
  EFI_MEMORY_ATTRIBUTE_PROTOCOL  *MemoryAttribute;

  DEBUG ((DEBUG_INFO, "%a Enter...\n", __FUNCTION__));

  Status = gBS->LocateProtocol (
                  &gEfiMemoryAttributeProtocolGuid,
                  NULL,
                  (VOID **)&MemoryAttribute
                  );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Allocates Pages and Pools of each memory type and
  checks that the returned buffers have restrictive
  access attributes.

  @param[in] Context            Unit test context

  @retval UNIT_TEST_PASSED      The unit test passed
  @retval other                 The unit test failed
**/
UNIT_TEST_STATUS
EFIAPI
AllocatedPagesAndPoolsAreProtected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT64      Attributes;
  UINTN       Index;
  EFI_STATUS  Status;
  BOOLEAN     TestFailure;
  UINTN       *PageAllocations[EfiMaxMemoryType];
  UINTN       *PoolAllocations[EfiMaxMemoryType];

  DEBUG ((DEBUG_INFO, "%a Enter...\n", __FUNCTION__));

  TestFailure = FALSE;
  ZeroMem (PageAllocations, sizeof (PageAllocations));
  ZeroMem (PoolAllocations, sizeof (PoolAllocations));

  for (Index = 0; Index < EfiMaxMemoryType; Index++) {
    if ((Index != EfiConventionalMemory) && (Index != EfiPersistentMemory)) {
      PageAllocations[Index] = AllocatePages (1);
      if (PageAllocations[Index] == NULL) {
        UT_LOG_ERROR ("Failed to allocate one page for memory type %d\n", Index);
        TestFailure = TRUE;
      }

      PoolAllocations[Index] = AllocatePool (8);
      if (PoolAllocations[Index] == NULL) {
        UT_LOG_ERROR ("Failed to allocate an 8 byte pool for memory type %d\n", Index);
        TestFailure = TRUE;
      }
    }
  }

  Status = PopulatePageTableMap ();

  if (!EFI_ERROR (Status) && (mMap.Entries != NULL)) {
    for (Index = 0; Index < EfiMaxMemoryType; Index++) {
      if ((Index != EfiConventionalMemory) && (Index != EfiPersistentMemory)) {
        Attributes = 0;
        Status     = GetRegionCommonAccessAttributes (
                       &mMap,
                       (UINT64)PageAllocations[Index],
                       EFI_PAGE_SIZE,
                       &Attributes
                       );
        if (Status != EFI_NOT_FOUND) {
          if (EFI_ERROR (Status)) {
            UT_LOG_ERROR (
              "Failed to get attributes for range 0x%llx - 0x%llx\n",
              PageAllocations[Index],
              PageAllocations[Index] + EFI_PAGE_SIZE
              );
            TestFailure = TRUE;
          }

          if (Attributes == 0) {
            UT_LOG_ERROR (
              "Page range 0x%llx - 0x%llx has no restrictive access attributes\n",
              PageAllocations[Index],
              PageAllocations[Index] + EFI_PAGE_SIZE
              );
            TestFailure = TRUE;
          }
        }

        Attributes = 0;
        Status     = GetRegionCommonAccessAttributes (
                       &mMap,
                       ALIGN_ADDRESS ((UINTN)PoolAllocations[Index]),
                       EFI_PAGE_SIZE,
                       &Attributes
                       );
        if ((Status != EFI_NOT_FOUND)) {
          if (EFI_ERROR (Status)) {
            UT_LOG_ERROR (
              "Failed to get attributes for range 0x%llx - 0x%llx\n",
              ALIGN_ADDRESS ((UINTN)PoolAllocations[Index]),
              ALIGN_ADDRESS ((UINTN)PoolAllocations[Index]) + EFI_PAGE_SIZE
              );
            TestFailure = TRUE;
          } else if (Attributes == 0) {
            UT_LOG_ERROR (
              "Pool range 0x%llx - 0x%llx has no restrictive access attributes\n",
              PoolAllocations[Index],
              PoolAllocations[Index] + sizeof (UINT64)
              );
            TestFailure = TRUE;
          }
        }
      }
    }
  } else {
    UT_LOG_ERROR ("Failed to populate page table map\n");
    TestFailure = TRUE;
  }

  for (Index = 0; Index < EfiMaxMemoryType; Index++) {
    if (PageAllocations[Index] != NULL) {
      FreePages (PageAllocations[Index], 1);
    }

    if (PoolAllocations[Index] != NULL) {
      FreePool (PoolAllocations[Index]);
    }
  }

  UT_ASSERT_FALSE (TestFailure);

  return UNIT_TEST_PASSED;
}

/**
  Checks that the NULL page is not mapped or is
  EFI_MEMORY_RP.

  @param[in] Context            Unit test context

  @retval UNIT_TEST_PASSED      The unit test passed
  @retval other                 The unit test failed
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
NullCheck (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT64      Attributes;
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a Enter...\n", __FUNCTION__));

  UT_ASSERT_NOT_EFI_ERROR (PopulatePageTableMap ());
  UT_ASSERT_NOT_NULL (mMap.Entries);

  Attributes = 0;
  Status     = GetRegionCommonAccessAttributes (&mMap, 0, EFI_PAGE_SIZE, &Attributes);

  if ((Status != EFI_NOT_FOUND)) {
    if (EFI_ERROR (Status)) {
      UT_ASSERT_NOT_EFI_ERROR (Status);
    } else {
      UT_ASSERT_NOT_EQUAL (Attributes & EFI_MEMORY_RP, 0);
    }
  }

  return UNIT_TEST_PASSED;
}

/**
  Checks that MMIO regions in the EFI memory map are
  EFI_MEMORY_XP.

  @param[in] Context            Unit test context

  @retval UNIT_TEST_PASSED      The unit test passed
  @retval other                 The unit test failed
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
MmioIsXp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_MEMORY_DESCRIPTOR  *EfiMemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *EfiMemoryMapEnd;
  UINT64                 Attributes;
  BOOLEAN                TestFailure;
  EFI_STATUS             Status;
  UINTN                  Index;

  DEBUG ((DEBUG_INFO, "%a Enter...\n", __FUNCTION__));

  UT_ASSERT_NOT_EFI_ERROR (PopulateEfiMemoryMap ());
  UT_ASSERT_NOT_NULL (mEfiMemoryMap);
  UT_ASSERT_NOT_EFI_ERROR (PopulateMemorySpaceMap ());
  UT_ASSERT_NOT_NULL (mMemorySpaceMap);
  UT_ASSERT_NOT_EFI_ERROR (PopulatePageTableMap ());
  UT_ASSERT_NOT_NULL (mMap.Entries);

  TestFailure = FALSE;

  EfiMemoryMapEntry = mEfiMemoryMap;
  EfiMemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mEfiMemoryMap + mEfiMemoryMapSize);

  while (EfiMemoryMapEntry < EfiMemoryMapEnd) {
    if (EfiMemoryMapEntry->Type == EfiMemoryMappedIO) {
      Attributes = 0;
      Status     = GetRegionCommonAccessAttributes (
                     &mMap,
                     EfiMemoryMapEntry->PhysicalStart,
                     EfiMemoryMapEntry->NumberOfPages * EFI_PAGE_SIZE,
                     &Attributes
                     );

      if (Status != EFI_NOT_FOUND) {
        if (EFI_ERROR (Status)) {
          UT_LOG_ERROR (
            "Failed to get attributes for range 0x%llx - 0x%llx\n",
            EfiMemoryMapEntry->PhysicalStart,
            EfiMemoryMapEntry->PhysicalStart + (EfiMemoryMapEntry->NumberOfPages * EFI_PAGE_SIZE)
            );
          TestFailure = TRUE;
        } else if ((Attributes & EFI_MEMORY_XP) == 0) {
          UT_LOG_ERROR (
            "Memory Range 0x%llx-0x%llx is not EFI_MEMORY_XP\n",
            EfiMemoryMapEntry->PhysicalStart,
            EfiMemoryMapEntry->PhysicalStart + (EfiMemoryMapEntry->NumberOfPages * EFI_PAGE_SIZE)
            );
          TestFailure = TRUE;
        }
      }
    }

    EfiMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (EfiMemoryMapEntry, mEfiMemoryMapDescriptorSize);
  }

  for (Index = 0; Index < mMemorySpaceMapCount; Index++) {
    if (mMemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeMemoryMappedIo) {
      Attributes = 0;
      Status     = GetRegionCommonAccessAttributes (
                     &mMap,
                     mMemorySpaceMap[Index].BaseAddress,
                     mMemorySpaceMap[Index].Length,
                     &Attributes
                     );

      if (Status != EFI_NOT_FOUND) {
        if (EFI_ERROR (Status)) {
          UT_LOG_ERROR (
            "Failed to get attributes for range 0x%llx - 0x%llx\n",
            mMemorySpaceMap[Index].BaseAddress,
            mMemorySpaceMap[Index].BaseAddress + mMemorySpaceMap[Index].Length
            );
          TestFailure = TRUE;
        } else if ((Attributes & EFI_MEMORY_XP) == 0) {
          UT_LOG_ERROR (
            "Memory Range 0x%llx-0x%llx is not EFI_MEMORY_XP\n",
            mMemorySpaceMap[Index].BaseAddress,
            mMemorySpaceMap[Index].BaseAddress + mMemorySpaceMap[Index].Length
            );
          TestFailure = TRUE;
        }
      }
    }
  }

  UT_ASSERT_FALSE (TestFailure);

  return UNIT_TEST_PASSED;
}

/**
  Checks that loaded image sections containing code
  are EFI_MEMORY_RO and sections containing data
  are EFI_MEMORY_XP.

  @param[in] Context            Unit test context

  @retval UNIT_TEST_PASSED      The unit test passed
  @retval other                 The unit test failed
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
ImageCodeSectionsRoDataSectionsXp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                           Status;
  UINTN                                NoHandles;
  EFI_HANDLE                           *HandleBuffer;
  UINTN                                Index;
  UINTN                                Index2;
  EFI_LOADED_IMAGE_PROTOCOL            *LoadedImage;
  EFI_IMAGE_DOS_HEADER                 *DosHdr;
  UINT32                               PeCoffHeaderOffset;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr;
  UINT32                               SectionAlignment;
  EFI_IMAGE_SECTION_HEADER             *Section;
  UINT64                               Attributes;
  BOOLEAN                              TestFailure;
  UINT64                               SectionStart;
  UINT64                               SectionEnd;
  CHAR8                                *PdbFileName;

  DEBUG ((DEBUG_INFO, "%a Enter...\n", __FUNCTION__));

  UT_ASSERT_NOT_EFI_ERROR (PopulatePageTableMap ());
  UT_ASSERT_NOT_NULL (mMap.Entries);

  TestFailure = FALSE;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiLoadedImageProtocolGuid,
                  NULL,
                  &NoHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status) && (NoHandles == 0)) {
    UT_LOG_ERROR ("Unable to query EFI Loaded Image Protocol\n");
    UT_ASSERT_NOT_EFI_ERROR (Status);
    UT_ASSERT_NOT_EQUAL (NoHandles, 0);
  }

  for (Index = 0; Index < NoHandles; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **)&LoadedImage
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    // Check PE/COFF image
    PdbFileName = PeCoffLoaderGetPdbPointer (LoadedImage->ImageBase);
    if (PdbFileName == NULL) {
      DEBUG ((
        DEBUG_WARN,
        "%a Could not get name of image loaded at 0x%llx - 0x%llx...\n",
        __func__,
        (UINTN)LoadedImage->ImageBase,
        (UINTN)LoadedImage->ImageBase + LoadedImage->ImageSize
        ));
    }

    DosHdr             = (EFI_IMAGE_DOS_HEADER *)(UINTN)LoadedImage->ImageBase;
    PeCoffHeaderOffset = 0;
    if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
      PeCoffHeaderOffset = DosHdr->e_lfanew;
    }

    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINT8 *)(UINTN)LoadedImage->ImageBase + PeCoffHeaderOffset);

    // Get SectionAlignment
    if (Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      SectionAlignment = Hdr.Pe32->OptionalHeader.SectionAlignment;
    } else {
      SectionAlignment = Hdr.Pe32Plus->OptionalHeader.SectionAlignment;
    }

    if (!IsLoadedImageSectionAligned (SectionAlignment, LoadedImage->ImageCodeType)) {
      UT_LOG_ERROR (
        "Image %a: 0x%llx - 0x%llx is not aligned\n",
        PdbFileName,
        (UINTN)LoadedImage->ImageBase,
        (UINTN)LoadedImage->ImageBase + LoadedImage->ImageSize
        );
      TestFailure = TRUE;
      continue;
    }

    Section = (EFI_IMAGE_SECTION_HEADER *)(
                                           (UINT8 *)(UINTN)LoadedImage->ImageBase +
                                           PeCoffHeaderOffset +
                                           sizeof (UINT32) +
                                           sizeof (EFI_IMAGE_FILE_HEADER) +
                                           Hdr.Pe32->FileHeader.SizeOfOptionalHeader
                                           );

    for (Index2 = 0; Index2 < Hdr.Pe32->FileHeader.NumberOfSections; Index2++) {
      Attributes   = 0;
      SectionStart = (UINT64)LoadedImage->ImageBase + Section[Index2].VirtualAddress;
      SectionEnd   = SectionStart + ALIGN_VALUE (Section[Index2].SizeOfRawData, SectionAlignment);

      // Check if the section contains code and data
      if (((Section[Index2].Characteristics & EFI_IMAGE_SCN_CNT_CODE) != 0) &&
          ((Section[Index2].Characteristics &
            (EFI_IMAGE_SCN_CNT_INITIALIZED_DATA || EFI_IMAGE_SCN_CNT_UNINITIALIZED_DATA)) != 0))
      {
        UT_LOG_ERROR (
          "Image %a: Section 0x%llx-0x%llx contains code and data\n",
          PdbFileName,
          SectionStart,
          SectionEnd
          );
        TestFailure = TRUE;
      }

      Status = GetRegionCommonAccessAttributes (
                 &mMap,
                 SectionStart,
                 SectionEnd - SectionStart,
                 &Attributes
                 );

      if (EFI_ERROR (Status)) {
        TestFailure = TRUE;
        UT_LOG_ERROR (
          "Failed to get attributes for memory range 0x%llx-0x%llx\n",
          SectionStart,
          SectionEnd
          );
      } else if ((Section[Index2].Characteristics &
                  (EFI_IMAGE_SCN_MEM_WRITE | EFI_IMAGE_SCN_MEM_EXECUTE)) == EFI_IMAGE_SCN_MEM_EXECUTE)
      {
        if ((Attributes & EFI_MEMORY_RO) == 0) {
          UT_LOG_ERROR (
            "Image %a: Section 0x%llx-0x%llx is not EFI_MEMORY_RO\n",
            PdbFileName,
            SectionStart,
            SectionEnd
            );
          TestFailure = TRUE;
        }
      } else {
        if ((Attributes & EFI_MEMORY_XP) == 0) {
          UT_LOG_ERROR (
            "Image %a: Section 0x%llx-0x%llx is not EFI_MEMORY_XP\n",
            PdbFileName,
            SectionStart,
            SectionEnd
            );
          TestFailure = TRUE;
        }
      }
    }
  }

  FreePool (HandleBuffer);

  UT_ASSERT_FALSE (TestFailure);

  return UNIT_TEST_PASSED;
}

/**
  Checks that the stack is EFI_MEMORY_XP and has
  an EFI_MEMORY_RP page to catch overflow.

  @param[in] Context            Unit test context

  @retval UNIT_TEST_PASSED      The unit test passed
  @retval other                 The unit test failed
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
BspStackIsXpAndHasGuardPage (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryHob;
  BOOLEAN                    TestFailure;
  EFI_PHYSICAL_ADDRESS       StackBase;
  UINT64                     StackLength;
  UINT64                     Attributes;
  EFI_STATUS                 Status;

  DEBUG ((DEBUG_INFO, "%a Enter...\n", __FUNCTION__));

  UT_ASSERT_NOT_EFI_ERROR (PopulatePageTableMap ());
  UT_ASSERT_NOT_NULL (mMap.Entries);

  Hob.Raw     = GetHobList ();
  TestFailure = FALSE;

  while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw)) != NULL) {
    MemoryHob = Hob.MemoryAllocation;
    if (CompareGuid (&gEfiHobMemoryAllocStackGuid, &MemoryHob->AllocDescriptor.Name)) {
      StackBase   = (EFI_PHYSICAL_ADDRESS)((MemoryHob->AllocDescriptor.MemoryBaseAddress / EFI_PAGE_SIZE) * EFI_PAGE_SIZE);
      StackLength = (EFI_PHYSICAL_ADDRESS)(EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (MemoryHob->AllocDescriptor.MemoryLength)));

      UT_LOG_INFO ("BSP stack located at 0x%llx - 0x%llx\n", StackBase, StackBase + StackLength);

      Attributes = 0;
      Status     = GetRegionCommonAccessAttributes (
                     &mMap,
                     StackBase,
                     EFI_PAGE_SIZE,
                     &Attributes
                     );
      if ((Status != EFI_NOT_FOUND)) {
        if (EFI_ERROR (Status)) {
          UT_LOG_ERROR (
            "Failed to get attributes for memory range 0x%llx-0x%llx\n",
            StackBase,
            StackBase + EFI_PAGE_SIZE
            );
          TestFailure = TRUE;
        } else if (((Attributes & EFI_MEMORY_RP) == 0)) {
          UT_LOG_ERROR (
            "Stack 0x%llx-0x%llx does not have an EFI_MEMORY_RP page to catch overflow\n",
            StackBase,
            StackBase + EFI_PAGE_SIZE
            );
          TestFailure = TRUE;
        }
      }

      Attributes = 0;
      Status     = GetRegionCommonAccessAttributes (
                     &mMap,
                     StackBase + EFI_PAGE_SIZE,
                     StackLength - EFI_PAGE_SIZE,
                     &Attributes
                     );

      if (EFI_ERROR (Status)) {
        UT_LOG_ERROR (
          "Failed to get attributes for memory range 0x%llx-0x%llx\n",
          StackBase + EFI_PAGE_SIZE,
          StackBase + StackLength
          );
        TestFailure = TRUE;
      } else if ((Attributes & EFI_MEMORY_XP) == 0) {
        UT_LOG_ERROR (
          "Stack 0x%llx-0x%llx is executable\n",
          StackBase + EFI_PAGE_SIZE,
          StackBase + StackLength
          );
        TestFailure = TRUE;
      }

      break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  UT_ASSERT_FALSE (TestFailure);

  return UNIT_TEST_PASSED;
}

/**
  Checks that memory ranges not in the EFI
  memory map will cause a CPU fault if accessed.

  @param[in] Context            Unit test context

  @retval UNIT_TEST_PASSED      The unit test passed
  @retval other                 The unit test failed
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
MemoryOutsideEfiMemoryMapIsInaccessible (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT64                 StartOfAddressSpace;
  UINT64                 EndOfAddressSpace;
  EFI_MEMORY_DESCRIPTOR  *EndOfEfiMemoryMap;
  EFI_MEMORY_DESCRIPTOR  *CurrentEfiMemoryMapEntry;
  BOOLEAN                TestFailure;
  EFI_PHYSICAL_ADDRESS   LastMemoryMapEntryEnd;
  UINT64                 Attributes;
  EFI_STATUS             Status;

  DEBUG ((DEBUG_INFO, "%a Enter...\n", __FUNCTION__));

  UT_ASSERT_NOT_EFI_ERROR (PopulateEfiMemoryMap ());
  UT_ASSERT_NOT_NULL (mEfiMemoryMap);
  UT_ASSERT_NOT_EFI_ERROR (PopulateMemorySpaceMap ());
  UT_ASSERT_NOT_NULL (mMemorySpaceMap);
  UT_ASSERT_NOT_EFI_ERROR (PopulatePageTableMap ());
  UT_ASSERT_NOT_NULL (mMap.Entries);

  StartOfAddressSpace = mMemorySpaceMap[0].BaseAddress;
  EndOfAddressSpace   = mMemorySpaceMap[mMemorySpaceMapCount - 1].BaseAddress +
                        mMemorySpaceMap[mMemorySpaceMapCount - 1].Length;
  TestFailure              = FALSE;
  EndOfEfiMemoryMap        = (EFI_MEMORY_DESCRIPTOR *)(((UINT8 *)mEfiMemoryMap + mEfiMemoryMapSize));
  CurrentEfiMemoryMapEntry = mEfiMemoryMap;

  if (CurrentEfiMemoryMapEntry->PhysicalStart > StartOfAddressSpace) {
    Attributes = 0;
    Status     = GetRegionCommonAccessAttributes (
                   &mMap,
                   StartOfAddressSpace,
                   CurrentEfiMemoryMapEntry->PhysicalStart - StartOfAddressSpace,
                   &Attributes
                   );

    if ((Status != EFI_NOT_FOUND) && ((Attributes & EFI_MEMORY_RP) == 0)) {
      UT_LOG_ERROR (
        "Memory Range 0x%llx-0x%llx is not EFI_MEMORY_RP\n",
        StartOfAddressSpace,
        CurrentEfiMemoryMapEntry->PhysicalStart
        );
      TestFailure = TRUE;
    }
  }

  LastMemoryMapEntryEnd = CurrentEfiMemoryMapEntry->PhysicalStart +
                          (CurrentEfiMemoryMapEntry->NumberOfPages * EFI_PAGE_SIZE);
  CurrentEfiMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (CurrentEfiMemoryMapEntry, mEfiMemoryMapDescriptorSize);

  while ((UINTN)CurrentEfiMemoryMapEntry < (UINTN)EndOfEfiMemoryMap) {
    if (CurrentEfiMemoryMapEntry->PhysicalStart > LastMemoryMapEntryEnd) {
      Attributes = 0;
      Status     = GetRegionCommonAccessAttributes (
                     &mMap,
                     LastMemoryMapEntryEnd,
                     CurrentEfiMemoryMapEntry->PhysicalStart - LastMemoryMapEntryEnd,
                     &Attributes
                     );
      if ((Status != EFI_NOT_FOUND) && ((Attributes & EFI_MEMORY_RP) == 0)) {
        UT_LOG_ERROR (
          "Memory Range 0x%llx-0x%llx is not EFI_MEMORY_RP\n",
          LastMemoryMapEntryEnd,
          CurrentEfiMemoryMapEntry->PhysicalStart
          );
        TestFailure = TRUE;
      }
    }

    LastMemoryMapEntryEnd = CurrentEfiMemoryMapEntry->PhysicalStart +
                            (CurrentEfiMemoryMapEntry->NumberOfPages * EFI_PAGE_SIZE);
    CurrentEfiMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (CurrentEfiMemoryMapEntry, mEfiMemoryMapDescriptorSize);
  }

  if (LastMemoryMapEntryEnd < EndOfAddressSpace) {
    Attributes = 0;
    Status     = GetRegionCommonAccessAttributes (
                   &mMap,
                   LastMemoryMapEntryEnd,
                   EndOfAddressSpace - LastMemoryMapEntryEnd,
                   &Attributes
                   );
    if ((Status != EFI_NOT_FOUND) && ((Attributes & EFI_MEMORY_RP) == 0)) {
      UT_LOG_ERROR (
        "Memory Range 0x%llx-0x%llx is not EFI_MEMORY_RP\n",
        LastMemoryMapEntryEnd,
        EndOfAddressSpace
        );
      TestFailure = TRUE;
    }
  }

  UT_ASSERT_FALSE (TestFailure);

  return UNIT_TEST_PASSED;
}

/**
  Entry Point of the shell app.

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
    DEBUG ((DEBUG_ERROR, "%a Could not retrieve command line args!\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  if (ShellParams->Argc > 1) {
    RunTests = FALSE;
    if (StrnCmp (ShellParams->Argv[1], L"-r", MAX_CHARS_TO_READ) == 0) {
      RunTests = TRUE;
    } else if (StrnCmp (ShellParams->Argv[1], L"-d", MAX_CHARS_TO_READ) == 0) {
      Status = OpenAppSFS (&Fs_Handle);

      if (!EFI_ERROR ((Status))) {
        DumpPagingInfo (Fs_Handle);
      } else {
        DumpPagingInfo (NULL);
      }
    } else {
      if (StrnCmp (ShellParams->Argv[1], L"-h", MAX_CHARS_TO_READ) != 0) {
        DEBUG ((DEBUG_ERROR, "Invalid argument!\n"));
      }

      Print (L"-h : Print available flags\n");
      Print (L"-d : Dump the page table files\n");
      Print (L"-r : Run the application tests\n");
      Print (L"NOTE: Combined flags (i.e. -rd) is not supported\n");
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

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Unable to fetch the GCD memory map. Test results may be inaccurate. Status: %r\n", __FUNCTION__, Status));
    }

    AddTestCase (Misc, "No pages are  readable, writable, and executable", "Security.Misc.NoReadWriteExecute", NoReadWriteExecute, NULL, GeneralTestCleanup, NULL);
    AddTestCase (Misc, "Unallocated memory is EFI_MEMORY_RP", "Security.Misc.UnallocatedMemoryIsRP", UnallocatedMemoryIsRP, NULL, GeneralTestCleanup, NULL);
    AddTestCase (Misc, "Memory Attribute Protocol is present", "Security.Misc.IsMemoryAttributeProtocolPresent", IsMemoryAttributeProtocolPresent, NULL, NULL, NULL);
    AddTestCase (Misc, "Calls to allocate pages and pools return buffers with restrictive access attributes", "Security.Misc.AllocatedPagesAndPoolsAreProtected", AllocatedPagesAndPoolsAreProtected, NULL, GeneralTestCleanup, NULL);
    AddTestCase (Misc, "NULL page is EFI_MEMORY_RP", "Security.Misc.NullCheck", NullCheck, NULL, GeneralTestCleanup, NULL);
    AddTestCase (Misc, "MMIO Regions are EFI_MEMORY_XP", "Security.Misc.MmioIsXp", MmioIsXp, NULL, GeneralTestCleanup, NULL);
    AddTestCase (Misc, "Image code sections are EFI_MEMORY_RO and and data sections are EFI_MEMORY_XP", "Security.Misc.ImageCodeSectionsRoDataSectionsXp", ImageCodeSectionsRoDataSectionsXp, NULL, GeneralTestCleanup, NULL);
    AddTestCase (Misc, "BSP stack is EFI_MEMORY_XP and has EFI_MEMORY_RP guard page", "Security.Misc.BspStackIsXpAndHasGuardPage", BspStackIsXpAndHasGuardPage, NULL, GeneralTestCleanup, NULL);
    AddTestCase (Misc, "Memory outside of the EFI Memory Map is inaccessible", "Security.Misc.MemoryOutsideEfiMemoryMapIsInaccessible", MemoryOutsideEfiMemoryMapIsInaccessible, NULL, GeneralTestCleanup, NULL);

    //
    // Execute the tests.
    //
    Status = RunAllTestSuites (Fw);
  }

EXIT:
  if (Fw != NULL) {
    FreeUnitTestFramework (Fw);
  }

  return EFI_SUCCESS;
}
