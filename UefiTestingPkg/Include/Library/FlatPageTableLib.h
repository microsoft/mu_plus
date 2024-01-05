/** @file
  Library to parse page/translation table entries.  This library
  is restricted to UEFI_APPLICATION modules because it should be
  used primarily for testing. For querying page attributes from
  non-application modules, core services like the GCD or Memory
  Attribute Protocol should be used to maintain coherency.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FLAT_PAGE_TABLE_LIB_H_
#define FLAT_PAGE_TABLE_LIB_H_

typedef struct {
  UINT64    LinearAddress;
  UINT64    Length;
  UINT64    PageEntry;
} PAGE_MAP_ENTRY;

typedef struct {
  UINT32            ArchSignature;
  PAGE_MAP_ENTRY    *Entries;
  UINTN             EntryCount;
  UINTN             EntryPagesAllocated;
} PAGE_MAP;

// The signature of the PAGE_MAP struct is used to determine the architecture of the page/translation table
// entries.
#define AARCH64_PAGE_MAP_SIGNATURE  SIGNATURE_32 ('A','A','6','4')
#define X64_PAGE_MAP_SIGNATURE      SIGNATURE_32 ('X','6','4',' ')

// This union can be used to interpret each PAGE_MAP_ENTRY on an AARCH64 system.
// These translation table bit definitions were taken from the Armv8 A Architecture Manual version H.a.
typedef union {
  struct {
    UINT64    Valid                      : 1;  // BIT0
    UINT64    BlockOrTable               : 1;  // BIT1
    UINT64    AttributeIndex             : 3;  // BIT2-4
    UINT64    NonSecure                  : 1;  // BIT5
    UINT64    AccessPermissions          : 2;  // BIT6-7
    UINT64    Shareability               : 2;  // BIT8-9
    UINT64    AccessFlag                 : 1;  // BIT10
    UINT64    NonGlobal                  : 1;  // BIT11
    UINT64    Oa                         : 4;  // BIT12-15
    UINT64    Nt                         : 1;  // BIT16
    UINT64    OutputAddress              : 33; // BIT17-49
    UINT64    Guarded                    : 1;  // BIT50
    UINT64    Dirty                      : 1;  // BIT51
    UINT64    Contiguous                 : 1;  // BIT52
    UINT64    Pxn                        : 1;  // BIT53
    UINT64    Xn                         : 1;  // BIT54
    UINT64    Ignored                    : 4;  // BIT55-58
    UINT64    PageBasedHardwareAttribute : 4;  // BIT59-62
    UINT64    Reserved                   : 1;  // BIT63
  } Bits;
  UINT64    Uint64;
} AARCH64_PAGE_MAP_ENTRY;

// This union can be used to interpret each PAGE_MAP_ENTRY on an X64 system.
// These page table bit defintions were taken from September 2023 Intel® 64 and IA-32 Architectures SDM
typedef union {
  struct {
    UINT64    Present              : 1;  // BIT0
    UINT64    ReadWrite            : 1;  // BIT1
    UINT64    UserSupervisor       : 1;  // BIT2
    UINT64    WriteThrough         : 1;  // BIT3
    UINT64    CacheDisabled        : 1;  // BIT4
    UINT64    Accessed             : 1;  // BIT5
    UINT64    Dirty                : 1;  // BIT6
    UINT64    Pat                  : 1;  // BIT7
    UINT64    Global               : 1;  // BIT8
    UINT64    Reserved1            : 3;  // BIT9-11
    UINT64    PageTableBaseAddress : 40; // BIT12-51
    UINT64    Reserved2            : 7;  // BIT52-58
    UINT64    ProtectionKey        : 4;  // BIT59-62
    UINT64    Nx                   : 1;  // BIT63
  } Bits;
  UINT64    Uint64;
} X64_PAGE_MAP_ENTRY;

// When the page/translation table is parsed to create an array of PAGE_MAP_ENTRY, the following bits
// are used to determine the attributes of one page/translation table entry are the same as another
// page/translation table entry. If contiguous leaf/block entries have the same attributes, then they
// will be represented in a single PAGE_MAP_ENTRY.
// AArch64: BIT2-11, BIT52-63
// X64:     BIT0-11, BIT52-63

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
  );

/**
  Dumps the contents of the input PAGE_MAP to the debug log.
**/
VOID
EFIAPI
DumpPageMap (
  IN PAGE_MAP  *Map
  );

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
  );

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
  );

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
  );

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
  );

#endif
