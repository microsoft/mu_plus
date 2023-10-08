/** @file -- PagingAuditProcessor.c

Platform specific memory audit functions.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/ArmLib.h>
#include <Protocol/MemoryAttribute.h>
#include "../PagingAuditCommon.h"

extern MEMORY_PROTECTION_DEBUG_PROTOCOL  *mMemoryProtectionProtocol;

#define TT_ADDRESS_MASK  (0xFFFFFFFFFULL << 12)

#define IS_TABLE(page, level)  ((level == 3) ? FALSE : (((page) & TT_TYPE_MASK) == TT_TYPE_TABLE_ENTRY))
#define IS_BLOCK(page, level)  ((level == 3) ? (((page) & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY_LEVEL3) : ((page & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY))
#define ROOT_TABLE_LEN(T0SZ)   (TT_ENTRY_COUNT >> ((T0SZ) - 16) % 9)

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
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINT64      *Pml0;
  UINT64      *Pte1G;
  UINT64      *Pte2M;
  UINT64      *Pte4K;
  UINT64      Index3              = 0;
  UINT64      Index2              = 0;
  UINT64      Index1              = 0;
  UINT64      Index0              = 0;
  UINTN       MyGuardCount        = 0;
  UINTN       MyPdeCount          = 0;
  UINTN       My4KCount           = 0;
  UINTN       My2MCount           = 0;
  UINTN       My1GCount           = 0;
  UINTN       NumPage4KNotPresent = 0;
  UINTN       NumPage2MNotPresent = 0;
  UINTN       NumPage1GNotPresent = 0;
  UINT64      RootEntryCount      = 0;
  UINT64      Address;
  BOOLEAN     Valid;

  //  Count parameters should be provided.
  if ((Pte1GCount == NULL) || (Pte2MCount == NULL) || (Pte4KCount == NULL) || (PdeCount == NULL) || (GuardCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // If a count is greater than 0, the corresponding buffer pointer MUST be provided.
  // It will be assumed that all buffers have space for any corresponding count.
  if (((*Pte1GCount > 0) && (Pte1GEntries == NULL)) || ((*Pte2MCount > 0) && (Pte2MEntries == NULL)) ||
      ((*Pte4KCount > 0) && (Pte4KEntries == NULL)) || ((*PdeCount > 0) && (PdeEntries == NULL)) ||
      ((*GuardCount > 0) && (GuardEntries == NULL)))
  {
    return EFI_INVALID_PARAMETER;
  }

  Pml0           = (UINT64 *)ArmGetTTBR0BaseAddress ();
  RootEntryCount = ROOT_TABLE_LEN (ArmGetTCR () & TCR_T0SZ_MASK);
  MyPdeCount++;
  if (MyPdeCount <= *PdeCount) {
    PdeEntries[MyPdeCount-1] = (UINT64)Pml0;
  }

  for (Index0 = 0x0; Index0 < RootEntryCount; Index0++) {
    Index1 = 0;
    Index2 = 0;
    Index3 = 0;
    if (!IS_TABLE (Pml0[Index0], 0)) {
      continue;
    }

    Pte1G = (UINT64 *)(Pml0[Index0] & TT_ADDRESS_MASK);

    MyPdeCount++;
    if (MyPdeCount <= *PdeCount) {
      PdeEntries[MyPdeCount-1] = (UINT64)Pte1G;
    }

    for (Index1 = 0x0; Index1 < TT_ENTRY_COUNT; Index1++ ) {
      Index2 = 0;
      Index3 = 0;
      Valid  = TRUE;
      if ((Pte1G[Index1] & 0x1) == 0) {
        NumPage1GNotPresent++;
        Valid = FALSE;
      }

      if (!IS_BLOCK (Pte1G[Index1], 1) && Valid) {
        Pte2M = (UINT64 *)(Pte1G[Index1] & TT_ADDRESS_MASK);

        MyPdeCount++;
        if (MyPdeCount <= *PdeCount) {
          PdeEntries[MyPdeCount-1] = (UINT64)Pte2M;
        }

        for (Index2 = 0x0; Index2 < TT_ENTRY_COUNT; Index2++ ) {
          Index3 = 0;
          Valid  = TRUE;
          if ((Pte2M[Index2] & 0x1) == 0) {
            NumPage2MNotPresent++;
            Valid = FALSE;
          }

          if (!IS_BLOCK (Pte2M[Index2], 2) && Valid) {
            Pte4K = (UINT64 *)(Pte2M[Index2] & TT_ADDRESS_MASK);
            MyPdeCount++;

            if (MyPdeCount <= *PdeCount) {
              PdeEntries[MyPdeCount-1] = (UINT64)Pte4K;
            }

            for (Index3 = 0x0; Index3 < TT_ENTRY_COUNT; Index3++ ) {
              Address = IndexToAddress (Index0, Index1, Index2, Index3);
              if ((mMemoryProtectionProtocol != NULL) && (mMemoryProtectionProtocol->IsGuardPage (Address)) && ((Pte4K[Index3] & TT_AF) == 0)) {
                MyGuardCount++;
                if (MyGuardCount <= *GuardCount) {
                  GuardEntries[MyGuardCount - 1] = Address;
                }

                continue;
              }

              if (!IS_BLOCK (Pte4K[Index3], 3)) {
                NumPage4KNotPresent++;
                continue;
              }

              My4KCount++;
              if (My4KCount <= *Pte4KCount) {
                Pte4KEntries[My4KCount-1] = Pte4K[Index3] | IndexToAddress (Index0, Index1, Index2, Index3);
              }
            }
          } else {
            My2MCount++;

            if (My2MCount <= *Pte2MCount) {
              Pte2MEntries[My2MCount-1] = Pte2M[Index2] | IndexToAddress (Index0, Index1, Index2, Index3);
            }
          }
        }
      } else {
        My1GCount++;

        if (My1GCount <= *Pte1GCount) {
          Pte1GEntries[My1GCount-1] = Pte1G[Index1] | IndexToAddress (Index0, Index1, Index2, Index3);
        }
      }
    }
  }

  DEBUG ((DEBUG_ERROR, "Pages used for Page Tables   = %d\n", MyPdeCount));
  DEBUG ((DEBUG_ERROR, "Number of   4K Pages active  = %d - NotPresent = %d\n", My4KCount - NumPage4KNotPresent, NumPage4KNotPresent));
  DEBUG ((DEBUG_ERROR, "Number of   2M Pages active  = %d - NotPresent = %d\n", My2MCount - NumPage2MNotPresent, NumPage2MNotPresent));
  DEBUG ((DEBUG_ERROR, "Number of   1G Pages active  = %d - NotPresent = %d\n", My1GCount - NumPage1GNotPresent, NumPage1GNotPresent));
  DEBUG ((DEBUG_ERROR, "Number of   Guard Pages active  = %d\n", MyGuardCount));

  //
  // determine whether any of the buffers were too small.
  // Only matters if a given buffer was provided.
  //
  if (((Pte1GEntries != NULL) && (*Pte1GCount < My1GCount)) || ((Pte2MEntries != NULL) && (*Pte2MCount < My2MCount)) ||
      ((Pte4KEntries != NULL) && (*Pte4KCount < My4KCount)) || ((PdeEntries != NULL) && (*PdeCount < MyPdeCount)) ||
      ((GuardEntries != NULL) && (*GuardCount < MyGuardCount)))
  {
    Status = EFI_BUFFER_TOO_SMALL;
  }

  //
  // Update all the return pointers.
  //
  *Pte1GCount = My1GCount;
  *Pte2MCount = My2MCount;
  *Pte4KCount = My4KCount;
  *PdeCount   = MyPdeCount;
  *GuardCount = MyGuardCount;

  return Status;
}

/**
  Calculate the maximum physical address bits supported.

  @return the maximum support physical address bits supported.
**/
UINT8
EFIAPI
CalculateMaximumSupportAddressBits (
  VOID
  )
{
  return 36;
}

/**
   Dump platform specific handler. Created handler(s) need to be compliant with
   Windows\PagingReportGenerator.py, i.e. TSEG.
**/
VOID
EFIAPI
DumpProcessorSpecificHandlers (
  VOID
  )
{
  return;
}

/**
  Dumps platform info required to correctly parse the pages (architecture,
  execution level, etc.)
**/
VOID
EFIAPI
DumpPlatforminfo (
  VOID
  )
{
  CHAR8                          TempString[MAX_STRING_SIZE];
  UINTN                          ExecutionLevel;
  CHAR8                          *ElString;
  UINTN                          StringIndex;
  EFI_MEMORY_ATTRIBUTE_PROTOCOL  *MemoryAttributeProtocol;

  ExecutionLevel = ArmReadCurrentEL ();

  if (ExecutionLevel == AARCH64_EL1) {
    ElString = "EL1";
  } else if (ExecutionLevel == AARCH64_EL2) {
    ElString = "EL2";
  } else if (ExecutionLevel == AARCH64_EL3) {
    ElString = "EL3";
  } else {
    ElString = "Unknown";
  }

  MemoryAttributeProtocol = NULL;
  gBS->LocateProtocol (&gEfiMemoryAttributeProtocolGuid, NULL, (VOID **)&MemoryAttributeProtocol);

  // Dump the execution level of UEFI
  StringIndex = AsciiSPrint (
                  &TempString[0],
                  MAX_STRING_SIZE,
                  "Architecture,AARCH64\nBitwidth,%d\nPhase,DXE\nExecutionLevel,%a\nMemoryAttributeProtocolPresent,%a\n",
                  CalculateMaximumSupportAddressBits (),
                  ElString,
                  (MemoryAttributeProtocol == NULL) ?  "FALSE" : "TRUE"
                  );

  WriteBufferToFile (L"PlatformInfo", TempString, StringIndex);
}
