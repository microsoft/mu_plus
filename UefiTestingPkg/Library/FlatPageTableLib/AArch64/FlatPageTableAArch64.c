/** @file
  AArch64 specific page table attribute library functions.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/ArmLib.h>
#include <AArch64/AArch64.h>
#include <Library/DebugLib.h>
#include <Library/FlatPageTableLib.h>

#define TCR_EL1_HPD_FIELD             BIT41   // Assumes translation table is located at TTBR0 (UEFI spec dictated)
#define TCR_EL2_HPD_FIELD             BIT24
#define ID_AA64MMFR1_EL1_HPD_MASK     0xF000
#define TT_HERITABLE_ATTRIBUTES_MASK  (TT_TABLE_AP_MASK | TT_TABLE_PXN | TT_TABLE_UXN)
#define AARCH64_ATTRIBUTES_MASK       ((0xFFFULL << 52) | (0x3FFULL << 2))
#define MIN_T0SZ                      16
#define BITS_PER_LEVEL                9

#define IS_VALID(page)                 ((page & 0x1) != 0)
#define IS_TABLE(page, level)          ((level == 3) ? FALSE : (((page) & TT_TYPE_MASK) == TT_TYPE_TABLE_ENTRY))
#define IS_BLOCK(page, level)          ((level == 3) ? (((page) & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY_LEVEL3) : ((page & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY))
#define ARM_TT_BLOCK_ATTRIBUTES(page)  (page & AARCH64_ATTRIBUTES_MASK)
#define TT_ADDRESS_MASK  (0xFFFFFFFFFULL << 12)

// All translation table bit definitions were taken from the Armv8 A
// Architecture Manual version H.a.
typedef union {
  struct {
    UINT64    Valid           : 1;  // BIT0
    UINT64    BlockOrTable    : 1;  // BIT1
    UINT64    LowerAttributes : 10; // BIT2-11
    UINT64    Address         : 36; // BIT12-47
    UINT64    Reserved        : 3;  // BIT48-50
    UINT64    Ignored         : 8;  // BIT51-58
    UINT64    PxnTable        : 1;  // BIT59
    UINT64    XnTable         : 1;  // BIT60
    UINT64    ApTable         : 2;  // BIT61-62
    UINT64    NsTable         : 1;  // BIT63
  } Bits;
  UINT64    Uint64;
} TRANSLATION_TABLE_ENTRY_HERITABLE;

typedef union {
  struct {
    UINT64    Valid           : 1;  // BIT0
    UINT64    BlockOrTable    : 1;  // BIT1
    UINT64    LowerAttributes : 10; // BIT2-11
    UINT64    LevelZeroIndex  : 9;  // BIT12-20
    UINT64    LevelOneIndex   : 9;  // BIT21-29
    UINT64    LevelTwoIndex   : 9;  // BIT30-38
    UINT64    LevelThreeIndex : 9;  // BIT39-47
    UINT64    Reserved        : 3;  // BIT48-50
    UINT64    Ignored         : 8;  // BIT51-58
    UINT64    PxnTable        : 1;  // BIT59
    UINT64    XnTable         : 1;  // BIT60
    UINT64    ApTable         : 2;  // BIT61-62
    UINT64    NsTable         : 1;  // BIT63
  } Bits;
  UINT64    Uint64;
} TRANSLATION_TABLE_ENTRY_TABLE;

typedef AARCH64_PAGE_MAP_ENTRY TRANSLATION_TABLE_ENTRY_BLOCK;

typedef union {
  TRANSLATION_TABLE_ENTRY_BLOCK        Tteb;
  TRANSLATION_TABLE_ENTRY_TABLE        Ttet;
  TRANSLATION_TABLE_ENTRY_HERITABLE    Tteh;
  UINT64                               Uint64;
} TRANSLATION_TABLE_ENTRY_UNION;

STATIC BOOLEAN  mHierarchicalControlEnabled = FALSE;

STATIC BOOLEAN  mLpa2Enabled = FALSE;

STATIC
BOOLEAN
TranslationRegimeIsDual (
  VOID
  )
{
  if (ArmReadCurrentEL () == AARCH64_EL2) {
    return (ArmReadHcr () & ARM_HCR_E2H) != 0;
  }

  return TRUE;
}

STATIC
BOOLEAN
IsLpa2Enabled (
  VOID
  )
{
  UINT64  Tcr;

  Tcr = ArmGetTCR ();

  return !TranslationRegimeIsDual () ?
         ((Tcr & TCR_DS_NVHE) != 0) :
         ((Tcr & TCR_DS) != 0);
}

STATIC
UINT64
GetOutputAddress (
  IN UINT64   Entry,
  IN BOOLEAN  Lpa2Enabled
  )
{
  if (Lpa2Enabled) {
    return (Entry & TT_ADDRESS_MASK_BLOCK_ENTRY_LPA2) | ((Entry & TT_UPPER_ADDRESS_MASK) << (50 - 8));
  }

  return Entry & TT_ADDRESS_MASK_BLOCK_ENTRY;
}

STATIC
UINTN
GetRootTableEntryCount (
  IN UINTN  T0Sz
  )
{
  return TT_ENTRY_COUNT >> (T0Sz - MIN_T0SZ) % BITS_PER_LEVEL;
}

STATIC
INTN
GetRootTableLevel (
  IN UINTN  T0Sz
  )
{
  INTN  RootTableLevel;

  RootTableLevel = (T0Sz < MIN_T0SZ) ? -1 : (INTN)(T0Sz - MIN_T0SZ) / BITS_PER_LEVEL;
  ASSERT (RootTableLevel >= 0 || mLpa2Enabled);

  return RootTableLevel;
}

#if !defined (__clang__) && !defined (__GNUC__)

/**
  Reads the ID_AA64MMFR1_EL1 special register.

  @retval The UINT64 value of the ID_AA64MMFR1_EL1 special register.
**/
UINT64
Asm_Read_ID_AA64MMFR1_EL1 (
  VOID
  );

#else

/**
  Reads the ID_AA64MMFR1_EL1 special register.

  @retval The UINT64 value of the ID_AA64MMFR1_EL1 special register.
**/
STATIC
UINT64
Asm_Read_ID_AA64MMFR1_EL1 (
  VOID
  )
{
  UINT64  value = 0;
  __asm volatile ("mrs %0, ID_AA64MMFR1_EL1" : "=r" (value) ::);

  return value;
}

#endif

/**
  Checks the ID_AA64MMFR1_EL1 and TCR special registers to see if hierarchical control is enabled.

  Function was derived from Armv8 A Architecture Manual version H.a
  Section 13.2.65: "ID_AA64MMFR1_EL1, AArch64 Memory Model Feature Register 1"
  Section D13.2.131: TCR_EL1, Translation Control Register (EL1)
  Section D13.2.132: TCR_EL2, Translation Control Register (EL2)

  @retval TRUE  Hierarchical control is enabled.
  @retval FALSE Hierarchical control is disabled.
**/
STATIC
BOOLEAN
IsHierarchicalControlEnabled (
  VOID
  )
{
  return ((Asm_Read_ID_AA64MMFR1_EL1 () & ID_AA64MMFR1_EL1_HPD_MASK) != 0) &&
         ((ArmReadCurrentEL () == AARCH64_EL2) ? (ArmGetTCR () & TCR_EL2_HPD_FIELD) == 0 :
          (ArmGetTCR () & TCR_EL1_HPD_FIELD) == 0);
}

/**
  Recursively parse the translation table and populate the entries in the input Map.

  @param[in]      PageTableBaseAddress        The base address of the 512 page table entries in the specified level
  @param[in]      Level                       Page level (-1, 0, 1, 2, 3)
  @param[in]      RegionStart                 The base linear address of the region covered by the page table entries
  @param[in]      ParentHeritableAttributes    The heritable attributes of parent table entries.
  @param[in, out] Map                         Pointer to an array that describes multiple linear address ranges.
  @param[in, out] MapCount                    Pointer to a UINTN that hold the actual number of entries in the Map.
  @param[in]      MapCapacity                 The maximum number of entries the Map can hold.
  @param[in]      LastEntry                   Pointer to last map entry.
  @param[in]      OneEntry                    Pointer to a library internal storage that holds one map entry which is
                                              used when Map array is at capacity.
  @param[in]      SelfMapped                  TRUE if the page tables are self-mapped, FALSE otherwise.
  @param[in]      IsRootTable                 TRUE if parsing the root table level.
**/
STATIC
VOID
TranslationTableParseRecursive (
  IN     UINT64                             PageTableBaseAddress,
  IN     INTN                               Level,
  IN     UINT64                             RegionStart,
  IN     TRANSLATION_TABLE_ENTRY_HERITABLE  ParentHeritableAttributes,
  IN OUT PAGE_MAP_ENTRY                     *Map,
  IN OUT UINTN                              *MapCount,
  IN     UINTN                              MapCapacity,
  IN     PAGE_MAP_ENTRY                     **LastEntry,
  IN     PAGE_MAP_ENTRY                     *OneEntry,
  IN     BOOLEAN                            SelfMapped,
  IN     BOOLEAN                            IsRootTable
  )
{
  TRANSLATION_TABLE_ENTRY_UNION  *PagingEntry;
  UINTN                          Index;
  TRANSLATION_TABLE_ENTRY_UNION  ScratchEntry;
  UINT64                         RegionLength;
  UINTN                          EntryCount;

  if ((OneEntry == NULL) || (MapCount == NULL) || (LastEntry == NULL)) {
    return;
  }

  if (SelfMapped) {
    switch (Level) {
      case 0:
        PagingEntry = (TRANSLATION_TABLE_ENTRY_UNION *)0xFFFFFFFFF000;
        break;
      case 1:
        PagingEntry = (TRANSLATION_TABLE_ENTRY_UNION *)(0xFFFFFFE00000 + SIZE_4KB * ((RegionStart >> 39) & 0x1FF));
        break;
      case 2:
        PagingEntry = (TRANSLATION_TABLE_ENTRY_UNION *)(0xFFFFC0000000 +
                                                        SIZE_2MB * ((RegionStart >> 39) & 0x1FF) +
                                                        SIZE_4KB * ((RegionStart >> 30) & 0x1FF));
        break;
      case 3:
        PagingEntry = (TRANSLATION_TABLE_ENTRY_UNION *)(0xFF8000000000 +
                                                        SIZE_1GB * ((RegionStart >> 39) & 0x1FF) +
                                                        SIZE_2MB * ((RegionStart >> 30) & 0x1FF) +
                                                        SIZE_4KB * ((RegionStart >> 21) & 0x1FF));
        break;
      default:
        PagingEntry = (TRANSLATION_TABLE_ENTRY_UNION *)(UINTN)PageTableBaseAddress;
        break;
    }
  } else {
    PagingEntry = (TRANSLATION_TABLE_ENTRY_UNION *)(UINTN)PageTableBaseAddress;
  }

  RegionLength = TT_BLOCK_ENTRY_SIZE_AT_LEVEL (Level);
  if (IsRootTable) {
    EntryCount = GetRootTableEntryCount (ArmGetTCR () & TCR_T0SZ_MASK);
  } else {
    EntryCount = TT_ENTRY_COUNT;
  }

  for (Index = 0; Index < EntryCount; Index++, RegionStart += RegionLength) {
    // Skip unmapped entries
    if (!IS_VALID (PagingEntry[Index].Uint64)) {
      continue;
    }

    if (IsRootTable && ((Index == 0x1FF) || (Index == 0x1FE)) && SelfMapped) {
      // Skip self-map entries in root table
      continue;
    }

    if (IS_BLOCK (PagingEntry[Index].Uint64, Level)) {
      // FUTURE WORK: Should we check the feature register to see if blocks are allowed at level 1?
      ASSERT (Level == 1 || Level == 2 || Level == 3);

      ScratchEntry.Uint64 = PagingEntry[Index].Tteb.Uint64;

      // If the entry is a block, then then some access attributes are inherited from the parent
      // when FEAT_HPDS is active
      if (mHierarchicalControlEnabled) {
        // Check if the parent table entry has XnTable bits to pass down to the block entry.
        //
        // This logic was derived from the Armv8 A Architecture Manual version H.a
        // Section D5.4.5: Data access permission controls
        // Subsection: Hierarchical control of instruction fetching
        //
        // If in EL2, only the XN bit is valid
        if ((ArmReadCurrentEL () == AARCH64_EL2) && (ParentHeritableAttributes.Bits.XnTable != 0)) {
          ScratchEntry.Tteb.Bits.Xn = 1;
        }
        // if in EL1, both UXN and PXN bits are valid
        else if ((ArmReadCurrentEL () == AARCH64_EL1)) {
          // Xn is Uxn for EL1
          if (ParentHeritableAttributes.Bits.XnTable != 0) {
            ScratchEntry.Tteb.Bits.Xn = 1;
          }

          if (ParentHeritableAttributes.Bits.PxnTable != 0) {
            ScratchEntry.Tteb.Bits.Pxn = 1;
          }
        }

        // Check if the parent table entry has ApTable bits to pass down to the block entry
        //
        // This logic was derived from the Armv8 A Architecture Manual version H.a
        // Section D5.4.5: Data access permission controls
        // Subsection: Hierarchical control of data access permissions
        //
        // 0b01 -> Access from EL0 is not allowed, no effect on write permissions
        if ((ParentHeritableAttributes.Bits.ApTable & TT_TABLE_AP_MASK) == TT_TABLE_AP_EL0_NO_ACCESS) {
          // BIT6 toggles access from EL0
          ScratchEntry.Tteb.Bits.AccessPermissions &= ~((UINT64)BIT6);   // Clear BIT6
          // 0b10 -> Access from EL0 is read-only, no write permissions at any EL
        } else if ((ParentHeritableAttributes.Bits.ApTable & TT_TABLE_AP_MASK) == TT_TABLE_AP_NO_WRITE_ACCESS) {
          // BIT7 toggles write access for all ELs
          ScratchEntry.Tteb.Bits.AccessPermissions |= BIT7;   // Set BIT7
          // 0b11 -> Access from EL0 is not allowed, no write permissions at any EL
        } else if ((ParentHeritableAttributes.Bits.ApTable & TT_TABLE_AP_MASK) == TT_TABLE_AP_MASK) {
          ScratchEntry.Tteb.Bits.AccessPermissions |= BIT7;              // Set BIT7
          ScratchEntry.Tteb.Bits.AccessPermissions &= ~((UINT64)BIT6);   // Clear BIT6
        }
      }

      if ((*LastEntry != NULL) &&
          ((*LastEntry)->LinearAddress + (*LastEntry)->Length == RegionStart) &&
          (GetOutputAddress ((*LastEntry)->PageEntry, mLpa2Enabled) + (*LastEntry)->Length
           == GetOutputAddress (ScratchEntry.Uint64, mLpa2Enabled)) &&
          (ARM_TT_BLOCK_ATTRIBUTES ((*LastEntry)->PageEntry) == ARM_TT_BLOCK_ATTRIBUTES (ScratchEntry.Uint64))
          )
      {
        // Extend LastEntry.
        (*LastEntry)->Length += RegionLength;
      } else {
        if (*MapCount < MapCapacity) {
          // LastEntry points to next map entry in the array.
          *LastEntry = &Map[*MapCount];
        } else {
          // LastEntry points to library internal map entry.
          *LastEntry = OneEntry;
        }

        // Set LastEntry.
        (*LastEntry)->LinearAddress = RegionStart;
        (*LastEntry)->Length        = RegionLength;
        (*LastEntry)->PageEntry     = ScratchEntry.Uint64;
        (*MapCount)++;
      }
    } else if (IS_TABLE (PagingEntry[Index].Uint64, Level)) {
      ScratchEntry.Uint64 = PagingEntry[Index].Ttet.Uint64 & TT_HERITABLE_ATTRIBUTES_MASK;
      // If the entry is a table and not the root, then pass the heritable access attributes
      // from the parent.
      if (Level > 0) {
        ScratchEntry.Uint64 |= (ParentHeritableAttributes.Uint64 & TT_HERITABLE_ATTRIBUTES_MASK);
      }

      TranslationTableParseRecursive (
        GetOutputAddress (PagingEntry[Index].Ttet.Uint64, mLpa2Enabled),
        Level + 1,
        RegionStart,
        ScratchEntry.Tteh,
        Map,
        MapCount,
        MapCapacity,
        LastEntry,
        OneEntry,
        SelfMapped,
        FALSE
        );
    } else {
      ASSERT (FALSE);
      continue;
    }
  }
}

/**
  Populate the input page/translation table map.

  @param[in, out]      Map         Pointer to the PAGE_MAP struct to be populated.

  @retval EFI_SUCCESS              The translation table is parsed successfully.
  @retval EFI_INVALID_PARAMETER    MapCount is NULL, or Map is NULL but *MapCount is nonzero.
  @retval EFI_BUFFER_TOO_SMALL     *MapCount is too small.
                                   MapCount is updated to indicate the expected number of entries.
                                   Caller may still get EFI_BUFFER_TOO_SMALL with the new MapCount.
**/
EFI_STATUS
EFIAPI
CreateFlatPageTable (
  IN OUT PAGE_MAP  *Map
  )
{
  UINTN                              LocalEntryCount;
  PAGE_MAP_ENTRY                     *LastEntry;
  PAGE_MAP_ENTRY                     OneEntry;
  TRANSLATION_TABLE_ENTRY_HERITABLE  HeritableAttributes;
  UINTN                              T0SZ;
  INTN                               RootTableLevel;
  BOOLEAN                            SelfMapped;
  UINT64                             Base;

  ASSERT (sizeof (OneEntry.PageEntry) == sizeof (AARCH64_PAGE_MAP_ENTRY));

  if ((Map == NULL) || ((Map->Entries == NULL) && (Map->EntryCount != 0))) {
    return EFI_INVALID_PARAMETER;
  }

  Map->ArchSignature = AARCH64_PAGE_MAP_SIGNATURE;

  T0SZ = ArmGetTCR () & TCR_T0SZ_MASK;

  mLpa2Enabled                = IsLpa2Enabled ();
  mHierarchicalControlEnabled = IsHierarchicalControlEnabled ();
  HeritableAttributes.Uint64  = 0;
  LastEntry                   = NULL;
  LocalEntryCount             = 0;
  RootTableLevel              = GetRootTableLevel (T0SZ);

  Base       = (UINT64)(UINTN)ArmGetTTBR0BaseAddress ();
  SelfMapped = (((UINT64 *)Base)[0x1FF] & TT_ADDRESS_MASK) == Base ? TRUE : FALSE;
  if (RootTableLevel < 0) {
    SelfMapped = FALSE;
  }

  TranslationTableParseRecursive (
    (UINTN)ArmGetTTBR0BaseAddress (),
    RootTableLevel,
    0,
    HeritableAttributes,
    Map->Entries,
    &LocalEntryCount,
    Map->EntryCount,
    &LastEntry,
    &OneEntry,
    SelfMapped,
    TRUE
    );

  if (LocalEntryCount > Map->EntryCount) {
    Map->EntryCount = LocalEntryCount;
    return EFI_BUFFER_TOO_SMALL;
  }

  Map->EntryCount = LocalEntryCount;
  return RETURN_SUCCESS;
}

/**
  Parses the input page to determine if it is writable.

  @param[in] Page The page entry to parse.

  @retval TRUE    The page is writable.
  @retval FALSE   The page is not writable.
**/
BOOLEAN
EFIAPI
IsPageWritable (
  IN UINT64  Page
  )
{
  return ((Page & TT_AP_MASK) == TT_AP_RW_RW) || ((Page & TT_AP_MASK) == TT_AP_NO_RW);
}

/**
  Parses the input page to determine if it is executable.

  @param[in] Page The page entry to parse.

  @retval TRUE    The page is executable.
  @retval FALSE   The page is not executable.
**/
BOOLEAN
EFIAPI
IsPageExecutable (
  IN UINT64  Page
  )
{
  return (ArmReadCurrentEL () == AARCH64_EL2) ?
         ((Page & TT_XN_MASK) == 0) : ((Page & TT_UXN_MASK) == 0 || (Page & TT_PXN_MASK) == 0);
}

/**
  Parses the input page to determine if it is readable.

  @param[in] Page The page entry to parse.

  @retval TRUE    The page is readable.
  @retval FALSE   The page is not readable.
**/
BOOLEAN
EFIAPI
IsPageReadable (
  IN UINT64  Page
  )
{
  return (Page & TT_AF) != 0;
}
