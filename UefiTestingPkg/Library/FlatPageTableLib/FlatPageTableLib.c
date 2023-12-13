/** @file
  Library to parse page/translation table entries.  This library
  is restricted to UEFI_APPLICATION modules because it should be
  used primarily for testing. For querying page attributes from
  non-application modules, core services like the GCD or Memory
  Attribute Protocol should be used to maintain coherency.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/SafeIntLib.h>
#include <Library/FlatPageTableLib.h>
#include <Library/DebugLib.h>

// TRUE if A and B have overlapping intervals.
// The intervals are inclusive.
#define CHECK_OVERLAP(AStart, AEnd, BStart, BEnd)   \
  ((AEnd >= AStart) && (BEnd >= BStart) &&          \
  ((AStart <= BStart && AEnd >= BStart) ||          \
  (BStart <= AStart && BEnd >= AStart)))

/**
  Dumps the contents of the input PAGE_MAP to the debug log.
**/
VOID
EFIAPI
DumpPageMap (
  IN PAGE_MAP  *Map
  )
{
  UINTN   Index;
  UINT64  Attributes;

  DEBUG ((DEBUG_INFO, "Page Map: %p\n", Map));
  DEBUG ((DEBUG_INFO, "  EntryCount: %d\n", Map->EntryCount));
  DEBUG ((DEBUG_INFO, "  Entries:\n"));
  for (Index = 0; Index < Map->EntryCount; Index++) {
    Attributes  = IsPageExecutable (Map->Entries[Index].PageEntry) ? 0 : EFI_MEMORY_XP;
    Attributes |= IsPageWritable (Map->Entries[Index].PageEntry) ? 0 : EFI_MEMORY_RO;
    Attributes |= IsPageReadable (Map->Entries[Index].PageEntry) ? 0 : EFI_MEMORY_RP;
    DEBUG ((
      DEBUG_INFO,
      "    %d: %p-%p. Attributes: 0x%llx\n",
      Index,
      Map->Entries[Index].LinearAddress,
      Map->Entries[Index].LinearAddress + Map->Entries[Index].Length - 1,
      Attributes
      ));
  }
}

/**
  Checks the input flat page/translation table for the input region and converts the associated
  table entries to EFI access attributes (EFI_MEMORY_XP, EFI_MEMORY_RO, EFI_MEMORY_RP). The caller
  of this function is responsible for checking ActualCheckedLength if the return value is
  EFI_NOT_FOUND or EFI_NO_MAPPING. EFI_NOT_FOUND indicates that the attributes vary across
  the region. EFI_NO_MAPPING indicates that the section from RegionStart to RegionStart +
  ActualCheckedLength is not mapped. If ActualCheckedLength == RegionLength,
  when EFI_NO_MAPPING is returned, the entire input region is not mapped.

  @param[in]  Map                     Pointer to the PAGE_MAP struct to be parsed
  @param[in]  RegionStart             Starting address of the region to check.
  @param[in]  RegionLength            Length, in bytes, of the region to check.
  @param[out] Attributes              EFI Attributes of the region.
  @param[out] ActualCheckedLength     The length checked from RegionStart.
                                      If the region has varying attributes or the start
                                      of the region is not mapped, this will be the
                                      length from RegionStart to which the return
                                      value applies.

  @retval EFI_SUCCESS             The region attributes were successfully determined.
  @retval EFI_INVALID_PARAMETER   An input argument is invalid.
  @retval EFI_ABORTED             The input PAGE_MAP is invalid.
  @retval EFI_NOT_FOUND           The input region starting at RegionStart has varying
                                  attributes. See ActualCheckedLength for the length of
                                  the contiguous region with the same attributes as
                                  the start of the input region.
  @retval EFI_NO_MAPPING          The region starting at RegionStart is not mapped. If
                                  ActualCheckedLength == RegionLength, the region is
                                  not mapped at all. Otherwise, ActualCheckedLength will
                                  be the length of the unmapped region from RegionStart.
**/
EFI_STATUS
EFIAPI
GetRegionAccessAttributes (
  IN PAGE_MAP  *Map,
  IN UINT64    RegionStart,
  IN UINT64    RegionLength,
  OUT UINT64   *Attributes,
  OUT UINT64   *ActualCheckedLength
  )
{
  UINTN    Index;
  UINT64   EntryStartAddress;
  UINT64   EntryEndAddress;
  UINT64   CurrentStartAddress;
  UINT64   RegionEnd;
  BOOLEAN  FoundRange;
  UINT64   FoundAttributes;
  UINT64   FoundAttributesOriginal;

  if ((Map->Entries == NULL) || (Map->EntryCount == 0) ||
      (Attributes == NULL) || (ActualCheckedLength == NULL) || (RegionLength == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  FoundRange          = FALSE;
  Index               = 0;
  CurrentStartAddress = RegionStart;
  RegionEnd           = 0;

  if (EFI_ERROR (SafeUint64Add (RegionStart, RegionLength - 1, &RegionEnd))) {
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
      *ActualCheckedLength = 0;
      return EFI_ABORTED;
    }

    if (CHECK_OVERLAP (CurrentStartAddress, RegionEnd, EntryStartAddress, EntryEndAddress)) {
      FoundAttributes  = IsPageExecutable (Map->Entries[Index].PageEntry) ? 0 : EFI_MEMORY_XP;
      FoundAttributes |= IsPageWritable (Map->Entries[Index].PageEntry) ? 0 : EFI_MEMORY_RO;
      FoundAttributes |= IsPageReadable (Map->Entries[Index].PageEntry) ? 0 : EFI_MEMORY_RP;

      // There is a gap between the current address and the start of the entry.
      if (EntryStartAddress > CurrentStartAddress) {
        if (FoundRange) {
          *Attributes          = FoundAttributesOriginal;
          *ActualCheckedLength = CurrentStartAddress - RegionStart;
          return EFI_NOT_FOUND;
        } else {
          *Attributes          = 0;
          *ActualCheckedLength = EntryStartAddress - RegionStart;
          return EFI_NO_MAPPING;
        }
      }

      if (!FoundRange) {
        FoundAttributesOriginal = FoundAttributes;
        FoundRange              = TRUE;
      } else if (FoundAttributesOriginal != FoundAttributes) {
        *Attributes          = FoundAttributesOriginal;
        *ActualCheckedLength = CurrentStartAddress - RegionStart;
        return EFI_NOT_FOUND;
      }

      // The entry end address is inclusive, so add one to get the next region start.
      // If the addition overflows, the region is contiguous to the end of the address space.
      if (EFI_ERROR (SafeUint64Add (EntryEndAddress, 1, &CurrentStartAddress))) {
        *Attributes          = FoundAttributesOriginal;
        *ActualCheckedLength = RegionLength;
        return EFI_SUCCESS;
      }
    }

    if (CurrentStartAddress >= RegionEnd) {
      break;
    }
  } while (++Index < Map->EntryCount);

  if (FoundRange) {
    *Attributes          = FoundAttributesOriginal;
    *ActualCheckedLength = CurrentStartAddress >= RegionEnd ? RegionLength : CurrentStartAddress - RegionStart;
  } else {
    *Attributes          = 0;
    *ActualCheckedLength = RegionLength;
  }

  return FoundRange ? EFI_SUCCESS : EFI_NO_MAPPING;
}
