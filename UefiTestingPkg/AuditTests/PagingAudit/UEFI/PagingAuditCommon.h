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
  );

/**
   Dump platform specific handler. Created handler(s) need to be compliant with
   Windows\PagingReportGenerator.py, i.e. TSEG.
**/
VOID
EFIAPI
DumpProcessorSpecificHandlers (
  VOID
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
  );

/**
 * @brief      Writes the UEFI memory map to file.
 */
VOID
EFIAPI
MemoryMapDumpHandler (
  VOID
  );

/**
 * @brief      Writes the name, base, and limit of each image in the image table to a file.
 */
VOID
EFIAPI
LoadedImageTableDump (
  VOID
  );

/**
 * @brief      Writes the MemoryAttributesTable to a file.
 */
VOID
EFIAPI
MemoryAttributesTableDump (
  VOID
  );

/**
  This helper function walks the page tables to retrieve:
  - a count of each entry
  - a count of each directory entry
  - [optional] a flat list of each entry
  - [optional] a flat list of each directory entry

  @param[in, out]   Pte1GCount, Pte2MCount, Pte4KCount, PdeCount
      On input, the number of entries that can fit in the corresponding buffer (if provided).
      It is expected that this will be zero if the corresponding buffer is NULL.
      On output, the number of entries that were encountered in the page table.
  @param[out]       Pte1GEntries, Pte2MEntries, Pte4KEntries, PdeEntries
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
  IN OUT UINTN  *PdeCount,
  IN OUT UINTN  *GuardCount,
  OUT UINT64    *Pte1GEntries,
  OUT UINT64    *Pte2MEntries,
  OUT UINT64    *Pte4KEntries,
  OUT UINT64    *PdeEntries,
  OUT UINT64    *GuardEntries
  );

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
  );

/**
  Dumps platform info required to correctly parse the pages (architecture,
  execution level, etc.)
**/
VOID
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
