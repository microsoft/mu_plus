/** @file -- DxePagingAuditCommon.h

This DXE Driver writes page table and memory map information to SFS when triggered
by an event.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PAGING_AUDIT_COMMON_H_
#define _PAGING_AUDIT_COMMON_H_

#include <Uefi.h>
#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Guid/EventGroup.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/MemoryProtectionDebug.h>
#include <Register/Cpuid.h>
#include <Register/Amd/Cpuid.h>

#include <Library/DevicePathLib.h>
#include <Guid/DebugImageInfoTable.h>
#include <Guid/MemoryAttributesTable.h>

#define    MEM_INFO_DATABASE_REALLOC_CHUNK    0x1000
#define    MEM_INFO_DATABASE_MAX_STRING_SIZE  0x400
#define    MAX_STRING_SIZE                    0x1000

#define IndexToAddress(a, b, c, d)  ((UINT64) ((UINT64)a << 39) + ((UINT64)b << 30) + ((UINT64)c <<  21) + ((UINT64)d << 12))

#define TSEG_EFI_MEMORY_TYPE  (EfiMaxMemoryType + 1)
#define NONE_GCD_MEMORY_TYPE  (EfiGcdMemoryTypeMaximum + 1)
#define NONE_EFI_MEMORY_TYPE  (EfiMaxMemoryType + 2)

/**
  Calculate the maximum support address.

  @return the maximum support address.
**/
UINT8
CalculateMaximumSupportAddress (
  VOID
  );

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
  );

/**
   Dump platform specific handler. Created handler(s) need to be compliant with
   Windows\PagingReportGenerator.py, i.e. TSEG.

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
DumpProcessorSpecificHandlers (
  IN  BOOLEAN  AllowAllocation,
  OUT UINTN    *StringLength
  );

/**
   Dumps paging information to open EFI_FILE Fs_Handle if provided and the EFI partition otherwise.

  @param[in]  Fs_Handle File handle to deposit the paging audit info
**/
VOID
EFIAPI
DumpPagingInfo (
  IN      EFI_FILE  *Fs_Handle
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  This helper function walks the page tables to retrieve:
  - a count of each entry
  - a count of each directory entry
  - [optional] a flat list of each entry

  @param[in, out]   Pte1GCount, Pte2MCount, Pte4KCount
      On input, the number of entries that can fit in the corresponding buffer (if provided).
      It is expected that this will be zero if the corresponding buffer is NULL.
      On output, the number of entries that were encountered in the page table.
  @param[out]       Pte1GEntries, Pte2MEntries, Pte4KEntries
      A buffer which will be filled with the entries that are encountered in the tables.

  @retval     EFI_SUCCESS             All requested data has been returned.
  @retval     EFI_INVALID_PARAMETER   One or more of the count parameter pointers is NULL.
  @retval     EFI_INVALID_PARAMETER   Presence of buffer counts and pointers is incongruent.
  @retval     EFI_BUFFER_TOO_SMALL    One or more of the buffers was insufficient to hold
                                      all of the entries in the page tables. The counts
                                      have been updated with the total number of entries
                                      encountered.

**/
EFI_STATUS
EFIAPI
GetFlatPageTableData (
  IN OUT UINTN  *Pte1GCount,
  IN OUT UINTN  *Pte2MCount,
  IN OUT UINTN  *Pte4KCount,
  IN OUT UINTN  *GuardCount,
  OUT UINT64    *Pte1GEntries,
  OUT UINT64    *Pte2MEntries,
  OUT UINT64    *Pte4KEntries,
  OUT UINT64    *GuardEntries
  );

/**
  Dumps platform info required to correctly parse the pages (architecture,
  execution level, etc.)

  @retval EFI_SUCCESS           The platform info was successfully dumped to the associated
                                data file.
  @retval        EFI_ABORTED            An error occurred while opening the SFS volume.
  @retval        Others                 The return value of CreateAndWriteFileSFS()
**/
EFI_STATUS
EFIAPI
DumpPlatforminfo (
  VOID
  );

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
  );

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
  );

#endif // _PAGING_AUDIT_COMMON_H_
