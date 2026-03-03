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

typedef enum {
  Entry1g = 0,
  Entry2m,
  Entry4k,
  EntryGuard,
  EntryMax
} ENTRY;

UINT64  *mPteEntries[4]    = { NULL, NULL, NULL, NULL };
CHAR16  *mPteFileNames[4]  = { L"1G", L"2M", L"4K", L"GuardPage" };
UINTN   mPteCounts[4]      = { 0, 0, 0, 0 };
UINTN   mPteBufferSizes[4] = { 0, 0, 0, 0 };

EFI_MEMORY_DESCRIPTOR  *mMemoryMap          = NULL;
UINTN                  mMemoryMapSize       = 0;
UINTN                  mMemoryMapBufferSize = 0;

EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *mEfiMemorySpaceMap              = NULL;
UINTN                            mNumEfiMemorySpaceMapDescriptors = 0;
UINTN                            mEfiMemorySpaceMapDescriptorSize = 0;

CHAR8  *mGuardPageBuffer    = NULL;
UINTN  mGuardPageStringSize = 0;
UINTN  mGuardPageAllocSize  = 0;

MEMORY_PROTECTION_DEBUG_PROTOCOL  *mMemoryProtectionProtocol = NULL;
CPU_MP_DEBUG_PROTOCOL             *mCpuMpDebugProtocol       = NULL;
EFI_FILE                          *mFs_Handle                = NULL;

CHAR8  *mMemoryInfoDatabaseBuffer   = NULL;
UINTN  mMemoryInfoDatabaseSize      = 0;
UINTN  mMemoryInfoDatabaseAllocSize = 0;

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

  @param[in]  DatabaseString    The string to be added to the memory info database.
  @param[in]  AllowAllocation   If TRUE, then this function will allocate memory for the
                                database buffer if it is not large enough to hold the input
                                string. If FALSE, then this function will return an error
                                if the database buffer is not large enough to hold the input
                                string.

  @retval     EFI_SUCCESS           String was successfully added.
  @retval     EFI_OUT_OF_RESOURCES  AllowAllocation is TRUE but the call to allocate memory
                                    failed.
  @retval     EFI_BUFFER_TOO_SMALL  The database buffer is not large enough to hold the input
                                    string and AllowAllocation is FALSE.
  @retval     EFI_NOT_STARTED       The memory info database buffer has not been allocated
                                    and AllowAllocation is FALSE.
**/
EFI_STATUS
EFIAPI
AppendToMemoryInfoDatabase (
  IN CONST CHAR8  *DatabaseString,
  IN BOOLEAN      AllowAllocation
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINTN       NewStringSize, NewDatabaseSize;
  CHAR8       *NewDatabaseBuffer;

  if ((DatabaseString == NULL) || (DatabaseString[0] == '\0')) {
    return EFI_SUCCESS;
  }

  if (mMemoryInfoDatabaseBuffer == NULL) {
    if (AllowAllocation) {
      mMemoryInfoDatabaseBuffer    = AllocatePool (MEM_INFO_DATABASE_REALLOC_CHUNK);
      mMemoryInfoDatabaseAllocSize = MEM_INFO_DATABASE_REALLOC_CHUNK;
    } else {
      return EFI_NOT_STARTED;
    }
  }

  // Determine the length of the incoming string.
  // NOTE: This size includes the NULL terminator.
  NewStringSize   = AsciiStrnSizeS (DatabaseString, MEM_INFO_DATABASE_MAX_STRING_SIZE);
  NewStringSize   = NewStringSize - sizeof (CHAR8);  // Remove NULL.
  NewDatabaseSize = NewStringSize + mMemoryInfoDatabaseSize;
  if (NewDatabaseSize > mMemoryInfoDatabaseAllocSize) {
    if (AllowAllocation) {
      NewDatabaseBuffer = ReallocatePool (
                            mMemoryInfoDatabaseAllocSize,
                            mMemoryInfoDatabaseAllocSize + MEM_INFO_DATABASE_REALLOC_CHUNK,
                            mMemoryInfoDatabaseBuffer
                            );

      if (NewDatabaseBuffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
      } else {
        mMemoryInfoDatabaseBuffer     = NewDatabaseBuffer;
        mMemoryInfoDatabaseAllocSize += MEM_INFO_DATABASE_REALLOC_CHUNK;
      }
    } else {
      Status = EFI_BUFFER_TOO_SMALL;
    }
  }

  // Copy the new string to the end of the buffer and update the size.
  if (!EFI_ERROR (Status)) {
    CopyMem (&mMemoryInfoDatabaseBuffer[mMemoryInfoDatabaseSize], DatabaseString, NewStringSize);
    mMemoryInfoDatabaseSize = NewDatabaseSize;
  }

  return Status;
}

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

  if ((Fs_Handle == NULL) || (FileName == NULL) || (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

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
  Writes the input buffer to a .dat file with the input file name.

  @param[in]     FileName     The name of the file being written to.
  @param[in]     Buffer       The buffer to write to file.
  @param[in]     BufferSize   Size of the buffer.

  @retval        EFI_SUCCESS            The file was successfully written.
  @retval        EFI_INVALID_PARAMETER  One or more input parameters were invalid.
  @retval        EFI_ABORTED            An error occurred while opening the SFS volume.
  @retval        Others                 The return value of CreateAndWriteFileSFS()
**/
EFI_STATUS
EFIAPI
WriteBufferToFile (
  IN CONST CHAR16  *FileName,
  IN       VOID    *Buffer,
  IN       UINTN   BufferSize
  )
{
  EFI_STATUS  Status;
  CHAR16      FileNameAndExt[MAX_STRING_SIZE];

  if ((FileName == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (mFs_Handle == NULL) {
    Status = OpenVolumeSFS (&mFs_Handle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a error opening sfs volume - %r\n", __func__, Status));
      return EFI_ABORTED;
    }
  }

  // Calculate final file name.
  ZeroMem (FileNameAndExt, sizeof (CHAR16) * MAX_STRING_SIZE);
  UnicodeSPrint (FileNameAndExt, MAX_STRING_SIZE, L"%s.dat", FileName);

  Status = CreateAndWriteFileSFS (mFs_Handle, FileNameAndExt, BufferSize, Buffer);
  DEBUG ((DEBUG_ERROR, "%a Writing file %s - %r\n", __func__, FileNameAndExt, Status));

  return Status;
}

/**
  Writes the memory attributes table to MAT.dat.

  @retval EFI_SUCCESS           The MAT.dat was successfully written.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.
  @retval EFI_ABORTED           Failed to fetch the MAT or open the SFS volume.
  @retval Others                The return value of CreateAndWriteFileSFS()
**/
EFI_STATUS
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
    return EFI_ABORTED;
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
    return EFI_OUT_OF_RESOURCES;
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
  Status = WriteBufferToFile (L"MAT", Buffer, BufferSize-1);

  FreePool (Buffer);
  return Status;
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
  Updates the memory map to contain contiguous entries from StartOfAddressSpace to
  max(EndOfAddressSpace, address + length of the final memory map entry). If DetermineSize
  is TRUE, then this function will just determine the required buffer size for the output
  memory map.

  @param[in, out] MemoryMapSize         Size, in bytes, of MemoryMap
  @param[in, out] MemoryMap             IN:  Pointer to the EFI memory map which will have all gaps filled. The
                                             original buffer will be freed and updated to a newly allocated buffer
                                        OUT: Pointer to the pointer to a SORTED memory map
  @param[in]      DescriptorSize        Size, in bytes, of each descriptor region in the array. NOTE: This is not
                                        sizeof (EFI_MEMORY_DESCRIPTOR).
  @param[in]      InsertionPoint        Pointer to where new memory map entries should
                                        be inserted. This insertion point should be between MemoryMap
                                        and MemoryMap + MemoryMapBufferSize. If this is NULL, then
                                        DetermineSize must be TRUE.
  @param[in]      StartOfAddressSpace   Starting address from which there should be contiguous entries
  @param[in]      EndOfAddressSpace     Ending address at which the memory map should at least reach
  @param[in]      DetermineSize         If TRUE, then this function will only determine the required
                                        buffer size for the output memory map. If FALSE, then this
                                        function will fill in the memory map

  @retval EFI_SUCCESS                   Successfully filled in the memory map
  @retval EFI_INVALID_PARAMETER         An input parameter was invalid.
**/
EFI_STATUS
FillInMemoryMap (
  IN OUT    UINTN                  *MemoryMapSize,
  IN OUT    EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN        UINTN                  MemoryMapBufferSize,
  IN        UINTN                  DescriptorSize,
  IN        EFI_MEMORY_DESCRIPTOR  *InsertionPoint,
  IN        EFI_PHYSICAL_ADDRESS   StartOfAddressSpace,
  IN        EFI_PHYSICAL_ADDRESS   EndOfAddressSpace,
  IN OUT    BOOLEAN                DetermineSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapCurrent, *MemoryMapEnd;
  EFI_PHYSICAL_ADDRESS   LastEntryEnd, NextEntryStart;
  UINTN                  AdditionalEntriesCount;

  if ((MemoryMap == NULL) ||
      (MemoryMapSize == NULL) ||
      (!DetermineSize && (InsertionPoint == NULL)))
  {
    DEBUG ((DEBUG_ERROR, "%a - Function had NULL input(s)!\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((*MemoryMapSize == 0)) {
    DEBUG ((DEBUG_ERROR, "%a - MemoryMapSize is zero!\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((!DetermineSize) &&
      !(((UINTN)MemoryMap < (UINTN)InsertionPoint) &&
        ((UINTN)InsertionPoint >= (UINTN)MemoryMap + *MemoryMapSize) &&
        ((UINTN)InsertionPoint < (UINTN)MemoryMap + MemoryMapBufferSize)))
  {
    DEBUG ((DEBUG_ERROR, "%a - Input InsertionPoint is Invalid!\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  SortMemoryMap (MemoryMap, *MemoryMapSize, DescriptorSize);
  if ((InsertionPoint != NULL) && !DetermineSize) {
    ZeroMem (InsertionPoint, MemoryMapBufferSize - *MemoryMapSize);
  }

  AdditionalEntriesCount = 0;
  MemoryMapCurrent       = MemoryMap;
  MemoryMapEnd           = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + *MemoryMapSize);

  // Check if we need to insert a new entry at the start of the memory map
  if (MemoryMapCurrent->PhysicalStart > StartOfAddressSpace) {
    if (!DetermineSize) {
      FILL_MEMORY_DESCRIPTOR_ENTRY (
        InsertionPoint,
        StartOfAddressSpace,
        EfiSizeToPages (MemoryMapCurrent->PhysicalStart - StartOfAddressSpace)
        );

      InsertionPoint = NEXT_MEMORY_DESCRIPTOR (InsertionPoint, DescriptorSize);
    } else {
      AdditionalEntriesCount++;
    }
  }

  while (MemoryMapCurrent < MemoryMapEnd) {
    if (NEXT_MEMORY_DESCRIPTOR (MemoryMapCurrent, DescriptorSize) < MemoryMapEnd) {
      LastEntryEnd   = MemoryMapCurrent->PhysicalStart + EfiPagesToSize (MemoryMapCurrent->NumberOfPages);
      NextEntryStart = NEXT_MEMORY_DESCRIPTOR (MemoryMapCurrent, DescriptorSize)->PhysicalStart;
      // Check for a gap in the memory map
      if (NextEntryStart > LastEntryEnd) {
        // Fill in missing region based on the GCD Memory Map
        if (!DetermineSize) {
          FILL_MEMORY_DESCRIPTOR_ENTRY (
            InsertionPoint,
            LastEntryEnd,
            EfiSizeToPages (NextEntryStart - LastEntryEnd)
            );
          InsertionPoint = NEXT_MEMORY_DESCRIPTOR (InsertionPoint, DescriptorSize);
        } else {
          AdditionalEntriesCount++;
        }
      }
    }

    MemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (MemoryMapCurrent, DescriptorSize);
  }

  LastEntryEnd = PREVIOUS_MEMORY_DESCRIPTOR (MemoryMapCurrent, DescriptorSize)->PhysicalStart +
                 EfiPagesToSize (PREVIOUS_MEMORY_DESCRIPTOR (MemoryMapCurrent, DescriptorSize)->NumberOfPages);

  // Check if we need to insert a new entry at the end of the memory map
  if (EndOfAddressSpace > LastEntryEnd) {
    if (!DetermineSize) {
      FILL_MEMORY_DESCRIPTOR_ENTRY (
        InsertionPoint,
        LastEntryEnd,
        EfiSizeToPages (EndOfAddressSpace - LastEntryEnd)
        );
      InsertionPoint = NEXT_MEMORY_DESCRIPTOR (InsertionPoint, DescriptorSize);
    } else {
      AdditionalEntriesCount++;
    }
  }

  if (DetermineSize) {
    *MemoryMapSize = *MemoryMapSize + (AdditionalEntriesCount * DescriptorSize);
  } else {
    *MemoryMapSize = (UINTN)InsertionPoint - (UINTN)MemoryMap;
    SortMemoryMap (MemoryMap, *MemoryMapSize, DescriptorSize);
  }

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

  @retval Remaining pages not covered by a GCD Memory region
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
  Allocate memory to hold the hybrid EFI/GCD memory map.

  @retval EFI_SUCCESS           Memory was successfully allocated.
  @retval EFI_OUT_OF_RESOURCES  Memory could not be allocated.
  @retval Others                The return status of a boot services or DXE services call.
**/
EFI_STATUS
EFIAPI
AllocateMemoryMapBuffer (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       MapKey;
  UINTN       MemoryMapDescriptorSize;
  UINT32      DescriptorVersion;

  // GetMemorySpaceMap() will allocate memory
  Status = gDS->GetMemorySpaceMap (&mNumEfiMemorySpaceMapDescriptors, &mEfiMemorySpaceMap);

  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto FailureFreeMem;
  }

  mEfiMemorySpaceMapDescriptorSize = sizeof (EFI_GCD_MEMORY_SPACE_DESCRIPTOR);
  SortMemorySpaceMap (mEfiMemorySpaceMap, mNumEfiMemorySpaceMapDescriptors * mEfiMemorySpaceMapDescriptorSize, mEfiMemorySpaceMapDescriptorSize);

  mMemoryMapSize = 0;
  Status         = gBS->GetMemoryMap (
                          &mMemoryMapSize,
                          mMemoryMap,
                          &MapKey,
                          &MemoryMapDescriptorSize,
                          &DescriptorVersion
                          );
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  do {
    mMemoryMap = (EFI_MEMORY_DESCRIPTOR *)AllocatePool (mMemoryMapSize);
    if (mMemoryMap == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      ASSERT_EFI_ERROR (Status);
      goto FailureFreeMem;
    }

    Status = gBS->GetMemoryMap (
                    &mMemoryMapSize,
                    mMemoryMap,
                    &MapKey,
                    &MemoryMapDescriptorSize,
                    &DescriptorVersion
                    );
    if (EFI_ERROR (Status)) {
      FreePool (mMemoryMap);
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);

  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto FailureFreeMem;
  }

  // Determine how large the filled in memory map will be.
  Status = FillInMemoryMap (
             &mMemoryMapSize,
             mMemoryMap,
             mMemoryMapSize,
             MemoryMapDescriptorSize,
             NULL,
             mEfiMemorySpaceMap->BaseAddress,
             mEfiMemorySpaceMap[mNumEfiMemorySpaceMapDescriptors - 1].BaseAddress + mEfiMemorySpaceMap[mNumEfiMemorySpaceMapDescriptors - 1].Length,
             TRUE
             );

  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto FailureFreeMem;
  }

  // mMemoryMapSize now contains the size of the filled in memory map. Increase
  // it by 20% to account for any additional entries that may be required after
  // other buffers are allocated.
  mMemoryMapSize      += (mMemoryMapSize / 5);
  mMemoryMapBufferSize = mMemoryMapSize;
  FreePool (mMemoryMap);
  mMemoryMap = AllocateZeroPool (mMemoryMapBufferSize);

  if (mMemoryMap == NULL) {
    ASSERT (mMemoryMap != NULL);
    Status = EFI_OUT_OF_RESOURCES;
    goto FailureFreeMem;
  }

  return Status;

FailureFreeMem:
  if (mMemoryMap != NULL) {
    FreePool (mMemoryMap);
  }

  if (mEfiMemorySpaceMap != NULL) {
    FreePool (mEfiMemorySpaceMap);
  }

  return Status;
}

/**
  Dumps the memory map to the memory info database string. If DetermineStrSize is
  TRUE, then this function will add the required string size to the global
  mMemoryInfoDatabaseAllocSize.

  @param[in]  AllowAllocation   If TRUE, then this function will allocate memory for the
                                database buffer if it is not large enough to hold the input
                                string. If FALSE, then this function will return an error
                                if the database buffer is not large enough to hold the input
                                string.
  @param[out] StringLength      The length of the string that was or would have been written
                                to the memory info database buffer.

  @retval     EFI_SUCCESS             The platform specific info was successfully dumped to
                                      the memory info database buffer.
  @retval     EFI_OUT_OF_RESOURCES    The database buffer is not large enough to hold the
                                      platform specific info and AllowAllocation is FALSE.
  @retval     EFI_NOT_STARTED         The memory info database buffer has not been allocated.
  @retval     EFI_BUFFER_TOO_SMALL    The database buffer is not large enough to hold the
                                      platform specific info and AllowAllocation is FALSE.
  @retval     EFI_INVALID_PARAMETER   StringLength is NULL.
**/
EFI_STATUS
EFIAPI
MemoryMapDumpHandler (
  IN  BOOLEAN  AllowAllocation,
  OUT UINTN    *StringLength
  )
{
  EFI_STATUS             Status;
  UINTN                  MapKey;
  UINTN                  MemoryMapDescriptorSize;
  UINT32                 DescriptorVersion;
  CHAR8                  TempString[MAX_STRING_SIZE];
  EFI_GCD_MEMORY_TYPE    MemorySpaceType;
  UINT64                 RemainingPages;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapNext;

  if ((mMemoryMap == NULL) || (mMemoryMapBufferSize == 0) ||
      (mEfiMemorySpaceMap == NULL) || (StringLength == NULL))
  {
    ASSERT (mMemoryMap != NULL);
    ASSERT (mMemoryMapBufferSize != 0);
    ASSERT (mEfiMemorySpaceMap != NULL);
    ASSERT (StringLength != NULL);
    return EFI_INVALID_PARAMETER;
  }

  *StringLength = 0;

  if (EFI_ERROR (PopulateHeapGuardDebugProtocol ())) {
    DEBUG ((DEBUG_ERROR, "%a - Error finding heap guard debug protocol\n", __func__));
  }

  mMemoryMapSize = mMemoryMapBufferSize;
  Status         = gBS->GetMemoryMap (
                          &mMemoryMapSize,
                          mMemoryMap,
                          &MapKey,
                          &MemoryMapDescriptorSize,
                          &DescriptorVersion
                          );

  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    Status = EFI_ABORTED;
    goto Failure;
  }

  // Fill gaps in the memory map
  Status = FillInMemoryMap (
             &mMemoryMapSize,
             mMemoryMap,
             mMemoryMapBufferSize,
             MemoryMapDescriptorSize,
             (EFI_MEMORY_DESCRIPTOR *)(((UINT8 *)mMemoryMap) + mMemoryMapSize),
             mEfiMemorySpaceMap->BaseAddress,
             mEfiMemorySpaceMap[mNumEfiMemorySpaceMapDescriptors - 1].BaseAddress + mEfiMemorySpaceMap[mNumEfiMemorySpaceMapDescriptors - 1].Length,
             FALSE
             );

  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    Status = EFI_ABORTED;
    goto Failure;
  }

  MemoryMapEnd  = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mMemoryMap + mMemoryMapSize);
  MemoryMapNext = mMemoryMap;

  while (MemoryMapNext < MemoryMapEnd) {
    RemainingPages = GetOverlappingMemorySpaceRegion (
                       mEfiMemorySpaceMap,
                       mNumEfiMemorySpaceMapDescriptors,
                       MemoryMapNext->PhysicalStart,
                       MemoryMapNext->NumberOfPages,
                       &MemorySpaceType
                       );

    AsciiSPrint (
      TempString,
      MAX_STRING_SIZE,
      "MemoryMap,0x%016lx,0x%016lx,0x%016lx,0x%016lx,0x%016lx,0x%x\n",
      MemoryMapNext->Type,
      MemoryMapNext->PhysicalStart,
      MemoryMapNext->VirtualStart,
      MemoryMapNext->NumberOfPages - RemainingPages,
      MemoryMapNext->Attribute,
      MemorySpaceType
      );
    *StringLength += AsciiStrLen (TempString);
    Status         = AppendToMemoryInfoDatabase (TempString, AllowAllocation);

    if (RemainingPages > 0) {
      MemoryMapNext->PhysicalStart += EfiPagesToSize (MemoryMapNext->NumberOfPages - RemainingPages);
      MemoryMapNext->NumberOfPages  = RemainingPages;
      if (MemoryMapNext->VirtualStart > 0) {
        MemoryMapNext->VirtualStart += EfiPagesToSize (MemoryMapNext->NumberOfPages - RemainingPages);
      }
    } else {
      MemoryMapNext = NEXT_MEMORY_DESCRIPTOR (MemoryMapNext, MemoryMapDescriptorSize);
    }
  }

  return Status;

Failure:
  if (mMemoryMap != NULL) {
    ZeroMem (mMemoryMap, mMemoryMapBufferSize);
  }

  return Status;
}

/**
  Dumps the guard page entries to the memory info database string. If DetermineStrSize is TRUE,
  then this function will add the required string size to the global mMemoryInfoDatabaseAllocSize.

  @param[in]  GuardPageEntries  The buffer containing the guard page entries
  @param[in]  GuardPageCount    The number of guard page entries in the buffer
  @param[in]  DetermineStrSize  If TRUE, then this function will only determine the required
                                buffer size for the output and add it to mMemoryInfoDatabaseAllocSize.

  @retval EFI_SUCCESS           Guard page entries were successfully dumped or the alloc size was calculated.
  @retval EFI_INVALID_PARAMETER GuardPageEntries was NULL.
  @retval EFI_OUT_OF_RESOURCES  The size of the database buffer was not large enough to hold the guard page entries.
**/
EFI_STATUS
GuardPageDump (
  IN UINT64   *GuardPageEntries,
  IN UINTN    GuardPageCount,
  IN BOOLEAN  DetermineStrSize
  )
{
  UINTN       Index;
  EFI_STATUS  Status;
  CHAR8       TempString[MAX_STRING_SIZE];

  Status = EFI_SUCCESS;

  if (!DetermineStrSize && (GuardPageEntries == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < GuardPageCount; Index++) {
    if (DetermineStrSize) {
      mGuardPageAllocSize += AsciiSPrint (
                               TempString,
                               MAX_STRING_SIZE,
                               "GuardPage,0x%016lx\n",
                               0x0
                               );
    } else {
      mGuardPageStringSize += AsciiSPrint (
                                (CHAR8 *)(mGuardPageBuffer + mGuardPageStringSize),
                                MAX_STRING_SIZE,
                                "GuardPage,0x%016lx\n",
                                GuardPageEntries[Index]
                                );
      if (mGuardPageStringSize > mGuardPageAllocSize) {
        Status = EFI_OUT_OF_RESOURCES;
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
    }
  }

  return Status;
}

/**
  Dumps the loaded image information to the memory info database string.
  If DetermineStrSize is TRUE, then this function will add the required
  string size to the global mMemoryInfoDatabaseAllocSize.

  @param[in]  AllowAllocation   If TRUE, then this function will allocate memory for the
                                database buffer if it is not large enough to hold the input
                                string. If FALSE, then this function will return an error
                                if the database buffer is not large enough to hold the input
                                string.
  @param[out] StringLength      The length of the string that was or would have been written
                                to the memory info database buffer.

  @retval     EFI_SUCCESS             The platform specific info was successfully dumped to
                                      the memory info database buffer.
  @retval     EFI_OUT_OF_RESOURCES    The database buffer is not large enough to hold the
                                      platform specific info and AllowAllocation is FALSE.
  @retval     EFI_NOT_STARTED         The memory info database buffer has not been allocated.
  @retval     EFI_BUFFER_TOO_SMALL    The database buffer is not large enough to hold the
                                      platform specific info and AllowAllocation is FALSE.
  @retval     EFI_INVALID_PARAMETER   StringLength is NULL.
**/
EFI_STATUS
EFIAPI
LoadedImageTableDump (
  IN  BOOLEAN  AllowAllocation,
  OUT UINTN    *StringLength
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

  if (StringLength == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *StringLength = 0;

  //
  // locate DebugImageInfoTable
  //
  Status = EfiGetSystemConfigurationTable (&gEfiDebugImageInfoTableGuid, (VOID **)&TableHeader);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to retrieve loaded image table %r", Status));
    return EFI_ABORTED;
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

    *StringLength += AsciiStrLen (TempString);
    Status         = AppendToMemoryInfoDatabase (TempString, AllowAllocation);
  }

  return Status;
}

/**

  Opens the SFS volume and if successful, returns a FS handle to the opened volume.

  @param[out]   Fs_Handle       Handle to the opened volume.

  @retval       EFI_SUCCESS     The FS volume was opened successfully.
  @retval       Others          The operation failed.
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

/**
  Parse the page/translation table entries.

  @param[out]   Pte1GCount      The number of 1G page table entries
  @param[out]   Pte2MCount      The number of 2M page table entries
  @param[out]   Pte4KCount      The number of 4K page table entries
  @param[out]   GuardCount      The number of guard page entries
  @param[out]   Pte1GEntries    The 1G page table entries
  @param[out]   Pte2MEntries    The 2M page table entries
  @param[out]   Pte4KEntries    The 4K page table entries
  @param[out]   GuardEntries    The guard page entries
  @param[in]    AllocateBuffers If TRUE, this routine will allocate buffers to hold
                                the tables. If FALSE, this routine will populate the
                                input buffers with the page data.
**/
EFI_STATUS
LoadFlatPageTableData (
  OUT UINTN    *Pte1GCount,
  OUT UINTN    *Pte2MCount,
  OUT UINTN    *Pte4KCount,
  OUT UINTN    *GuardCount,
  OUT UINT64   **Pte1GEntries,
  OUT UINT64   **Pte2MEntries,
  OUT UINT64   **Pte4KEntries,
  OUT UINT64   **GuardEntries,
  IN  BOOLEAN  AllocateBuffers
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  if (AllocateBuffers) {
    *Pte1GCount = 0;
    *Pte2MCount = 0;
    *Pte4KCount = 0;
    *GuardCount = 0;
    Status      = GetFlatPageTableData (Pte1GCount, Pte2MCount, Pte4KCount, GuardCount, NULL, NULL, NULL, NULL);

    // Allocate buffers if successful.
    if (!EFI_ERROR (Status)) {
      // Increase by 20% or at least 15 entries
      (*Pte1GCount) += (*Pte1GCount / 5) < 15 ? 15 : (*Pte1GCount / 5);
      (*Pte2MCount) += (*Pte2MCount / 5) < 15 ? 15 : (*Pte2MCount / 5);
      (*Pte4KCount) += (*Pte4KCount / 5) < 15 ? 15 : (*Pte4KCount / 5);

      if (*GuardCount != 0) {
        (*GuardCount) += (*GuardCount / 5) < 15 ? 15 : (*GuardCount / 5);
      }

      *Pte1GEntries = AllocateZeroPool (*Pte1GCount * sizeof (UINT64));
      *Pte2MEntries = AllocateZeroPool (*Pte2MCount * sizeof (UINT64));
      *Pte4KEntries = AllocateZeroPool (*Pte4KCount * sizeof (UINT64));
      *GuardEntries = AllocateZeroPool (*GuardCount * sizeof (UINT64));

      // Check for errors.
      if ((*Pte1GEntries == NULL) || (*Pte2MEntries == NULL) || (*Pte4KEntries == NULL) || (*GuardEntries == NULL)) {
        // If all the buffers could not be allocated, free the ones which were and return an error status
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

        if (*GuardEntries != NULL) {
          FreePool (*GuardEntries);
          *GuardEntries = NULL;
        }

        *Pte1GCount = 0;
        *Pte2MCount = 0;
        *Pte4KCount = 0;
        *GuardCount = 0;
      } else {
        Status = EFI_SUCCESS;
      }
    }
  } else {
    Status = GetFlatPageTableData (
               Pte1GCount,
               Pte2MCount,
               Pte4KCount,
               GuardCount,
               *Pte1GEntries,
               *Pte2MEntries,
               *Pte4KEntries,
               *GuardEntries
               );
  }

  return Status;
}

/**
  Writes the NULL page and stack information to the memory info database.

  @param[in] DetermineStrSize   If TRUE, then this function will only determine the required
                                buffer size for the output and add it to mMemoryInfoDatabaseAllocSize.

  @retval EFI_SUCCESS           The NULL page and stack information was successfully written or the alloc size was calculated.
  @retval EFI_OUT_OF_RESOURCES  The size of the database buffer was not large enough to hold the special memory data.
**/
EFI_STATUS
EFIAPI
SpecialMemoryDump (
  IN  BOOLEAN  AllowAllocation,
  OUT UINTN    *StringLength
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

  if (StringLength == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *StringLength = 0;

  // Capture the NULL address
  AsciiSPrint (
    TempString,
    MAX_STRING_SIZE,
    "Null,0x%016lx\n",
    NULL
    );

  *StringLength += AsciiStrLen (TempString);
  Status         = AppendToMemoryInfoDatabase (TempString, AllowAllocation);

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
        *StringLength += AsciiStrLen (TempString);
        Status         = AppendToMemoryInfoDatabase (TempString, AllowAllocation);
        StackBase     += EFI_PAGE_SIZE;
        StackLength   -= EFI_PAGE_SIZE;
      }

      // Capture the stack
      if (StackLength > 0) {
        AsciiSPrint (
          TempString,
          MAX_STRING_SIZE,
          "Stack,0x%016lx,0x%016lx\n",
          StackBase,
          StackLength
          );
        *StringLength += AsciiStrLen (TempString);
        Status         = AppendToMemoryInfoDatabase (TempString, AllowAllocation);
      }

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
          *StringLength += AsciiStrLen (TempString);
          Status         = AppendToMemoryInfoDatabase (TempString, AllowAllocation);
          StackBase     += EFI_PAGE_SIZE;
          StackLength   -= EFI_PAGE_SIZE;
        }

        // Capture the AP stack
        if (StackLength > 0) {
          AsciiSPrint (
            TempString,
            MAX_STRING_SIZE,
            "ApStack,0x%016lx,0x%016lx,0x%x\n",
            StackBase,
            StackLength,
            Entry->CpuNumber
            );
          *StringLength += AsciiStrLen (TempString);
          Status         = AppendToMemoryInfoDatabase (TempString, AllowAllocation);
        }
      } else {
        // Capture the AP switch stack
        if (StackLength > 0) {
          AsciiSPrint (
            TempString,
            MAX_STRING_SIZE,
            "ApSwitchStack,0x%016lx,0x%016lx,0x%x\n",
            StackBase,
            StackLength,
            Entry->CpuNumber
            );
          *StringLength += AsciiStrLen (TempString);
          Status         = AppendToMemoryInfoDatabase (TempString, AllowAllocation);
        }
      }
    }
  }

  return EFI_SUCCESS;
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
  EFI_STATUS  Status = EFI_SUCCESS;
  UINTN       Index;
  UINTN       StringLength;

  if (EFI_ERROR (PopulateHeapGuardDebugProtocol ())) {
    DEBUG ((DEBUG_ERROR, "%a - Error finding heap guard debug protocol\n", __func__));
  }

  if (Fs_Handle != NULL) {
    mFs_Handle = Fs_Handle;
  } else {
    Status = OpenVolumeSFS (&mFs_Handle);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      DEBUG ((DEBUG_ERROR, "%a - error opening sfs volume - %r\n", __func__, Status));
      return;
    }
  }

  // Dump the platform info file
  Status = DumpPlatforminfo ();

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Error dumping platform info\n", __func__));
  }

  // Dump the memory attributes table file
  Status = MemoryAttributesTableDump ();

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Error dumping memory attributes table\n", __func__));
  }

  // Allocate buffers for the page table entries
  Status = LoadFlatPageTableData (
             &mPteCounts[Entry1g],
             &mPteCounts[Entry2m],
             &mPteCounts[Entry4k],
             &mPteCounts[EntryGuard],
             &mPteEntries[Entry1g],
             &mPteEntries[Entry2m],
             &mPteEntries[Entry4k],
             &mPteEntries[EntryGuard],
             TRUE
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Error allocating buffers for page table entries\n", __func__));
    ASSERT_EFI_ERROR (Status);
    return;
  }

  // Allocate buffers for the memory map
  Status = AllocateMemoryMapBuffer ();

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Error allocating buffer for the memory map\n", __func__));
    ASSERT_EFI_ERROR (Status);
    goto Cleanup;
  }

  StringLength = 0;

  // Calculate the string size of the hybrid GCD and EFI memory map
  Status = MemoryMapDumpHandler (FALSE, &StringLength);

  if (EFI_ERROR (Status) && (Status != EFI_NOT_STARTED)) {
    DEBUG ((DEBUG_ERROR, "%a - Error tabulating required string size for the memory map in the memory info database\n", __func__));
    ASSERT_EFI_ERROR (Status);
    goto Cleanup;
  }

  mMemoryInfoDatabaseAllocSize += StringLength;
  StringLength                  = 0;

  // Calculate the string size of the Loaded Image Table
  Status = LoadedImageTableDump (FALSE, &StringLength);

  if (EFI_ERROR (Status) && (Status != EFI_NOT_STARTED)) {
    DEBUG ((DEBUG_ERROR, "%a - Error tabulating required string size for the loaded image info in the memory info database\n", __func__));
    ASSERT_EFI_ERROR (Status);
    goto Cleanup;
  }

  mMemoryInfoDatabaseAllocSize += StringLength;
  StringLength                  = 0;

  // Calculate the string size of the special memory info (i.e. NULL, stack, etc.)
  Status = SpecialMemoryDump (FALSE, &StringLength);

  if (EFI_ERROR (Status) && (Status != EFI_NOT_STARTED)) {
    DEBUG ((DEBUG_ERROR, "%a - Error tabulating required string size for special memory info in the memory info database\n", __func__));
    ASSERT_EFI_ERROR (Status);
    goto Cleanup;
  }

  mMemoryInfoDatabaseAllocSize += StringLength;
  StringLength                  = 0;

  // Calculate the string size of the processor specific data
  Status = DumpProcessorSpecificHandlers (FALSE, &StringLength);

  if (EFI_ERROR (Status) && (Status != EFI_NOT_STARTED) && (Status != EFI_UNSUPPORTED)) {
    DEBUG ((DEBUG_ERROR, "%a - Error tabulating required string size for processor specific data\n", __func__));
    ASSERT_EFI_ERROR (Status);
    goto Cleanup;
  }

  mMemoryInfoDatabaseAllocSize += StringLength;
  StringLength                  = 0;

  // Allocate the memory info database buffer with 20% extra space
  mMemoryInfoDatabaseBuffer = AllocateZeroPool (mMemoryInfoDatabaseAllocSize + (mMemoryInfoDatabaseAllocSize / 5));

  if (mMemoryInfoDatabaseBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Error allocating memory info database buffer\n", __func__));
    ASSERT (mMemoryInfoDatabaseBuffer != NULL);
    goto Cleanup;
  }

  if (mPteCounts[EntryGuard] > 0) {
    // Calculate the string size of the guard page entries
    Status = GuardPageDump (mPteEntries[EntryGuard], mPteCounts[EntryGuard], TRUE);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Error tabulating required string size for the guard page info file\n", __func__));
      ASSERT_EFI_ERROR (Status);
      goto Cleanup;
    }

    // Allocate the guard page database buffer with 20% extra space
    mGuardPageBuffer = AllocateZeroPool (mGuardPageAllocSize + (mGuardPageAllocSize / 5));

    if (mGuardPageBuffer == NULL) {
      DEBUG ((DEBUG_ERROR, "%a - Error allocating buffer for the guard page string\n", __func__));
      ASSERT (mGuardPageBuffer != NULL);
      goto Cleanup;
    }
  }

  // Dump the the memory map
  Status = MemoryMapDumpHandler (FALSE, &StringLength);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Error dumping the hybrid EFI/GCD memory map to the memory info database buffer\n", __func__));
    ASSERT_EFI_ERROR (Status);
    goto Cleanup;
  }

  // Dump the Loaded Image Table
  Status = LoadedImageTableDump (FALSE, &StringLength);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Error dumping loaded image table to the memory info database buffer\n", __func__));
    ASSERT_EFI_ERROR (Status);
    goto Cleanup;
  }

  // Dump any special memory
  Status = SpecialMemoryDump (FALSE, &StringLength);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Error dumping special memory info to the memory info database buffer\n", __func__));
    ASSERT_EFI_ERROR (Status);
    goto Cleanup;
  }

  // Dump the processor specific data
  Status = DumpProcessorSpecificHandlers (FALSE, &StringLength);

  if (EFI_ERROR (Status) && (Status != EFI_UNSUPPORTED)) {
    DEBUG ((DEBUG_ERROR, "%a - Error dumping processor specific data to the memory info database buffer\n", __func__));
    ASSERT_EFI_ERROR (Status);
    goto Cleanup;
  }

  // Get the page table entries
  Status = LoadFlatPageTableData (
             &mPteCounts[Entry1g],
             &mPteCounts[Entry2m],
             &mPteCounts[Entry4k],
             &mPteCounts[EntryGuard],
             &mPteEntries[Entry1g],
             &mPteEntries[Entry2m],
             &mPteEntries[Entry4k],
             &mPteEntries[EntryGuard],
             FALSE
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Error collecting page table data\n", __func__));
    ASSERT_EFI_ERROR (Status);
    goto Cleanup;
  }

  if (mPteCounts[EntryGuard] > 0) {
    // Dump the guard page entries as ASCII to the guard page string buffer
    Status = GuardPageDump (mPteEntries[EntryGuard], mPteCounts[EntryGuard], FALSE);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Error dumping guard page entries to the guard page info file\n", __func__));
      ASSERT_EFI_ERROR (Status);
      goto Cleanup;
    }
  }

  Status = WriteBufferToFile (L"MemoryInfoDatabase", mMemoryInfoDatabaseBuffer, mMemoryInfoDatabaseSize);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to write MemoryInfoDatabase.dat!\n", __func__));
    ASSERT_EFI_ERROR (Status);
  }

  // Write the page table entries to the file buffer
  for (Index = 0; Index < EntryGuard; Index++) {
    Status = WriteBufferToFile (mPteFileNames[Index], mPteEntries[Index], mPteCounts[Index] * sizeof (UINT64));

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Error creating %s!\n", __func__, mPteFileNames[EntryGuard]));
      ASSERT_EFI_ERROR (Status);
    }
  }

  if (mGuardPageBuffer != NULL) {
    Status = WriteBufferToFile (mPteFileNames[EntryGuard], mGuardPageBuffer, mGuardPageStringSize);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Error creating %s!\n", __func__, mPteFileNames[EntryGuard]));
      ASSERT_EFI_ERROR (Status);
    }
  }

Cleanup:
  // Free the page table buffers
  for (Index = 0; Index < EntryMax; Index++) {
    if (mPteEntries[Index] != NULL) {
      FreePool (mPteEntries[Index]);
      mPteCounts[Index] = 0;
    }
  }

  if (Fs_Handle == NULL) {
    if (mFs_Handle != NULL) {
      mFs_Handle->Close (mFs_Handle);
      mFs_Handle = NULL;
    }
  }

  if (mMemoryInfoDatabaseBuffer != NULL) {
    FreePool (mMemoryInfoDatabaseBuffer);
    mMemoryInfoDatabaseBuffer    = NULL;
    mMemoryInfoDatabaseAllocSize = 0;
    mMemoryInfoDatabaseSize      = 0;
  }

  if (mMemoryMap != NULL) {
    FreePool (mMemoryMap);
    mMemoryMap           = NULL;
    mMemoryMapBufferSize = 0;
    mMemoryMapSize       = 0;
  }

  if (mEfiMemorySpaceMap != NULL) {
    FreePool (mEfiMemorySpaceMap);
    mEfiMemorySpaceMap               = NULL;
    mNumEfiMemorySpaceMapDescriptors = 0;
    mEfiMemorySpaceMapDescriptorSize = 0;
  }
}
