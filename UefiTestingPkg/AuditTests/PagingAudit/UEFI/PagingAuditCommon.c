/** @file -- DxePagingAuditCommon.c
This DXE Driver writes page table and memory map information to SFS when triggered
by an event.

Copyright (c) Microsoft Corporation. All rights reserved.
Copyright (c) 2009 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PagingAuditCommon.h"
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>
#include <Library/HobLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DxeMemoryProtectionHobLib.h>
#include <Protocol/CpuMpDebug.h>

#define NEXT_MEMORY_SPACE_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_GCD_MEMORY_SPACE_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) + (Size)))

#define PREVIOUS_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) - (Size)))

#define FILL_MEMORY_DESCRIPTOR_ENTRY(Entry, Start, Pages)                    \
  ((EFI_MEMORY_DESCRIPTOR*)Entry)->PhysicalStart  = (EFI_PHYSICAL_ADDRESS)Start;        \
  ((EFI_MEMORY_DESCRIPTOR*)Entry)->NumberOfPages  = (UINT64)Pages;                      \
  ((EFI_MEMORY_DESCRIPTOR*)Entry)->Attribute      = 0;                                  \
  ((EFI_MEMORY_DESCRIPTOR*)Entry)->Type           = NONE_EFI_MEMORY_TYPE;               \
  ((EFI_MEMORY_DESCRIPTOR*)Entry)->VirtualStart   = 0

/**
  Converts a number of EFI_PAGEs to a size in bytes.

  NOTE: Do not use EFI_PAGES_TO_SIZE because it handles UINTN only.

  @param  Pages     The number of EFI_PAGES.

  @return  The number of bytes associated with the number of EFI_PAGEs specified
           by Pages.
**/
STATIC
UINT64
EfiPagesToSize (
  IN UINT64  Pages
  )
{
  return LShiftU64 (Pages, EFI_PAGE_SHIFT);
}

/**
  Converts a size, in bytes, to a number of EFI_PAGESs.

  NOTE: Do not use EFI_SIZE_TO_PAGES because it handles UINTN only.

  @param  Size      A size in bytes.

  @return  The number of EFI_PAGESs associated with the number of bytes specified
           by Size.

**/
STATIC
UINT64
EfiSizeToPages (
  IN UINT64  Size
  )
{
  return RShiftU64 (Size, EFI_PAGE_SHIFT) + ((((UINTN)Size) & EFI_PAGE_MASK) ? 1 : 0);
}

/**

  Opens the SFS volume and if successful, returns a FS handle to the opened volume.

  @param    mFs_Handle       Handle to the opened volume.

  @retval   EFI_SUCCESS     The FS volume was opened successfully.
  @retval   Others          The operation failed.

**/
EFI_STATUS
OpenVolumeSFS (
  OUT EFI_FILE  **Fs_Handle
  );

MEMORY_PROTECTION_DEBUG_PROTOCOL  *mMemoryProtectionProtocol = NULL;
CPU_MP_DEBUG_PROTOCOL             *mCpuMpDebugProtocol       = NULL;
EFI_FILE                          *mFs_Handle;
extern CHAR8                      *mMemoryInfoDatabaseBuffer;
extern UINTN                      mMemoryInfoDatabaseSize;
extern UINTN                      mMemoryInfoDatabaseAllocSize;

/**
  Populates the heap guard protocol global

  @retval EFI_SUCCESS Protocol is already populated or was successfully populated
  @retval other       Return value of LocateProtocol
**/
STATIC
EFI_STATUS
PopulateHeapGuardDebugProtocol (
  VOID
  )
{
  if (mMemoryProtectionProtocol != NULL) {
    return EFI_SUCCESS;
  }

  return gBS->LocateProtocol (&gMemoryProtectionDebugProtocolGuid, NULL, (VOID **)&mMemoryProtectionProtocol);
}

/**
  Populates the CPU MP debug protocol global

  @retval EFI_SUCCESS Protocol is already populated or was successfully populated
  @retval other       Return value of LocateProtocol
**/
STATIC
EFI_STATUS
PopulateCpuMpDebugProtocol (
  VOID
  )
{
  if (mCpuMpDebugProtocol != NULL) {
    return EFI_SUCCESS;
  }

  return gBS->LocateProtocol (&gCpuMpDebugProtocolGuid, NULL, (VOID **)&mCpuMpDebugProtocol);
}

/**
  This helper function writes a string entry to the memory info database buffer.
  If string would exceed current buffer allocation, it will realloc.

  NOTE: The buffer tracks its size. It does not work with NULL terminators.

  @param[in]  DatabaseString    A pointer to a CHAR8 string that should be
                                added to the database.

  @retval     EFI_SUCCESS           String was successfully added.
  @retval     EFI_OUT_OF_RESOURCES  Buffer could not be grown to accommodate string.
                                    String has not been added.

**/
EFI_STATUS
EFIAPI
AppendToMemoryInfoDatabase (
  IN CONST CHAR8  *DatabaseString
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINTN       NewStringSize, NewDatabaseSize;
  CHAR8       *NewDatabaseBuffer;

  // If the incoming string is NULL or empty, get out of here.
  if ((DatabaseString == NULL) || (DatabaseString[0] == '\0')) {
    return EFI_SUCCESS;
  }

  // Determine the length of the incoming string.
  // NOTE: This size includes the NULL terminator.
  NewStringSize = AsciiStrnSizeS (DatabaseString, MEM_INFO_DATABASE_MAX_STRING_SIZE);
  NewStringSize = NewStringSize - sizeof (CHAR8);    // Remove NULL.

  // If we need more space, realloc now.
  // Subtract 1 because we only need a single NULL terminator.
  NewDatabaseSize = NewStringSize + mMemoryInfoDatabaseSize;
  if (NewDatabaseSize > mMemoryInfoDatabaseAllocSize) {
    NewDatabaseBuffer = ReallocatePool (
                          mMemoryInfoDatabaseAllocSize,
                          mMemoryInfoDatabaseAllocSize + MEM_INFO_DATABASE_REALLOC_CHUNK,
                          mMemoryInfoDatabaseBuffer
                          );
    // If we failed, don't change anything.
    if (NewDatabaseBuffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    }
    // Otherwise, updated the pointers and sizes.
    else {
      mMemoryInfoDatabaseBuffer     = NewDatabaseBuffer;
      mMemoryInfoDatabaseAllocSize += MEM_INFO_DATABASE_REALLOC_CHUNK;
    }
  }

  // If we're still good, copy the new string to the end of
  // the buffer and update the size.
  if (!EFI_ERROR (Status)) {
    // Subtract 1 to remove the previous NULL terminator.
    CopyMem (&mMemoryInfoDatabaseBuffer[mMemoryInfoDatabaseSize], DatabaseString, NewStringSize);
    mMemoryInfoDatabaseSize = NewDatabaseSize;
  }

  return Status;
} // AppendToMemoryInfoDatabase()

/**
  Creates a new file and writes the contents of the caller's data buffer to the file.

  @param    Fs_Handle           Handle to an opened filesystem volume/partition.
  @param    FileName            Name of the file to create.
  @param    DataBufferSize      Size of data to buffer to be written in bytes.
  @param    Data                Data to be written.

  @retval   EFI_STATUS          File was created and data successfully written.
  @retval   Others              The operation failed.

**/
EFI_STATUS
CreateAndWriteFileSFS (
  IN EFI_FILE  *Fs_Handle,
  IN CHAR16    *FileName,
  IN UINTN     DataBufferSize,
  IN VOID      *Data
  )
{
  EFI_STATUS  Status      = EFI_SUCCESS;
  EFI_FILE    *FileHandle = NULL;

  DEBUG ((DEBUG_ERROR, "%a: Creating file: %s \n", __FUNCTION__, FileName));

  // Create the file with RW permissions.
  //
  Status = Fs_Handle->Open (
                        Fs_Handle,
                        &FileHandle,
                        FileName,
                        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                        0
                        );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to create file %s: %r !\n", __FUNCTION__, FileName, Status));
    goto CleanUp;
  }

  // Write the contents of the caller's data buffer to the file.
  //
  Status = FileHandle->Write (
                         FileHandle,
                         &DataBufferSize,
                         Data
                         );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to write to file %s: %r !\n", __FUNCTION__, FileName, Status));
    goto CleanUp;
  }

  FileHandle->Flush (Fs_Handle);

CleanUp:

  // Close the file if it was successfully opened.
  //
  if (FileHandle != NULL) {
    FileHandle->Close (FileHandle);
  }

  return Status;
}

/**
 * @brief      Writes a buffer to file.
 *
 * @param      FileName     The name of the file being written to.
 * @param      Buffer       The buffer to write to file.
 * @param[in]  BufferSize   Size of the buffer.
 * @param[in]  WriteCount   Number to append to the end of the file.
 */
VOID
EFIAPI
WriteBufferToFile (
  IN CONST CHAR16  *FileName,
  IN       VOID    *Buffer,
  IN       UINTN   BufferSize
  )
{
  EFI_STATUS  Status;
  CHAR16      FileNameAndExt[MAX_STRING_SIZE];

  if (mFs_Handle == NULL) {
    Status = OpenVolumeSFS (&mFs_Handle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a error opening sfs volume - %r\n", __FUNCTION__, Status));
      return;
    }
  }

  // Calculate final file name.
  ZeroMem (FileNameAndExt, sizeof (CHAR16) * MAX_STRING_SIZE);
  UnicodeSPrint (FileNameAndExt, MAX_STRING_SIZE, L"%s.dat", FileName);

  Status = CreateAndWriteFileSFS (mFs_Handle, FileNameAndExt, BufferSize, Buffer);
  DEBUG ((DEBUG_ERROR, "%a Writing file %s - %r\n", __FUNCTION__, FileNameAndExt, Status));
}

/**
 * @brief      Writes the MemoryAttributesTable to a file.
 */
VOID
EFIAPI
MemoryAttributesTableDump (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_MEMORY_ATTRIBUTES_TABLE  *MatMap;
  EFI_MEMORY_DESCRIPTOR        *Map;
  UINTN                        EntrySize;
  UINTN                        EntryCount;
  CHAR8                        *WriteString;
  CHAR8                        *Buffer;
  UINT64                       Index;
  UINTN                        BufferSize;
  UINTN                        FormattedStringSize;
  // NOTE: Important to use fixed-size formatters for pointer movement.
  CHAR8  MatFormatString[] = "MAT,0x%016lx,0x%016lx,0x%016lx,0x%016lx,0x%016lx,0x%016lx\n";
  CHAR8  TempString[MAX_STRING_SIZE];

  //
  // First, we need to locate the MAT table.
  //
  Status = EfiGetSystemConfigurationTable (&gEfiMemoryAttributesTableGuid, (VOID *)&MatMap);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a Failed to retrieve MAT %r\n", __FUNCTION__, Status));
    return;
  }

  // MAT should now be at the pointer.
  EntrySize  = MatMap->DescriptorSize;
  EntryCount = MatMap->NumberOfEntries;
  Map        = (VOID *)((UINT8 *)MatMap + sizeof (*MatMap));

  //
  // Next, we need to allocate a buffer to hold all of the entries.
  // We'll be storing the data as fixed-length strings.
  //
  // Do a dummy format to determine the size of a string.
  // We're safe to use 0's, since the formatters are fixed-size.
  FormattedStringSize = AsciiSPrint (TempString, MAX_STRING_SIZE, MatFormatString, 0, 0, 0, 0, 0, NONE_GCD_MEMORY_TYPE);
  // Make sure to add space for the NULL terminator at the end.
  BufferSize = (EntryCount * FormattedStringSize) + sizeof (CHAR8);
  Buffer     = AllocatePool (BufferSize);
  if (!Buffer) {
    DEBUG ((DEBUG_ERROR, "%a Failed to allocate buffer for data dump!\n", __FUNCTION__));
    return;
  }

  //
  // Add all entries to the buffer.
  //
  WriteString = Buffer;
  for (Index = 0; Index < EntryCount; Index++) {
    AsciiSPrint (
      WriteString,
      FormattedStringSize+1,
      MatFormatString,
      Map->Type,
      Map->PhysicalStart,
      Map->VirtualStart,
      Map->NumberOfPages,
      Map->Attribute,
      NONE_GCD_MEMORY_TYPE
      );

    WriteString += FormattedStringSize;
    Map          = NEXT_MEMORY_DESCRIPTOR (Map, EntrySize);
  }

  //
  // Finally, write the strings to the dump file.
  //
  // NOTE: Don't need to save the NULL terminator.
  WriteBufferToFile (L"MAT", Buffer, BufferSize-1);

  FreePool (Buffer);
}

/**
  Sort memory map entries based upon PhysicalStart, from low to high.

  @param[in, out]   MemoryMap       A pointer to the buffer in which firmware places
                                    the current memory map
  @param[in]        MemoryMapSize   Size, in bytes, of the MemoryMap buffer
  @param[in]        DescriptorSize  Size, in bytes, of each descriptor region in the array
                                    NOTE: This is not sizeof (EFI_MEMORY_DESCRIPTOR)
**/
VOID
EFIAPI
SortMemoryMap (
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                      MemoryMapSize,
  IN UINTN                      DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *NextMemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR  TempMemoryMap;

  MemoryMapEntry     = MemoryMap;
  NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  MemoryMapEnd       = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + MemoryMapSize);
  while (MemoryMapEntry < MemoryMapEnd) {
    while (NextMemoryMapEntry < MemoryMapEnd) {
      if (MemoryMapEntry->PhysicalStart > NextMemoryMapEntry->PhysicalStart) {
        CopyMem (&TempMemoryMap, MemoryMapEntry, sizeof (EFI_MEMORY_DESCRIPTOR));
        CopyMem (MemoryMapEntry, NextMemoryMapEntry, sizeof (EFI_MEMORY_DESCRIPTOR));
        CopyMem (NextMemoryMapEntry, &TempMemoryMap, sizeof (EFI_MEMORY_DESCRIPTOR));
      }

      NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (NextMemoryMapEntry, DescriptorSize);
    }

    MemoryMapEntry     = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
    NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }

  return;
}

/**
  Sort memory map entries based upon PhysicalStart, from low to high.

  @param[in, out]   MemoryMap       A pointer to the buffer containing the current memory map
  @param[in]        MemoryMapSize   Size, in bytes, of the MemoryMap buffer
  @param[in]        DescriptorSize  Size, in bytes, of an individual EFI_GCD_MEMORY_SPACE_DESCRIPTOR
**/
VOID
EFIAPI
SortMemorySpaceMap (
  IN OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemoryMap,
  IN UINTN                                MemoryMapSize,
  IN UINTN                                DescriptorSize
  )
{
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemoryMapEntry;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *NextMemoryMapEntry;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemoryMapEnd;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  TempMemoryMap;

  MemoryMapEntry     = MemoryMap;
  NextMemoryMapEntry = NEXT_MEMORY_SPACE_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  MemoryMapEnd       = (EFI_GCD_MEMORY_SPACE_DESCRIPTOR *)((UINT8 *)MemoryMap + MemoryMapSize);
  while (MemoryMapEntry < MemoryMapEnd) {
    while (NextMemoryMapEntry < MemoryMapEnd) {
      if (MemoryMapEntry->BaseAddress > NextMemoryMapEntry->BaseAddress) {
        CopyMem (&TempMemoryMap, MemoryMapEntry, sizeof (EFI_GCD_MEMORY_SPACE_DESCRIPTOR));
        CopyMem (MemoryMapEntry, NextMemoryMapEntry, sizeof (EFI_GCD_MEMORY_SPACE_DESCRIPTOR));
        CopyMem (NextMemoryMapEntry, &TempMemoryMap, sizeof (EFI_GCD_MEMORY_SPACE_DESCRIPTOR));
      }

      NextMemoryMapEntry = NEXT_MEMORY_SPACE_DESCRIPTOR (NextMemoryMapEntry, DescriptorSize);
    }

    MemoryMapEntry     = NEXT_MEMORY_SPACE_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
    NextMemoryMapEntry = NEXT_MEMORY_SPACE_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }

  return;
}

/**
  Merges contiguous entries with the same GCD type

  @param[in, out]  NumberOfDescriptors IN:  Pointer to the number of descriptors of the input allocated memory map
                                       OUT: Value at pointer updated to the number of descriptors of the merged memory map
  @param[in, out]  MemorySpaceMap      IN:  Pointer to a valid EFI memory map. The memory map at the pointer will be freed
                                       OUT: Pointer to a merged memory map

  @retval EFI_SUCCESS           Successfully merged entries
  @retval EFI_OUT_OF_RECOURCES  Failed to allocate pools
  @retval EFI_INVALID_PARAMETER MemorySpaceMap or NumberOfDescriptors was NULL
**/
STATIC
EFI_STATUS
MergeMemorySpaceMap (
  IN OUT UINTN                            *NumberOfDescriptors,
  IN OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  **MemorySpaceMap
  )
{
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *NewMemoryMap = NULL;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *NewMemoryMapStart;
  UINTN                            Index = 0;

  if ((MemorySpaceMap == NULL) || (*MemorySpaceMap == NULL) || (NumberOfDescriptors == NULL) || (*NumberOfDescriptors <= 1)) {
    return EFI_INVALID_PARAMETER;
  }

  NewMemoryMap = AllocatePool (*NumberOfDescriptors * sizeof (EFI_GCD_MEMORY_SPACE_DESCRIPTOR));

  if (NewMemoryMap == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewMemoryMapStart = NewMemoryMap;

  while (Index < *NumberOfDescriptors) {
    CopyMem (NewMemoryMap, &((*MemorySpaceMap)[Index]), sizeof (EFI_GCD_MEMORY_SPACE_DESCRIPTOR));
    while (Index + 1 < *NumberOfDescriptors) {
      if ((NewMemoryMap->GcdMemoryType == ((*MemorySpaceMap)[Index + 1].GcdMemoryType)) &&
          ((NewMemoryMap->BaseAddress + NewMemoryMap->Length) == (*MemorySpaceMap)[Index + 1].BaseAddress))
      {
        NewMemoryMap->Length += (*MemorySpaceMap)[++Index].Length;
      } else {
        break;
      }
    }

    NewMemoryMap++;
    Index++;
  }

  *NumberOfDescriptors = NewMemoryMap - NewMemoryMapStart;

  FreePool (*MemorySpaceMap);
  *MemorySpaceMap = AllocateCopyPool (*NumberOfDescriptors * sizeof (EFI_GCD_MEMORY_SPACE_DESCRIPTOR), NewMemoryMapStart);
  FreePool (NewMemoryMapStart);

  if (*MemorySpaceMap == NULL ) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Updates the memory map to contain contiguous entries from StartOfAddressSpace to
  max(EndOfAddressSpace, address + length of the final memory map entry)

  @param[in, out] MemoryMapSize         Size, in bytes, of MemoryMap
  @param[in, out] MemoryMap             IN:  Pointer to the EFI memory map which will have all gaps filled. The
                                             original buffer will be freed and updated to a newly allocated buffer
                                        OUT: Pointer to the pointer to a SORTED memory map
  @param[in]      DescriptorSize        Size, in bytes, of each descriptor region in the array. NOTE: This is not
                                        sizeof (EFI_MEMORY_DESCRIPTOR).
  @param[in]      StartOfAddressSpace   Starting address from which there should be contiguous entries
  @param[in]      EndOfAddressSpace     Ending address at which the memory map should at least reach

  @retval EFI_SUCCESS                   Successfully merged entries
  @retval EFI_OUT_OF_RECOURCES          Failed to allocate pools
  @retval EFI_INVALID_PARAMETER         MemoryMap == NULL, *MemoryMap == NULL, *MemoryMapSize == 0, or
                                        DescriptorSize == 0
**/
STATIC
EFI_STATUS
FillInMemoryMap (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  **MemoryMap,
  IN     UINTN                  DescriptorSize,
  IN     EFI_PHYSICAL_ADDRESS   StartOfAddressSpace,
  IN     EFI_PHYSICAL_ADDRESS   EndOfAddressSpace
  )
{
  EFI_MEMORY_DESCRIPTOR  *OldMemoryMapCurrent, *OldMemoryMapEnd, *NewMemoryMapStart, *NewMemoryMapCurrent;
  EFI_PHYSICAL_ADDRESS   LastEntryEnd, NextEntryStart;

  if ((MemoryMap == NULL) || (*MemoryMap == NULL) || (MemoryMapSize == NULL) || (*MemoryMapSize == 0) || (DescriptorSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  NewMemoryMapStart = NULL;

  // Double the size of the memory map for the worst case of every entry being non-contiguous
  NewMemoryMapStart = AllocatePool ((*MemoryMapSize * 2) + (DescriptorSize * 2));

  if (NewMemoryMapStart == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewMemoryMapCurrent = NewMemoryMapStart;
  OldMemoryMapCurrent = *MemoryMap;
  OldMemoryMapEnd     = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)*MemoryMap + *MemoryMapSize);

  // Check if we need to insert a new entry at the start of the memory map
  if (OldMemoryMapCurrent->PhysicalStart > StartOfAddressSpace) {
    FILL_MEMORY_DESCRIPTOR_ENTRY (
      NewMemoryMapCurrent,
      StartOfAddressSpace,
      EfiSizeToPages (OldMemoryMapCurrent->PhysicalStart - StartOfAddressSpace)
      );

    NewMemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapCurrent, DescriptorSize);
  }

  while (OldMemoryMapCurrent < OldMemoryMapEnd) {
    CopyMem (NewMemoryMapCurrent, OldMemoryMapCurrent, DescriptorSize);
    if (NEXT_MEMORY_DESCRIPTOR (OldMemoryMapCurrent, DescriptorSize) < OldMemoryMapEnd) {
      LastEntryEnd   = NewMemoryMapCurrent->PhysicalStart + EfiPagesToSize (NewMemoryMapCurrent->NumberOfPages);
      NextEntryStart = NEXT_MEMORY_DESCRIPTOR (OldMemoryMapCurrent, DescriptorSize)->PhysicalStart;
      // Check for a gap in the memory map
      if (NextEntryStart != LastEntryEnd) {
        NewMemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapCurrent, DescriptorSize);
        FILL_MEMORY_DESCRIPTOR_ENTRY (
          NewMemoryMapCurrent,
          LastEntryEnd,
          EfiSizeToPages (NextEntryStart - LastEntryEnd)
          );
      }
    }

    NewMemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapCurrent, DescriptorSize);
    OldMemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (OldMemoryMapCurrent, DescriptorSize);
  }

  LastEntryEnd = PREVIOUS_MEMORY_DESCRIPTOR (NewMemoryMapCurrent, DescriptorSize)->PhysicalStart +
                 EfiPagesToSize (PREVIOUS_MEMORY_DESCRIPTOR (NewMemoryMapCurrent, DescriptorSize)->NumberOfPages);

  // Check if we need to insert a new entry at the end of the memory map
  if (EndOfAddressSpace > LastEntryEnd) {
    FILL_MEMORY_DESCRIPTOR_ENTRY (
      NewMemoryMapCurrent,
      LastEntryEnd,
      EfiSizeToPages (EndOfAddressSpace - LastEntryEnd)
      );

    NewMemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapCurrent, DescriptorSize);
  }

  // Re-use this stack variable as an intermediate to ensure we can allocate a buffer before updating the old memory map
  OldMemoryMapCurrent = AllocateCopyPool ((UINTN)((UINT8 *)NewMemoryMapCurrent - (UINT8 *)NewMemoryMapStart), NewMemoryMapStart);

  if (OldMemoryMapCurrent == NULL ) {
    FreePool (NewMemoryMapStart);
    return EFI_OUT_OF_RESOURCES;
  }

  FreePool (*MemoryMap);
  *MemoryMap = OldMemoryMapCurrent;

  *MemoryMapSize = (UINTN)((UINT8 *)NewMemoryMapCurrent - (UINT8 *)NewMemoryMapStart);
  FreePool (NewMemoryMapStart);

  return EFI_SUCCESS;
}

/**
  Find GCD memory type for the input region. If one GCD type does not cover the entire region, return the remaining
  pages which are covered by one or more subsequent GCD descriptors

  @param[in]  MemorySpaceMap        A SORTED array of GCD memory descrptors
  @param[in]  NumberOfDescriptors   The number of descriptors in the GCD descriptor array
  @param[in]  PhysicalStart         Page-aligned starting address to check against GCD descriptors
  @param[in]  NumberOfPages         Number of pages of the region being checked
  @param[out] Type                  The GCD memory type which applies to
                                    PhyscialStart + NumberOfPages - <remaining uncovered pages>

  @return Remaining pages not covered by a GCD Memory region
**/
STATIC
UINT64
GetOverlappingMemorySpaceRegion (
  IN EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap,
  IN UINTN                            NumberOfDescriptors,
  IN EFI_PHYSICAL_ADDRESS             PhysicalStart,
  IN UINT64                           NumberOfPages,
  OUT EFI_GCD_MEMORY_TYPE             *Type
  )
{
  UINTN                 Index;
  EFI_PHYSICAL_ADDRESS  PhysicalEnd, MapEntryStart, MapEntryEnd;

  if ((MemorySpaceMap == NULL) || (Type == NULL) || (NumberOfPages == 0) || (NumberOfDescriptors == 0)) {
    return 0;
  }

  PhysicalEnd = PhysicalStart + EfiPagesToSize (NumberOfPages);

  // Ensure the PhysicalStart is page aligned
  ASSERT ((PhysicalStart & EFI_PAGE_MASK) == 0);

  // Go through each memory space map entry
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    MapEntryStart = MemorySpaceMap[Index].BaseAddress;
    MapEntryEnd   = MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length;

    // Ensure the MapEntryStart and MapEntryEnd are page aligned
    ASSERT ((MapEntryStart & EFI_PAGE_MASK) == 0);
    ASSERT ((MapEntryEnd & EFI_PAGE_MASK) == 0);

    // Check if the memory map entry contains the physical start
    if ((MapEntryStart <= PhysicalStart) && (MapEntryEnd > PhysicalStart)) {
      *Type = MemorySpaceMap[Index].GcdMemoryType;
      // Check if the memory map entry contains the entire physical region
      if (MapEntryEnd >= PhysicalEnd) {
        return 0;
      } else {
        // Return remaining number of pages
        return EfiSizeToPages (PhysicalEnd - MapEntryEnd);
      }
    }
  }

  *Type = EfiGcdMemoryTypeNonExistent;
  return 0;
}

/**
 * @brief      Writes the UEFI memory map to file.
 */
VOID
EFIAPI
MemoryMapDumpHandler (
  VOID
  )
{
  EFI_STATUS                       Status;
  UINTN                            EfiMemoryMapSize;
  UINTN                            EfiMapKey;
  UINTN                            EfiDescriptorSize;
  UINT32                           EfiDescriptorVersion;
  EFI_MEMORY_DESCRIPTOR            *EfiMemoryMap;
  EFI_MEMORY_DESCRIPTOR            *EfiMemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR            *EfiMemNext;
  CHAR8                            TempString[MAX_STRING_SIZE];
  UINTN                            NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap = NULL;
  EFI_GCD_MEMORY_TYPE              MemorySpaceType;
  UINT64                           RemainingPages;

  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));

  if (EFI_ERROR (PopulateHeapGuardDebugProtocol ())) {
    DEBUG ((DEBUG_ERROR, "%a - Error finding heap guard debug protocol\n", __FUNCTION__));
  }

  //
  // Get the EFI memory map.
  //
  EfiMemoryMapSize = 0;
  EfiMemoryMap     = NULL;
  Status           = gBS->GetMemoryMap (
                            &EfiMemoryMapSize,
                            EfiMemoryMap,
                            &EfiMapKey,
                            &EfiDescriptorSize,
                            &EfiDescriptorVersion
                            );
  //
  // Loop to allocate space for the memory map and then copy it in.
  //
  do {
    EfiMemoryMap = (EFI_MEMORY_DESCRIPTOR *)AllocateZeroPool (EfiMemoryMapSize);
    if (EfiMemoryMap == NULL) {
      ASSERT (EfiMemoryMap != NULL);
      DEBUG ((DEBUG_ERROR, "%a - Unable to allocate memory for the EFI memory map.\n", __FUNCTION__));
      return;
    }

    Status = gBS->GetMemoryMap (
                    &EfiMemoryMapSize,
                    EfiMemoryMap,
                    &EfiMapKey,
                    &EfiDescriptorSize,
                    &EfiDescriptorVersion
                    );
    if (EFI_ERROR (Status)) {
      FreePool (EfiMemoryMap);
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);

  if (!EFI_ERROR (Status)) {
    Status = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Unable to fetch memory space map. Status; %r\n", __FUNCTION__, Status));
      goto Done;
    }

    SortMemorySpaceMap (MemorySpaceMap, NumberOfDescriptors, sizeof (EFI_GCD_MEMORY_SPACE_DESCRIPTOR));
    Status = MergeMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "%a - Unable to merge memory space map entries. Status: %r\n", __FUNCTION__, Status));
    }

    SortMemoryMap (EfiMemoryMap, EfiMemoryMapSize, EfiDescriptorSize);

    Status = FillInMemoryMap (
               &EfiMemoryMapSize,
               &EfiMemoryMap,
               EfiDescriptorSize,
               MemorySpaceMap->BaseAddress,
               MemorySpaceMap[NumberOfDescriptors - 1].BaseAddress + MemorySpaceMap[NumberOfDescriptors - 1].Length
               );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "%a - Error filling in gaps in memory map - the output data may not be complete. Status: %r\n",
        __FUNCTION__,
        Status
        ));
    }

    EfiMemoryMapEnd = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)EfiMemoryMap + EfiMemoryMapSize);
    EfiMemNext      = EfiMemoryMap;

    while (EfiMemNext < EfiMemoryMapEnd) {
      RemainingPages = GetOverlappingMemorySpaceRegion (
                         MemorySpaceMap,
                         NumberOfDescriptors,
                         EfiMemNext->PhysicalStart,
                         EfiMemNext->NumberOfPages,
                         &MemorySpaceType
                         );

      AsciiSPrint (
        TempString,
        MAX_STRING_SIZE,
        "MemoryMap,0x%016lx,0x%016lx,0x%016lx,0x%016lx,0x%016lx,0x%x\n",
        EfiMemNext->Type,
        EfiMemNext->PhysicalStart,
        EfiMemNext->VirtualStart,
        EfiMemNext->NumberOfPages - RemainingPages,
        EfiMemNext->Attribute,
        MemorySpaceType
        );
      AppendToMemoryInfoDatabase (TempString);
      if (RemainingPages > 0) {
        EfiMemNext->PhysicalStart += EfiPagesToSize (EfiMemNext->NumberOfPages - RemainingPages);
        EfiMemNext->NumberOfPages  = RemainingPages;
        if (EfiMemNext->VirtualStart > 0) {
          EfiMemNext->VirtualStart += EfiPagesToSize (EfiMemNext->NumberOfPages - RemainingPages);
        }
      } else {
        EfiMemNext = NEXT_MEMORY_DESCRIPTOR (EfiMemNext, EfiDescriptorSize);
      }
    }
  }

Done:
  if (EfiMemoryMap != NULL) {
    FreePool (EfiMemoryMap);
  }

  if (MemorySpaceMap != NULL) {
    FreePool (MemorySpaceMap);
  }
}

/**
 * @brief      Writes the name, base, and limit of each image in the image table to a file.
 */
VOID
EFIAPI
LoadedImageTableDump (
  VOID
  )
{
  EFI_STATUS                         Status;
  EFI_DEBUG_IMAGE_INFO_TABLE_HEADER  *TableHeader;
  EFI_DEBUG_IMAGE_INFO               *Table;
  EFI_LOADED_IMAGE_PROTOCOL          *LoadedImageProtocolInstance;
  UINTN                              ImageBase;
  UINT64                             ImageSize;
  UINT64                             Index;
  UINT32                             TableSize;
  EFI_DEBUG_IMAGE_INFO_NORMAL        *NormalImage;
  CHAR8                              *PdbFileName;
  CHAR8                              TempString[MAX_STRING_SIZE];

  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));

  //
  // locate DebugImageInfoTable
  //
  Status = EfiGetSystemConfigurationTable (&gEfiDebugImageInfoTableGuid, (VOID **)&TableHeader);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to retrieve loaded image table %r", Status));
    return;
  }

  Table     = TableHeader->EfiDebugImageInfoTable;
  TableSize = TableHeader->TableSize;

  DEBUG ((DEBUG_VERBOSE, "%a\n\nLength %lx Start x0x%016lx\n\n", __FUNCTION__, TableHeader->TableSize, Table));

  for (Index = 0; Index < TableSize; Index++) {
    if (Table[Index].NormalImage == NULL) {
      continue;
    }

    NormalImage                 = Table[Index].NormalImage;
    LoadedImageProtocolInstance = NormalImage->LoadedImageProtocolInstance;
    ImageSize                   = LoadedImageProtocolInstance->ImageSize;
    ImageBase                   = (UINTN)LoadedImageProtocolInstance->ImageBase;

    if (ImageSize == 0) {
      // No need to register empty slots in the table as images.
      continue;
    }

    PdbFileName = PeCoffLoaderGetPdbPointer (LoadedImageProtocolInstance->ImageBase);
    AsciiSPrint (
      TempString,
      MAX_STRING_SIZE,
      "LoadedImage,0x%016lx,0x%016lx,%a\n",
      ImageBase,
      ImageSize,
      PdbFileName
      );
    AppendToMemoryInfoDatabase (TempString);
  }
}

/**

  Opens the SFS volume and if successful, returns a FS handle to the opened volume.

  @param    mFs_Handle       Handle to the opened volume.

  @retval   EFI_SUCCESS     The FS volume was opened successfully.
  @retval   Others          The operation failed.

**/
EFI_STATUS
OpenVolumeSFS (
  OUT EFI_FILE  **Fs_Handle
  )
{
  EFI_DEVICE_PATH_PROTOCOL         *DevicePath;
  BOOLEAN                          Found;
  EFI_HANDLE                       Handle;
  EFI_HANDLE                       *HandleBuffer;
  UINTN                            Index;
  UINTN                            NumHandles;
  EFI_DEVICE_PATH_PROTOCOL         *OrigDevicePath;
  EFI_STRING                       PathNameStr;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SfProtocol;
  EFI_STATUS                       Status;

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
    // Save the original device path because we change it as we're checking it
    // below. We'll need the unmodified version if we determine that it's good.
    //
    OrigDevicePath = DevicePath;

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

    //
    // Restore the device path and check if this is a GPT partition. We only
    // want to write our log on GPT partitions.
    //
    DevicePath = OrigDevicePath;
    while (IsDevicePathEnd (DevicePath) == FALSE) {
      //
      // If the device path is not a hard drive, we don't want it.
      //
      if ((DevicePathType (DevicePath) == MEDIA_DEVICE_PATH) &&
          (DevicePathSubType (DevicePath) == MEDIA_HARDDRIVE_DP))
      {
        //
        // Check if this is a gpt partition. If it is, we'll use it. Otherwise,
        // keep searching.
        //
        if ((((HARDDRIVE_DEVICE_PATH *)DevicePath)->MBRType == MBR_TYPE_EFI_PARTITION_TABLE_HEADER) &&
            (((HARDDRIVE_DEVICE_PATH *)DevicePath)->SignatureType == SIGNATURE_TYPE_GUID))
        {
          DevicePath = OrigDevicePath;
          Found      = TRUE;
          break;
        }
      }

      //
      // Still searching. Advance to the next device path node.
      //
      DevicePath = NextDevicePathNode (DevicePath);
    }

    //
    // If we found a good device path, stop searching.
    //
    if (Found != FALSE) {
      DEBUG ((DEBUG_ERROR, "%a: found GPT partition Index:%d\n", __FUNCTION__, Index));
      break;
    }
  }

  //
  // If a suitable handle was not found, return error.
  //
  if (Found == FALSE) {
    Status = EFI_NOT_FOUND;
    goto CleanUp;
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
  Status = SfProtocol->OpenVolume (SfProtocol, Fs_Handle);
  if (EFI_ERROR (Status) != FALSE) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to open Simple FS volume fs0: %r \n", __FUNCTION__, Status));
    goto CleanUp;
  }

CleanUp:
  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  return Status;
}

BOOLEAN
LoadFlatPageTableData (
  OUT UINTN   *Pte1GCount,
  OUT UINTN   *Pte2MCount,
  OUT UINTN   *Pte4KCount,
  OUT UINTN   *PdeCount,
  OUT UINTN   *GuardCount,
  OUT UINT64  **Pte1GEntries,
  OUT UINT64  **Pte2MEntries,
  OUT UINT64  **Pte4KEntries,
  OUT UINT64  **PdeEntries,
  OUT UINT64  **GuardEntries
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  // Run once to get counts.
  DEBUG ((DEBUG_ERROR, "%a - First call to determine required buffer sizes.\n", __FUNCTION__));
  *Pte1GCount = 0;
  *Pte2MCount = 0;
  *Pte4KCount = 0;
  *PdeCount   = 0;
  *GuardCount = 0;
  Status      = GetFlatPageTableData (Pte1GCount, Pte2MCount, Pte4KCount, PdeCount, GuardCount, NULL, NULL, NULL, NULL, NULL);

  (*Pte1GCount) += 15;
  (*Pte2MCount) += 15;
  (*Pte4KCount) += 15;
  (*PdeCount)   += 15;

  // Allocate buffers if successful.
  if (!EFI_ERROR (Status)) {
    *Pte1GEntries = AllocateZeroPool (*Pte1GCount * sizeof (UINT64));
    *Pte2MEntries = AllocateZeroPool (*Pte2MCount * sizeof (UINT64));
    *Pte4KEntries = AllocateZeroPool (*Pte4KCount * sizeof (UINT64));
    *PdeEntries   = AllocateZeroPool (*PdeCount * sizeof (UINT64));
    *GuardEntries = AllocateZeroPool (*GuardCount * sizeof (UINT64));

    // Check for errors.
    if ((*Pte1GEntries == NULL) || (*Pte2MEntries == NULL) || (*Pte4KEntries == NULL) || (*PdeEntries == NULL) || (*GuardEntries == NULL)) {
      Status = EFI_OUT_OF_RESOURCES;
    }
  }

  // If still good, grab the data.
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a - Second call to grab the data.\n", __FUNCTION__));
    Status = GetFlatPageTableData (
               Pte1GCount,
               Pte2MCount,
               Pte4KCount,
               PdeCount,
               GuardCount,
               *Pte1GEntries,
               *Pte2MEntries,
               *Pte4KEntries,
               *PdeEntries,
               *GuardEntries
               );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      DEBUG ((DEBUG_ERROR, "%a Second GetFlatPageTableData call returned - %r\n", __FUNCTION__, Status));
      FreePool (*Pte1GEntries);
      FreePool (*Pte2MEntries);
      FreePool (*Pte4KEntries);
      FreePool (*PdeEntries);
      FreePool (*GuardEntries);

      (*Pte1GCount) += 15;
      (*Pte2MCount) += 15;
      (*Pte4KCount) += 15;
      (*PdeCount)   += 15;
      (*GuardCount) += 15;

      *Pte1GEntries = AllocateZeroPool (*Pte1GCount * sizeof (UINT64));
      *Pte2MEntries = AllocateZeroPool (*Pte2MCount * sizeof (UINT64));
      *Pte4KEntries = AllocateZeroPool (*Pte4KCount * sizeof (UINT64));
      *PdeEntries   = AllocateZeroPool (*PdeCount * sizeof (UINT64));
      *GuardEntries = AllocateZeroPool (*GuardCount * sizeof (UINT64));

      Status = GetFlatPageTableData (
                 Pte1GCount,
                 Pte2MCount,
                 Pte4KCount,
                 PdeCount,
                 GuardCount,
                 *Pte1GEntries,
                 *Pte2MEntries,
                 *Pte4KEntries,
                 *PdeEntries,
                 *GuardEntries
                 );
    }
  }

  // If an error occurred, bail and free.
  if (EFI_ERROR (Status)) {
    if (*Pte1GEntries != NULL) {
      FreePool (*Pte1GEntries);
      *Pte1GEntries = NULL;
    }

    if (*Pte2MEntries != NULL) {
      FreePool (*Pte2MEntries);
      *Pte2MEntries = NULL;
    }

    if (*Pte4KEntries != NULL) {
      FreePool (*Pte4KEntries);
      *Pte4KEntries = NULL;
    }

    if (*PdeEntries != NULL) {
      FreePool (*PdeEntries);
      *PdeEntries = NULL;
    }

    if (*GuardEntries != NULL) {
      FreePool (*GuardEntries);
      *GuardEntries = NULL;
    }

    *Pte1GCount = 0;
    *Pte2MCount = 0;
    *Pte4KCount = 0;
    *PdeCount   = 0;
    *GuardCount = 0;
  }

  DEBUG ((DEBUG_ERROR, "%a - Exit... - %r\n", __FUNCTION__, Status));
  return !EFI_ERROR (Status);
}

/**
  This helper function will flush the MemoryInfoDatabase to its corresponding
  file and free all resources currently associated with it.

  @param[in]  FileName    Name of the file to be flushed to.

  @retval     EFI_SUCCESS     Database has been flushed to file.

**/
EFI_STATUS
EFIAPI
FlushAndClearMemoryInfoDatabase (
  IN CONST CHAR16  *FileName
  )
{
  // If we have database contents, flush them to the file.
  if (mMemoryInfoDatabaseSize > 0) {
    WriteBufferToFile (FileName, mMemoryInfoDatabaseBuffer, mMemoryInfoDatabaseSize);
  }

  // If we have a database, free it, and reset all counters.
  if (mMemoryInfoDatabaseBuffer != NULL) {
    FreePool (mMemoryInfoDatabaseBuffer);
    mMemoryInfoDatabaseBuffer = NULL;
  }

  mMemoryInfoDatabaseAllocSize = 0;
  mMemoryInfoDatabaseSize      = 0;

  return EFI_SUCCESS;
} // FlushAndClearMemoryInfoDatabase()

/*
  Writes the NULL page and stack information to the memory info database
 */
VOID
EFIAPI
SpecialMemoryDump (
  VOID
  )
{
  CHAR8                      TempString[MAX_STRING_SIZE];
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryHob;
  EFI_STATUS                 Status;
  LIST_ENTRY                 *List;
  CPU_MP_DEBUG_PROTOCOL      *Entry;
  EFI_PHYSICAL_ADDRESS       StackBase;
  UINT64                     StackLength;

  // Capture the NULL address
  AsciiSPrint (
    TempString,
    MAX_STRING_SIZE,
    "Null,0x%016lx\n",
    NULL
    );
  AppendToMemoryInfoDatabase (TempString);

  Hob.Raw = GetHobList ();

  while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw)) != NULL) {
    MemoryHob = Hob.MemoryAllocation;
    if (CompareGuid (&gEfiHobMemoryAllocStackGuid, &MemoryHob->AllocDescriptor.Name)) {
      StackBase   = (EFI_PHYSICAL_ADDRESS)((MemoryHob->AllocDescriptor.MemoryBaseAddress / EFI_PAGE_SIZE) * EFI_PAGE_SIZE);
      StackLength = (EFI_PHYSICAL_ADDRESS)(EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (MemoryHob->AllocDescriptor.MemoryLength)));

      // Capture the stack guard
      if (gDxeMps.CpuStackGuard == TRUE) {
        AsciiSPrint (
          TempString,
          MAX_STRING_SIZE,
          "StackGuard,0x%016lx,0x%x\n",
          StackBase,
          EFI_PAGE_SIZE
          );
        AppendToMemoryInfoDatabase (TempString);
        StackBase   += EFI_PAGE_SIZE;
        StackLength -= EFI_PAGE_SIZE;
      }

      // Capture the stack
      AsciiSPrint (
        TempString,
        MAX_STRING_SIZE,
        "Stack,0x%016lx,0x%016lx\n",
        StackBase,
        StackLength
        );
      AppendToMemoryInfoDatabase (TempString);

      break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  Status = PopulateCpuMpDebugProtocol ();

  // The protocol should only be published if CpuStackGuard is active
  if (!EFI_ERROR (Status)) {
    for (List = mCpuMpDebugProtocol->Link.ForwardLink; List != &mCpuMpDebugProtocol->Link; List = List->ForwardLink) {
      Entry = CR (
                List,
                CPU_MP_DEBUG_PROTOCOL,
                Link,
                CPU_MP_DEBUG_SIGNATURE
                );
      StackBase   = (EFI_PHYSICAL_ADDRESS)((Entry->ApStackBuffer / EFI_PAGE_SIZE) * EFI_PAGE_SIZE);
      StackLength = (EFI_PHYSICAL_ADDRESS)(EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (Entry->ApStackSize)));

      if (!Entry->IsSwitchStack) {
        if (gDxeMps.CpuStackGuard == TRUE) {
          // Capture the AP stack guard
          AsciiSPrint (
            TempString,
            MAX_STRING_SIZE,
            "ApStackGuard,0x%016lx,0x%016lx,0x%x\n",
            StackBase,
            EFI_PAGE_SIZE,
            Entry->CpuNumber
            );
          AppendToMemoryInfoDatabase (TempString);

          StackBase   += EFI_PAGE_SIZE;
          StackLength -= EFI_PAGE_SIZE;
        }

        // Capture the AP stack
        AsciiSPrint (
          TempString,
          MAX_STRING_SIZE,
          "ApStack,0x%016lx,0x%016lx,0x%x\n",
          StackBase,
          StackLength,
          Entry->CpuNumber
          );
        AppendToMemoryInfoDatabase (TempString);
      } else {
        // Capture the AP switch stack
        AsciiSPrint (
          TempString,
          MAX_STRING_SIZE,
          "ApSwitchStack,0x%016lx,0x%016lx,0x%x\n",
          StackBase,
          StackLength,
          Entry->CpuNumber
          );
        AppendToMemoryInfoDatabase (TempString);
      }
    }
  }
}

/**
   Dumps paging information to open EFI_FILE Fs_Handle if provided and the EFI partition otherwise.

  @param[in]  Fs_Handle File handle to deposit the paging audit info

**/
VOID
EFIAPI
DumpPagingInfo (
  IN      EFI_FILE  *Fs_Handle
  )
{
  EFI_STATUS  Status        = EFI_SUCCESS;
  UINTN       Pte1GCount    = 0;
  UINTN       Pte2MCount    = 0;
  UINTN       Pte4KCount    = 0;
  UINTN       PdeCount      = 0;
  UINTN       GuardCount    = 0;
  UINT64      *Pte1GEntries = NULL;
  UINT64      *Pte2MEntries = NULL;
  UINT64      *Pte4KEntries = NULL;
  UINT64      *PdeEntries   = NULL;
  UINT64      *GuardEntries = NULL;
  CHAR8       TempString[MAX_STRING_SIZE];

  if (EFI_ERROR (PopulateHeapGuardDebugProtocol ())) {
    DEBUG ((DEBUG_ERROR, "%a - Error finding heap guard debug protocol\n", __FUNCTION__));
  }

  if (Fs_Handle != NULL) {
    mFs_Handle = Fs_Handle;
  } else {
    Status = OpenVolumeSFS (&mFs_Handle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a error opening sfs volume - %r\n", __FUNCTION__, Status));
      return;
    }
  }

  if (LoadFlatPageTableData (
        &Pte1GCount,
        &Pte2MCount,
        &Pte4KCount,
        &PdeCount,
        &GuardCount,
        &Pte1GEntries,
        &Pte2MEntries,
        &Pte4KEntries,
        &PdeEntries,
        &GuardEntries
        ))
  {
    CreateAndWriteFileSFS (mFs_Handle, L"1G.dat", Pte1GCount * sizeof (UINT64), Pte1GEntries);
    CreateAndWriteFileSFS (mFs_Handle, L"2M.dat", Pte2MCount * sizeof (UINT64), Pte2MEntries);
    CreateAndWriteFileSFS (mFs_Handle, L"4K.dat", Pte4KCount * sizeof (UINT64), Pte4KEntries);
    CreateAndWriteFileSFS (mFs_Handle, L"PDE.dat", PdeCount * sizeof (UINT64), PdeEntries);

    // Only populate guard pages when function call is successful
    for (UINT64 i = 0; i < GuardCount; i++) {
      AsciiSPrint (
        TempString,
        MAX_STRING_SIZE,
        "GuardPage,0x%016lx\n",
        GuardEntries[i]
        );
      AppendToMemoryInfoDatabase (TempString);
    }
  } else {
    DEBUG ((DEBUG_ERROR, "%a - LoadFlatPageTableData returned with failure, bail from here!\n", __FUNCTION__));
    goto Cleanup;
  }

  FlushAndClearMemoryInfoDatabase (L"GuardPage");
  DumpProcessorSpecificHandlers ();
  MemoryMapDumpHandler ();
  LoadedImageTableDump ();
  MemoryAttributesTableDump ();
  SpecialMemoryDump ();
  FlushAndClearMemoryInfoDatabase (L"MemoryInfoDatabase");
  DumpPlatforminfo ();

Cleanup:
  if (Pte1GEntries != NULL) {
    FreePool (Pte1GEntries);
  }

  if (Pte2MEntries != NULL) {
    FreePool (Pte2MEntries);
  }

  if (Pte4KEntries != NULL) {
    FreePool (Pte4KEntries);
  }

  if (PdeEntries != NULL) {
    FreePool (PdeEntries);
  }

  if (GuardEntries != NULL) {
    FreePool (GuardEntries);
  }
}
