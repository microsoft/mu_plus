/** @file -- MemmapAndMatTestApp.c
This application contains tests and utility functions for the MemoryMap and
UEFI Memory Attributes Table.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UnitTestLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Guid/MemoryAttributesTable.h>

#define UNIT_TEST_APP_NAME        "MemoryMap and MemoryAttributesTable Unit Test"
#define UNIT_TEST_APP_SHORT_NAME  "MemMap_and_MAT_Test"
#define UNIT_TEST_APP_VERSION     "1.0"

#define A_IS_BETWEEN_B_AND_C(A, B, C) \
  (((B) < (A)) && ((A) < (C)))

#define A_B_OVERLAPS_C_D(A, B, C, D) \
  (((B) >= (C)) && ((D) >= (A)))

typedef struct _MEM_MAP_META {
  UINTN    MapSize;
  UINTN    EntrySize;
  UINTN    EntryCount;
  VOID     *Map;
} MEM_MAP_META;

MEM_MAP_META  mLegacyMapMeta;
MEM_MAP_META  mMatMapMeta;

/// ================================================================================================
/// ================================================================================================
///
/// HELPER FUNCTIONS
///
/// ================================================================================================
/// ================================================================================================

VOID
DumpDescriptor (
  IN  UINTN                  DebugLevel,
  IN  CHAR16                 *Prefix OPTIONAL,
  IN  EFI_MEMORY_DESCRIPTOR  *Descriptor
  )
{
  if (Prefix) {
    DEBUG ((DebugLevel, "%s ", Prefix));
  }

  DEBUG ((DebugLevel, "Type - 0x%08X, ", Descriptor->Type));
  DEBUG ((DebugLevel, "PStart - 0x%016lX, ", Descriptor->PhysicalStart));
  DEBUG ((DebugLevel, "VStart - 0x%016lX, ", Descriptor->VirtualStart));
  DEBUG ((DebugLevel, "NPages - 0x%016lX, ", Descriptor->NumberOfPages));
  DEBUG ((DebugLevel, "Attribute - 0x%016lX\n", Descriptor->Attribute));
} // DumpDescriptor()

/// ================================================================================================
/// ================================================================================================
///
/// TEST CASES
///
/// ================================================================================================
/// ================================================================================================

UNIT_TEST_STATUS
EFIAPI
MemoryMapShouldHaveFewEntries (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return (mLegacyMapMeta.EntryCount <= 500) ?
         UNIT_TEST_PASSED :
         UNIT_TEST_ERROR_TEST_FAILED;
} // MemoryMapShouldHaveFewEntries()

UNIT_TEST_STATUS
EFIAPI
ListsShouldHaveTheSameDescriptorSize (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return (mLegacyMapMeta.EntrySize == mMatMapMeta.EntrySize) ?
         UNIT_TEST_PASSED :
         UNIT_TEST_ERROR_TEST_FAILED;
} // ListsShouldHaveTheSameDescriptorSize()

UNIT_TEST_STATUS
EFIAPI
LegacyMapSizeShouldBeAMultipleOfDescriptorSize (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UNIT_TEST_STATUS  Status = UNIT_TEST_PASSED;

  if (((mLegacyMapMeta.MapSize / mLegacyMapMeta.EntrySize) != mLegacyMapMeta.EntryCount) ||
      ((mLegacyMapMeta.MapSize % mLegacyMapMeta.EntrySize) != 0))
  {
    Status = UNIT_TEST_ERROR_TEST_FAILED;
  }

  return Status;
} // LegacyMapSizeShouldBeAMultipleOfDescriptorSize()

UNIT_TEST_STATUS
EFIAPI
MatMapSizeShouldBeAMultipleOfDescriptorSize (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UNIT_TEST_STATUS  Status = UNIT_TEST_PASSED;

  if (((mMatMapMeta.MapSize / mMatMapMeta.EntrySize) != mMatMapMeta.EntryCount) ||
      ((mMatMapMeta.MapSize % mMatMapMeta.EntrySize) != 0))
  {
    Status = UNIT_TEST_ERROR_TEST_FAILED;
  }

  return Status;
} // MatMapSizeShouldBeAMultipleOfDescriptorSize()

UNIT_TEST_STATUS
EFIAPI
NoLegacyMapEntriesShouldHaveZeroSize (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UNIT_TEST_STATUS       Status = UNIT_TEST_PASSED;
  UINTN                  Index;
  EFI_MEMORY_DESCRIPTOR  *Descriptor;

  for (Index = 0; Index < mLegacyMapMeta.EntryCount; Index++) {
    Descriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mLegacyMapMeta.Map + (Index * mLegacyMapMeta.EntrySize));
    if (Descriptor->NumberOfPages == 0) {
      Status = UNIT_TEST_ERROR_TEST_FAILED;
      break;
    }
  }

  return Status;
} // NoLegacyMapEntriesShouldHaveZeroSize()

UNIT_TEST_STATUS
EFIAPI
NoMatMapEntriesShouldHaveZeroSize (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UNIT_TEST_STATUS       Status = UNIT_TEST_PASSED;
  UINTN                  Index;
  EFI_MEMORY_DESCRIPTOR  *Descriptor;

  for (Index = 0; Index < mMatMapMeta.EntryCount; Index++) {
    Descriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mMatMapMeta.Map + (Index * mMatMapMeta.EntrySize));
    if (Descriptor->NumberOfPages == 0) {
      Status = UNIT_TEST_ERROR_TEST_FAILED;
      break;
    }
  }

  return Status;
} // NoMatMapEntriesShouldHaveZeroSize()

UNIT_TEST_STATUS
EFIAPI
AllLegacyMapEntriesShouldBeAligned (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UNIT_TEST_STATUS       Status = UNIT_TEST_PASSED;
  UINTN                  Index;
  EFI_MEMORY_DESCRIPTOR  *Descriptor;

  for (Index = 0; Index < mLegacyMapMeta.EntryCount; Index++) {
    Descriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mLegacyMapMeta.Map + (Index * mLegacyMapMeta.EntrySize));
    if (((Descriptor->PhysicalStart & EFI_PAGE_MASK) != 0) ||
        ((Descriptor->VirtualStart & EFI_PAGE_MASK) != 0))
    {
      Status = UNIT_TEST_ERROR_TEST_FAILED;
      break;
    }

    // per the UEFI spec, these types need to have
    // RUNTIME_PAGE_ALLOCATION_GRANULARITY
    if ((Descriptor->Type == EfiRuntimeServicesCode) ||
        (Descriptor->Type == EfiRuntimeServicesData) ||
        (Descriptor->Type == EfiACPIMemoryNVS) ||
        (Descriptor->Type == EfiReservedMemoryType))
    {
      if (((Descriptor->PhysicalStart & (RUNTIME_PAGE_ALLOCATION_GRANULARITY - 1)) != 0) ||
          ((Descriptor->VirtualStart & (RUNTIME_PAGE_ALLOCATION_GRANULARITY - 1)) != 0))
      {
        Status = UNIT_TEST_ERROR_TEST_FAILED;
        break;
      }
    }
  }

  return Status;
} // AllLegacyMapEntriesShouldBeAligned()

UNIT_TEST_STATUS
EFIAPI
AllMatEntriesShouldBeCertainTypes (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UNIT_TEST_STATUS       Status = UNIT_TEST_PASSED;
  UINTN                  Index;
  EFI_MEMORY_DESCRIPTOR  *Descriptor;

  for (Index = 0; Index < mMatMapMeta.EntryCount; Index++) {
    Descriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mMatMapMeta.Map + (Index * mMatMapMeta.EntrySize));

    // Make sure that all entries have a Runtime type.
    if ((Descriptor->Type != EfiRuntimeServicesCode) && (Descriptor->Type != EfiRuntimeServicesData)) {
      Status = UNIT_TEST_ERROR_TEST_FAILED;
      break;
    }
  }

  return Status;
} // AllMatEntriesShouldBeCertainTypes()

UNIT_TEST_STATUS
EFIAPI
AllMatEntriesShouldHaveRuntimeAttribute (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UNIT_TEST_STATUS       Status = UNIT_TEST_PASSED;
  UINTN                  Index;
  EFI_MEMORY_DESCRIPTOR  *Descriptor;

  for (Index = 0; Index < mMatMapMeta.EntryCount; Index++) {
    Descriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mMatMapMeta.Map + (Index * mMatMapMeta.EntrySize));

    // Make sure that all entries have a Runtime attribute.
    if ((Descriptor->Attribute & EFI_MEMORY_RUNTIME) != EFI_MEMORY_RUNTIME) {
      Status = UNIT_TEST_ERROR_TEST_FAILED;
      break;
    }
  }

  return Status;
} // AllMatEntriesShouldHaveRuntimeAttribute()

UNIT_TEST_STATUS
EFIAPI
AllMatEntriesShouldHaveNxOrRoAttribute (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UNIT_TEST_STATUS       Status = UNIT_TEST_PASSED;
  UINTN                  Index;
  EFI_MEMORY_DESCRIPTOR  *Descriptor;

  for (Index = 0; Index < mMatMapMeta.EntryCount; Index++) {
    Descriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mMatMapMeta.Map + (Index * mMatMapMeta.EntrySize));

    // Make sure that all data entries have a NX attribute.
    if (((Descriptor->Attribute & EFI_MEMORY_XP) != EFI_MEMORY_XP) &&
        ((Descriptor->Attribute & EFI_MEMORY_RO) != EFI_MEMORY_RO))
    {
      Status = UNIT_TEST_ERROR_TEST_FAILED;
      break;
    }
  }

  return Status;
} // AllMatEntriesShouldHaveNxOrRoAttribute()

UNIT_TEST_STATUS
EFIAPI
AllMatEntriesShouldBeRuntimePageGranularityAligned (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UNIT_TEST_STATUS       Status = UNIT_TEST_PASSED;
  UINTN                  Index;
  EFI_MEMORY_DESCRIPTOR  *Descriptor;

  for (Index = 0; Index < mMatMapMeta.EntryCount; Index++) {
    Descriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mMatMapMeta.Map + (Index * mMatMapMeta.EntrySize));

    // Make sure that all MAT entries are RUNTIME_PAGE_ALLOCATION_GRANULARITY aligned.
    if (((Descriptor->PhysicalStart & (RUNTIME_PAGE_ALLOCATION_GRANULARITY - 1)) != 0) ||
        ((Descriptor->VirtualStart & (RUNTIME_PAGE_ALLOCATION_GRANULARITY - 1)) != 0))
    {
      Status = UNIT_TEST_ERROR_TEST_FAILED;
      break;
    }
  }

  return Status;
} // AllMatEntriesShouldBe4kAligned()

UNIT_TEST_STATUS
EFIAPI
AllMatEntriesMustBeInAscendingOrder (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UNIT_TEST_STATUS       Status = UNIT_TEST_PASSED;
  UINTN                  Index;
  EFI_MEMORY_DESCRIPTOR  *Descriptor;
  EFI_PHYSICAL_ADDRESS   HighestKnownAddress = 0;

  for (Index = 0; Index < mMatMapMeta.EntryCount; Index++) {
    Descriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mMatMapMeta.Map + (Index * mMatMapMeta.EntrySize));

    // Make sure that the physical address for this descriptor is higher than the last.
    if (Descriptor->PhysicalStart <= HighestKnownAddress) {
      Status = UNIT_TEST_ERROR_TEST_FAILED;
      break;
    }

    // Increment the highest known address.
    HighestKnownAddress = Descriptor->PhysicalStart;
  }

  return Status;
} // AllMatEntriesMustBeInAscendingOrder()

UNIT_TEST_STATUS
EFIAPI
EntriesInASingleMapShouldNotOverlapAtAll (
  MEM_MAP_META  *TestMap
  )
{
  UNIT_TEST_STATUS       Status = UNIT_TEST_PASSED;
  UINTN                  LeftIndex, RightIndex;
  EFI_PHYSICAL_ADDRESS   LeftEnd, RightEnd;
  EFI_MEMORY_DESCRIPTOR  *LeftDescriptor, *RightDescriptor;
  BOOLEAN                Ok;

  Ok = TRUE;
  for (LeftIndex = 0; LeftIndex < TestMap->EntryCount && Ok; LeftIndex++) {
    LeftDescriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)TestMap->Map + (LeftIndex * TestMap->EntrySize));
    LeftEnd        = LeftDescriptor->PhysicalStart + EFI_PAGES_TO_SIZE (LeftDescriptor->NumberOfPages) - 1;

    // Start a new loop checking all of the entries in this list against the current one.
    for (RightIndex = LeftIndex + 1; RightIndex < TestMap->EntryCount && Ok; RightIndex++) {
      RightDescriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)TestMap->Map + (RightIndex * TestMap->EntrySize));
      RightEnd        = RightDescriptor->PhysicalStart + EFI_PAGES_TO_SIZE (RightDescriptor->NumberOfPages) - 1;

      // Actually compare the data.
      if (A_B_OVERLAPS_C_D (LeftDescriptor->PhysicalStart, LeftEnd, RightDescriptor->PhysicalStart, RightEnd)) {
        DumpDescriptor (DEBUG_VERBOSE, L"[LeftDescriptor]", LeftDescriptor);
        DumpDescriptor (DEBUG_VERBOSE, L"[RightDescriptor]", RightDescriptor);
        Status = UNIT_TEST_ERROR_TEST_FAILED;
        Ok     = FALSE;
        break;
      }
    }
  }

  return Status;
} // EntriesInASingleMapShouldNotOverlapAtAll()

UNIT_TEST_STATUS
EFIAPI
EntriesInLegacyMapShouldNotOverlapAtAll (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return EntriesInASingleMapShouldNotOverlapAtAll (&mLegacyMapMeta);
} // EntriesInLegacyMapShouldNotOverlapAtAll()

UNIT_TEST_STATUS
EFIAPI
EntriesInMatMapShouldNotOverlapAtAll (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return EntriesInASingleMapShouldNotOverlapAtAll (&mMatMapMeta);
} // EntriesInMatMapShouldNotOverlapAtAll()

UNIT_TEST_STATUS
EFIAPI
EntriesBetweenListsShouldNotOverlapBoundaries (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UNIT_TEST_STATUS       Status = UNIT_TEST_PASSED;
  UINTN                  LegacyIndex, MatIndex;
  EFI_PHYSICAL_ADDRESS   LegacyEnd, MatEnd;
  EFI_MEMORY_DESCRIPTOR  *LegacyDescriptor, *MatDescriptor;

  // Create an outer loop for the first list.
  for (LegacyIndex = 0; LegacyIndex < mLegacyMapMeta.EntryCount; LegacyIndex++) {
    LegacyDescriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mLegacyMapMeta.Map + (LegacyIndex * mLegacyMapMeta.EntrySize));
    LegacyEnd        = LegacyDescriptor->PhysicalStart + EFI_PAGES_TO_SIZE (LegacyDescriptor->NumberOfPages) - 1;

    // Create an inner loop for the second list.
    for (MatIndex = 0; MatIndex < mMatMapMeta.EntryCount; MatIndex++) {
      MatDescriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mMatMapMeta.Map + (MatIndex * mMatMapMeta.EntrySize));
      MatEnd        = MatDescriptor->PhysicalStart + EFI_PAGES_TO_SIZE (MatDescriptor->NumberOfPages) - 1;

      //
      // A bondary overlap is defined as an entry that lies across the start OR the end of another entry,
      // but not both (See diagram).
      //
      //    |---------|
      //    |         |
      //    |    A    |   |---------|
      //    |         |   |         |
      //    |         |   |    B    |
      //    |         |   |         |
      //    |---------|   |         |
      //                  |         |
      //                  |---------|
      //
      if ((A_IS_BETWEEN_B_AND_C (MatDescriptor->PhysicalStart, LegacyDescriptor->PhysicalStart, LegacyEnd) && (MatEnd > LegacyEnd)) ||
          (A_IS_BETWEEN_B_AND_C (LegacyDescriptor->PhysicalStart, MatDescriptor->PhysicalStart, MatEnd) && (LegacyEnd > MatEnd)))
      {
        DEBUG ((DEBUG_VERBOSE, "%a - Overlap between MemoryMaps!\n", __FUNCTION__));
        DumpDescriptor (DEBUG_VERBOSE, L"[MatDescriptor]", MatDescriptor);
        DumpDescriptor (DEBUG_VERBOSE, L"[LegacyDescriptor]", LegacyDescriptor);
        Status = UNIT_TEST_ERROR_TEST_FAILED;
        break;
      }
    }
  }

  return Status;
} // EntriesBetweenListsShouldNotOverlapBoundaries()

UNIT_TEST_STATUS
EFIAPI
AllEntriesInMatShouldLieWithinAMatchingEntryInMemmap (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UNIT_TEST_STATUS       Status = UNIT_TEST_PASSED;
  UINTN                  MatIndex, LegacyIndex;
  EFI_PHYSICAL_ADDRESS   MatEnd, LegacyEnd;
  EFI_MEMORY_DESCRIPTOR  *MatDescriptor, *LegacyDescriptor;
  BOOLEAN                MatchFound;

  // Create an outer loop for the first list.
  for (MatIndex = 0; MatIndex < mMatMapMeta.EntryCount; MatIndex++) {
    MatDescriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mMatMapMeta.Map + (MatIndex * mMatMapMeta.EntrySize));
    MatEnd        = MatDescriptor->PhysicalStart + EFI_PAGES_TO_SIZE (MatDescriptor->NumberOfPages) - 1;

    // Indicate that no match has yet been found for this entry.
    MatchFound = FALSE;

    // Create an inner loop for the second list.
    for (LegacyIndex = 0; LegacyIndex < mLegacyMapMeta.EntryCount && !MatchFound; LegacyIndex++) {
      LegacyDescriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mLegacyMapMeta.Map + (LegacyIndex * mLegacyMapMeta.EntrySize));
      LegacyEnd        = LegacyDescriptor->PhysicalStart + EFI_PAGES_TO_SIZE (LegacyDescriptor->NumberOfPages) - 1;

      //
      // Determine whether this MAT entry lies entirely within this Legacy entry.
      // An entry lies within if:
      //    - It starts at the same address or starts within AND
      //    - It ends at the same address or ends within.
      //
      if ((A_IS_BETWEEN_B_AND_C (MatDescriptor->PhysicalStart, LegacyDescriptor->PhysicalStart, LegacyEnd) ||
           (MatDescriptor->PhysicalStart == LegacyDescriptor->PhysicalStart)) &&
          (A_IS_BETWEEN_B_AND_C (MatEnd, LegacyDescriptor->PhysicalStart, LegacyEnd) || (MatEnd == LegacyEnd)))
      {
        // Now, make sure that the type matches.
        if (MatDescriptor->Type == LegacyDescriptor->Type) {
          MatchFound = TRUE;
        }
      }
    }

    // If a match was not found for this MAT entry, we have a problem.
    if (!MatchFound) {
      DEBUG ((DEBUG_VERBOSE, "%a - MAT entry not found in Legacy MemoryMap!\n", __FUNCTION__));
      DumpDescriptor (DEBUG_VERBOSE, NULL, MatDescriptor);
      Status = UNIT_TEST_ERROR_TEST_FAILED;
      break;
    }
  }

  return Status;
} // AllEntriesInMatShouldLieWithinAMatchingEntryInMemmap()

UNIT_TEST_STATUS
EFIAPI
AllMemmapRuntimeCodeAndDataEntriesMustBeEntirelyDescribedByMat (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UNIT_TEST_STATUS       Status = UNIT_TEST_PASSED;
  UINTN                  LegacyIndex, MatIndex;
  EFI_PHYSICAL_ADDRESS   LegacyEnd, MatEnd;
  EFI_MEMORY_DESCRIPTOR  *LegacyDescriptor, *MatDescriptor;
  EFI_PHYSICAL_ADDRESS   CurrentEntryProgress;
  BOOLEAN                EntryComplete;

  // Create an outer loop for the first list.
  for (LegacyIndex = 0; LegacyIndex < mLegacyMapMeta.EntryCount; LegacyIndex++) {
    LegacyDescriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mLegacyMapMeta.Map + (LegacyIndex * mLegacyMapMeta.EntrySize));
    LegacyEnd        = LegacyDescriptor->PhysicalStart + EFI_PAGES_TO_SIZE (LegacyDescriptor->NumberOfPages) - 1;

    // If this entry is not EfiRuntimeServicesCode or EfiRuntimeServicesData, we don't care.
    if ((LegacyDescriptor->Type != EfiRuntimeServicesCode) && (LegacyDescriptor->Type != EfiRuntimeServicesData)) {
      continue;   // Just keep looping over other entries.
    }

    //
    // Now that we've found an entry of interest, we must make sure that
    // the entire region is covered by MAT entries.
    // We'll start by setting a "high water mark" for how much of the current entry has been verified.
    // Since there's a prerequisite on the MAT entries being in ascending order, we can be confident
    // that a bottom-up approach will work.
    //
    CurrentEntryProgress = LegacyDescriptor->PhysicalStart;
    EntryComplete        = FALSE;

    // Create an inner loop for the second list.
    for (MatIndex = 0; MatIndex < mMatMapMeta.EntryCount && !EntryComplete; MatIndex++) {
      MatDescriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mMatMapMeta.Map + (MatIndex * mMatMapMeta.EntrySize));
      MatEnd        = MatDescriptor->PhysicalStart + EFI_PAGES_TO_SIZE (MatDescriptor->NumberOfPages) - 1;

      // If this entry doesn't match the type we're looking for, then it's of no interest.
      if (LegacyDescriptor->Type != MatDescriptor->Type) {
        continue;
      }

      //
      // If the start is the same as the high-water mark, we can remove the size from
      // the "unaccounted" region of the current entry.
      //
      if ((CurrentEntryProgress == MatDescriptor->PhysicalStart) ||
          A_IS_BETWEEN_B_AND_C (CurrentEntryProgress, MatDescriptor->PhysicalStart, MatEnd))
      {
        CurrentEntryProgress = MatEnd + 1;
      }

      // If the progress has now covered the entire entry, we're good.
      if (CurrentEntryProgress > LegacyEnd) {
        EntryComplete = TRUE;
        break;
      }
    }

    // If we never completed this entry, we're borked.
    if (!EntryComplete) {
      DEBUG ((DEBUG_VERBOSE, "%a - Legacy MemoryMap entry not covered by MAT entries!\n", __FUNCTION__));
      DumpDescriptor (DEBUG_VERBOSE, NULL, LegacyDescriptor);
      Status = UNIT_TEST_ERROR_TEST_FAILED;
      break;
    }
  }

  return Status;
} // AllMemmapRuntimeCodeAndDataEntriesMustBeEntirelyDescribedByMat()

/// ================================================================================================
/// ================================================================================================
///
/// TEST ENGINE
///
/// ================================================================================================
/// ================================================================================================

/**
  This function will gather information and configure the
  environment for all tests to operate.

  @retval     EFI_SUCCESS
  @retval     Others

**/
STATIC
EFI_STATUS
InitializeTestEnvironment (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_MEMORY_ATTRIBUTES_TABLE  *MatMap;
  EFI_MEMORY_DESCRIPTOR        *LegacyMap = NULL;
  UINTN                        MapSize, DescriptorSize;

  //
  // Make sure that the structures are clear.
  //
  ZeroMem (&mLegacyMapMeta, sizeof (mLegacyMapMeta));
  ZeroMem (&mMatMapMeta, sizeof (mMatMapMeta));

  //
  // Grab the legacy MemoryMap...
  //
  MapSize = 0;
  Status  = gBS->GetMemoryMap (&MapSize, LegacyMap, NULL, &DescriptorSize, NULL);
  if ((Status != EFI_BUFFER_TOO_SMALL) || !MapSize) {
    // If we're here, we had something weird happen.
    // By passing a size of 0, it should have returned EFI_BUFFER_TOO_SMALL.
    return EFI_UNSUPPORTED;
  }

  LegacyMap = AllocateZeroPool (MapSize);
  if (!LegacyMap) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->GetMemoryMap (&MapSize, LegacyMap, NULL, &DescriptorSize, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // MemoryMap data should now be in the structure.
  mLegacyMapMeta.MapSize    = MapSize;
  mLegacyMapMeta.EntrySize  = DescriptorSize;
  mLegacyMapMeta.EntryCount = (MapSize / DescriptorSize);
  mLegacyMapMeta.Map        = (VOID *)LegacyMap;      // This should be freed at some point.

  //
  // Grab the MAT memory map...
  //
  Status = EfiGetSystemConfigurationTable (&gEfiMemoryAttributesTableGuid, (VOID **)&MatMap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // MAT should now be at the pointer.
  mMatMapMeta.MapSize    = MatMap->NumberOfEntries * MatMap->DescriptorSize;
  mMatMapMeta.EntrySize  = MatMap->DescriptorSize;
  mMatMapMeta.EntryCount = MatMap->NumberOfEntries;
  mMatMapMeta.Map        = (VOID *)((UINT8 *)MatMap + sizeof (*MatMap));

  return Status;
} // InitializeTestEnvironment()

/**
  MemmapAndMatTestApp

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
MemmapAndMatTestApp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Fw = NULL;
  UNIT_TEST_SUITE_HANDLE      TableStructureTests, MatTableContentTests, TableEntryRangeTests;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  //
  // First, let's set up somethings that will be used by all test cases.
  //
  Status = InitializeTestEnvironment ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FAILED to initialize test environment!!\n"));
    goto EXIT;
  }

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the TableStructureTests Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&TableStructureTests, Fw, "Table Structure Tests", "Security.MAT.TableStructure", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for TableStructureTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (TableStructureTests, "Memory Maps should not have more than 500 entries", "Security.MAT.NumEntries", MemoryMapShouldHaveFewEntries, NULL, NULL, NULL);
  AddTestCase (TableStructureTests, "Memory Maps should have the same Descriptor size", "Security.MAT.DescriptorSize", ListsShouldHaveTheSameDescriptorSize, NULL, NULL, NULL);
  AddTestCase (TableStructureTests, "Standard MemoryMap size should be a multiple of the Descriptor size", "Security.MAT.MemMapSize", LegacyMapSizeShouldBeAMultipleOfDescriptorSize, NULL, NULL, NULL);
  AddTestCase (TableStructureTests, "MAT size should be a multiple of the Descriptor size", "Security.MAT.Size", MatMapSizeShouldBeAMultipleOfDescriptorSize, NULL, NULL, NULL);
  AddTestCase (TableStructureTests, "No standard MemoryMap entries should have a 0 size", "Security.MAT.MemMapZeroSizeEntries", NoLegacyMapEntriesShouldHaveZeroSize, NULL, NULL, NULL);
  AddTestCase (TableStructureTests, "No MAT entries should have a 0 size", "Security.MAT.MatZeroSizeEntries", NoMatMapEntriesShouldHaveZeroSize, NULL, NULL, NULL);
  AddTestCase (TableStructureTests, "All standard MemoryMap entries should be page aligned", "Security.MAT.MemMapAlignment", AllLegacyMapEntriesShouldBeAligned, NULL, NULL, NULL);

  //
  // Populate the MatTableContentTests Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&MatTableContentTests, Fw, "MAT Memory Map Content Tests", "Security.MAT.MatEntries", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MatTableContentTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (MatTableContentTests, "MAT entries should be EfiRuntimeServicesCode or EfiRuntimeServicesData", "Security.MAT.RtMemoryType", AllMatEntriesShouldBeCertainTypes, NULL, NULL, NULL);
  AddTestCase (MatTableContentTests, "MAT entries should all have the Runtime attribute", "Security.MAT.RtAttributes", AllMatEntriesShouldHaveRuntimeAttribute, NULL, NULL, NULL);
  AddTestCase (MatTableContentTests, "All MAT entries should have the XP or RO attribute", "Security.MAT.XPorRO", AllMatEntriesShouldHaveNxOrRoAttribute, NULL, NULL, NULL);
  AddTestCase (MatTableContentTests, "All MAT entries should be aligned on a 4k boundary", "Security.MAT.4kAlign", AllMatEntriesShouldBeRuntimePageGranularityAligned, NULL, NULL, NULL);
  AddTestCase (MatTableContentTests, "All MAT entries must appear in ascending order by physical start address", "Security.MAT.EntryOrder", AllMatEntriesMustBeInAscendingOrder, NULL, NULL, NULL);

  //
  // Populate the TableEntryRangeTests Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&TableEntryRangeTests, Fw, "Memory Map Entry Range Tests", "Security.MAT.RangeTest", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for TableEntryRangeTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (TableEntryRangeTests, "Entries in standard MemoryMap should not overlap each other at all", "Security.MAT.MemMapEntryOverlap", EntriesInLegacyMapShouldNotOverlapAtAll, NULL, NULL, NULL);
  AddTestCase (TableEntryRangeTests, "Entries in MAT should not overlap each other at all", "Security.MAT.MatEntryOverlap", EntriesInMatMapShouldNotOverlapAtAll, NULL, NULL, NULL);
  AddTestCase (TableEntryRangeTests, "Entries in one list should not overlap any of the boundaries of entries in the other", "Security.MAT.EntryOverlap", EntriesBetweenListsShouldNotOverlapBoundaries, NULL, NULL, NULL);
  AddTestCase (TableEntryRangeTests, "All MAT entries should lie entirely within a standard MemoryMap entry of the same type", "Security.MAT.EntriesWithinMemMap", AllEntriesInMatShouldLieWithinAMatchingEntryInMemmap, NULL, NULL, NULL);
  // NOTE: For this test, it would be ideal for the AllMatEntriesMustBeInAscendingOrder test to be a prereq, but since the prototype for
  //       a test case and a prereq are now different (and since I'm too lazy to write a wrapper function...) here we are.
  AddTestCase (
    TableEntryRangeTests,
    "All EfiRuntimeServicesCode and EfiRuntimeServicesData entries in standard MemoryMap must be entirely described by MAT",
    "Security.MAT.AllRtCodeInMat",
    AllMemmapRuntimeCodeAndDataEntriesMustBeEntirelyDescribedByMat,
    NULL,
    NULL,
    NULL
    );

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Fw);

EXIT:
  // Need to free the memory that was allocated for the Legacy Mem Map.
  if (mLegacyMapMeta.Map) {
    FreePool (mLegacyMapMeta.Map);
  }

  if (Fw) {
    FreeUnitTestFramework (Fw);
  }

  return Status;
}
