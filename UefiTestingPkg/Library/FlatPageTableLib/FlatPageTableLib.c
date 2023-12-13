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

// TRUE if A and B have overlapping intervals
#define CHECK_OVERLAP(AStart, AEnd, BStart, BEnd)   \
  ((AEnd > AStart) && (BEnd > BStart) &&            \
  ((AStart <= BStart && AEnd > BStart) ||           \
  (BStart <= AStart && BEnd > AStart)))

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
  table entries to EFI access attributes (EFI_MEMORY_XP, EFI_MEMORY_RO, EFI_MEMORY_RP). If the
  access attributes vary across the region, EFI_NOT_FOUND is returned.

  @param[in]  Map                 Pointer to the PAGE_MAP struct to be parsed
  @param[in]  Length              Length in bytes of the region
  @param[in]  Length              Length of the region
  @param[out] Attributes          EFI Attributes of the region

  @retval EFI_SUCCESS             The output Attributes is valid
  @retval EFI_INVALID_PARAMETER   The flat translation table has not been built or
                                  Attributes was NULL or Length was 0
  @retval EFI_NOT_FOUND           The input region could not be found
  @retval EFI_NO_MAPPING          The access attributes are not consistent across the region.
**/
EFI_STATUS
EFIAPI
GetRegionAccessAttributes (
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
  UINT64   FoundAttributes;
  UINT64   FoundAttributesOriginal;

  if ((Map->Entries == NULL) || (Map->EntryCount == 0) ||
      (Attributes == NULL) || (Length == 0))
  {
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
        FoundAttributesOriginal  = 0;
        FoundAttributesOriginal |= IsPageExecutable (Map->Entries[Index].PageEntry) ? 0 : EFI_MEMORY_XP;
        FoundAttributesOriginal |= IsPageWritable (Map->Entries[Index].PageEntry) ? 0 : EFI_MEMORY_RO;
        FoundAttributesOriginal |= IsPageReadable (Map->Entries[Index].PageEntry) ? 0 : EFI_MEMORY_RP;
        FoundRange               = TRUE;
      } else {
        FoundAttributes  = 0;
        FoundAttributes |= IsPageExecutable (Map->Entries[Index].PageEntry) ? 0 : EFI_MEMORY_XP;
        FoundAttributes |= IsPageWritable (Map->Entries[Index].PageEntry) ? 0 : EFI_MEMORY_RO;
        FoundAttributes |= IsPageReadable (Map->Entries[Index].PageEntry) ? 0 : EFI_MEMORY_RP;
        if (FoundAttributesOriginal != FoundAttributes) {
          return EFI_NO_MAPPING;
        }
      }

      Address = EntryEndAddress + 1;
    }

    if (EntryEndAddress >= InputEndAddress) {
      break;
    }
  } while (++Index < Map->EntryCount);

  if (FoundRange) {
    *Attributes = FoundAttributesOriginal;
  }

  return FoundRange ? EFI_SUCCESS : EFI_NOT_FOUND;
}
