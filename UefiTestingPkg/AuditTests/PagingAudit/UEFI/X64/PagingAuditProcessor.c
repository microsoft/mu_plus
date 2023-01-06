/** @file -- PagingAuditProcessor.c

Platform specific memory audit functions.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "../PagingAuditCommon.h"
#include <Register/Msr.h>
#include <Library/UefiCpuLib.h>
#include <Pi/PiHob.h>
#include <Library/HobLib.h>

#define AMD_64_SMM_ADDR  0xC0010112
#define AMD_64_SMM_MASK  0xC0010113

#define VALID_SMRR_LOW_POS   BIT17
#define VALID_SMRR_HIGH_POS  BIT51
#define VALID_SMRR_BIT_MASK  (~(~(BIT51 - 1) | (BIT17 - 1)))

extern MEMORY_PROTECTION_DEBUG_PROTOCOL  *mMemoryProtectionProtocol;

//
// Page-Map Level-4 Offset (PML4) and
// Page-Directory-Pointer Offset (PDPE) entries 4K & 2MB
//
typedef union {
  struct {
    UINT64    Present              : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1;  // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;  // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1;  // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;  // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64    Reserved             : 1;  // Reserved
    UINT64    MustBeZero           : 2;  // Must Be Zero
    UINT64    Available            : 3;  // Available for use by system software
    UINT64    PageTableBaseAddress : 40; // Page Table Base Address
    UINT64    AvailableHigh        : 11; // Available for use by system software
    UINT64    Nx                   : 1;  // No Execute bit
  } Bits;
  UINT64    Uint64;
} PAGE_MAP_AND_DIRECTORY_POINTER;

//
// Page Table Entry 4KB
//
typedef union {
  struct {
    UINT64    Present              : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1;  // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;  // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1;  // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;  // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64    Dirty                : 1;  // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64    PAT                  : 1;  //
    UINT64    Global               : 1;  // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64    Available            : 3;  // Available for use by system software
    UINT64    PageTableBaseAddress : 40; // Page Table Base Address
    UINT64    AvailableHigh        : 11; // Available for use by system software
    UINT64    Nx                   : 1;  // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_4K_ENTRY;

//
// Page Table Entry 2MB
//
typedef union {
  struct {
    UINT64    Present              : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1;  // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;  // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1;  // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;  // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64    Dirty                : 1;  // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64    MustBe1              : 1;  // Must be 1
    UINT64    Global               : 1;  // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64    Available            : 3;  // Available for use by system software
    UINT64    PAT                  : 1;  //
    UINT64    MustBeZero           : 8;  // Must be zero;
    UINT64    PageTableBaseAddress : 31; // Page Table Base Address
    UINT64    AvailableHigh        : 11; // Available for use by system software
    UINT64    Nx                   : 1;  // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_ENTRY;

//
// Page Table Entry 1GB
//
typedef union {
  struct {
    UINT64    Present              : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1;  // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;  // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1;  // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;  // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64    Dirty                : 1;  // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64    MustBe1              : 1;  // Must be 1
    UINT64    Global               : 1;  // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64    Available            : 3;  // Available for use by system software
    UINT64    PAT                  : 1;  //
    UINT64    MustBeZero           : 17; // Must be zero;
    UINT64    PageTableBaseAddress : 22; // Page Table Base Address
    UINT64    AvailableHigh        : 11; // Available for use by system software
    UINT64    Nx                   : 1;  // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_1G_ENTRY;

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
  UINT32  RegEax;
  UINT8   PhysicalAddressBits;
  VOID    *Hob;

  //
  // Get physical address bits supported.
  //
  Hob = GetFirstHob (EFI_HOB_TYPE_CPU);
  if (Hob != NULL) {
    PhysicalAddressBits = ((EFI_HOB_CPU *)Hob)->SizeOfMemorySpace;
  } else {
    // Ref. 1: Intel Software Developer's Manual Vol.2, Chapter 3, Section "CPU-Identification".
    // Ref. 2: AMD Software Developer's Manual Vol. 3, Appendix E.
    // Use the 0x80000000 in EAX to determine the largest extended function this CPU supports
    AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
    if (RegEax >= 0x80000008) {
      // If 0x80000008 is supported, query the supported physical address size with it
      AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
      PhysicalAddressBits = (UINT8)RegEax;
    } else {
      // Note: below assumption is only found in Intel Software Developer's Manual Vol.3A, 11.11.2.3
      // If CPUID.80000008H is not available, software may assume that the processor supports
      // a 36-bit physical address size
      PhysicalAddressBits = 36;
    }
  }

  return PhysicalAddressBits;
}

/**
  Initializes the valid bits mask and valid address mask for MTRRs.

  This function initializes the valid bits mask and valid address mask for MTRRs.

  Copied from UefiCpuPkg MtrrLib

  @param[out]  MtrrValidBitsMask     The mask for the valid bit of the MTRR
  @param[out]  MtrrValidAddressMask  The valid address mask for the MTRR

**/
VOID
EFIAPI
InitializeMtrrMask (
  OUT UINT64  *MtrrValidBitsMask,
  OUT UINT64  *MtrrValidAddressMask
  )
{
  UINT32                          MaxExtendedFunction;
  CPUID_VIR_PHY_ADDRESS_SIZE_EAX  VirPhyAddressSize;

  AsmCpuid (CPUID_EXTENDED_FUNCTION, &MaxExtendedFunction, NULL, NULL, NULL);

  if (MaxExtendedFunction >= CPUID_VIR_PHY_ADDRESS_SIZE) {
    AsmCpuid (CPUID_VIR_PHY_ADDRESS_SIZE, &VirPhyAddressSize.Uint32, NULL, NULL, NULL);
  } else {
    VirPhyAddressSize.Bits.PhysicalAddressBits = 36;
  }

  *MtrrValidBitsMask    = LShiftU64 (1, VirPhyAddressSize.Bits.PhysicalAddressBits) - 1;
  *MtrrValidAddressMask = *MtrrValidBitsMask & 0xfffffffffffff000ULL;
}

/*
  Helper function inherited from MinPlatformPkg TestPointCheckSmrr()

  @param[out]   SmrrPhysBaseMsr     Input pointer to hold SMRR Base Msr
  @param[out]   SmrrPhysMaskMsr     Input pointer to hold SMRR Mask Msr

  @retval   EFI_STATUS              The system supports SMRR and pointers will be put in pointers passed in
  @retval   EFI_INVALID_PARAMETER   Input arguments have NULL pointers
  @retval   EFI_UNSUPPORTED         This system does not predefined SMRR in this module

*/
STATIC
EFI_STATUS
LookupSmrrIntel (
  OUT   UINT32  *SmrrPhysBaseMsr,
  OUT   UINT32  *SmrrPhysMaskMsr
  )
{
  EFI_STATUS  Status;

  UINT32  RegEax;
  UINT32  RegEdx;
  UINTN   FamilyId;
  UINTN   ModelId;

  Status = EFI_UNSUPPORTED;
  if ((SmrrPhysBaseMsr == NULL) || (SmrrPhysMaskMsr == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *SmrrPhysBaseMsr = MSR_IA32_SMRR_PHYSBASE;
  *SmrrPhysMaskMsr = MSR_IA32_SMRR_PHYSMASK;

  //
  // Retrieve CPU Family and Model
  //
  AsmCpuid (CPUID_VERSION_INFO, &RegEax, NULL, NULL, &RegEdx);
  FamilyId = (RegEax >> 8) & 0xf;
  ModelId  = (RegEax >> 4) & 0xf;

  if ((FamilyId == 0x06) || (FamilyId == 0x0f)) {
    ModelId = ModelId | ((RegEax >> 12) & 0xf0);
  }

  DEBUG ((DEBUG_INFO, "%a - FamilyId 0x%02x, ModelId 0x%02x\n", __FUNCTION__, FamilyId, ModelId));

  //
  // Check CPUID(CPUID_VERSION_INFO).EDX[12] for MTRR capability
  //
  if ((RegEdx & BIT12) != 0) {
    //
    // Check MTRR_CAP MSR bit 11 for SMRR support
    //
    if ((AsmReadMsr64 (MSR_IA32_MTRRCAP) & BIT11) != 0) {
      Status = EFI_SUCCESS;
    }
  }

  //
  // Intel(R) 64 and IA-32 Architectures Software Developer's Manual
  // Volume 3C, Section 35.3 MSRs in the Intel(R) Atom(TM) Processor Family
  //
  // If CPU Family/Model is 06_1CH, 06_26H, 06_27H, 06_35H or 06_36H, then
  // SMRR Physical Base and SMM Physical Mask MSRs are not available.
  //
  if (FamilyId == 0x06) {
    if ((ModelId == 0x1C) || (ModelId == 0x26) || (ModelId == 0x27) || (ModelId == 0x35) || (ModelId == 0x36)) {
      Status = EFI_UNSUPPORTED;
    }
  }

  //
  // Intel(R) 64 and IA-32 Architectures Software Developer's Manual
  // Volume 3C, Section 35.2 MSRs in the Intel(R) Core(TM) 2 Processor Family
  //
  // If CPU Family/Model is 06_0F or 06_17, then use Intel(R) Core(TM) 2
  // Processor Family MSRs
  //
  if (FamilyId == 0x06) {
    if ((ModelId == 0x17) || (ModelId == 0x0f)) {
      *SmrrPhysBaseMsr = MSR_CORE2_SMRR_PHYSBASE;
      *SmrrPhysMaskMsr = MSR_CORE2_SMRR_PHYSMASK;
    }
  }

  //
  // The above check is great...
  // However, there are some virtual platforms that does not really support them. This PCD check
  // is to allow these virtual platforms to skip the SMRR check.
  //
  if (FixedPcdGetBool (PcdPlatformSmrrUnsupported)) {
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

/*
  Helper function inherited from MinPlatformPkg TestPointCheckSmrr()

  @param[out]   SmrrPhysBaseMsr     Input pointer to hold SMRR Base Msr
  @param[out]   SmrrPhysMaskMsr     Input pointer to hold SMRR Mask Msr

  @retval   EFI_STATUS              The system supports SMRR and pointers will be put in pointers passed in
  @retval   EFI_INVALID_PARAMETER   Input arguments have NULL pointers
  @retval   EFI_UNSUPPORTED         This system does not predefined SMRR in this module

*/
STATIC
EFI_STATUS
LookupSmrrAMD (
  OUT   UINT32  *SmrrPhysBaseMsr,
  OUT   UINT32  *SmrrPhysMaskMsr
  )
{
  EFI_STATUS  Status;

  UINT32  RegEax;
  UINT32  RegEdx;
  UINTN   FamilyId;
  UINTN   ModelId;

  if ((SmrrPhysBaseMsr == NULL) || (SmrrPhysMaskMsr == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *SmrrPhysBaseMsr = AMD_64_SMM_ADDR;
  *SmrrPhysMaskMsr = AMD_64_SMM_MASK;

  //
  // Retrieve CPU Family and Model
  //
  AsmCpuid (CPUID_VERSION_INFO, &RegEax, NULL, NULL, &RegEdx);
  FamilyId = (RegEax >> 8) & 0xf;
  ModelId  = (RegEax >> 4) & 0xf;

  if (FamilyId == 0x0f) {
    // Extended family id and model in use
    FamilyId = FamilyId + ((RegEax >> 20) & 0xff);
    ModelId  = ModelId | ((RegEax >> 12) & 0xf0);
  }

  DEBUG ((DEBUG_INFO, "%a - FamilyId 0x%02x, ModelId 0x%02x\n", __FUNCTION__, FamilyId, ModelId));

  //
  // In processors implementing the AMD64 architecture, SMBASE relocation is always supported.
  // However, there are some virtual platforms that does not really support them. This PCD check
  // is to allow these virtual platforms to skip the SMRR check.
  //
  if (FixedPcdGetBool (PcdPlatformSmrrUnsupported)) {
    Status = EFI_UNSUPPORTED;
  } else {
    Status = EFI_SUCCESS;
  }

  return Status;
}

STATIC
EFI_STATUS
TSEGDumpHandler (
  VOID
  )
{
  UINT32                mSmrrPhysBaseMsr;
  UINT32                mSmrrPhysMaskMsr;
  UINT64                SmrrBase;
  UINT64                SmrrMask;
  UINT64                Length;
  UINT64                MtrrValidBitsMask;
  UINT64                MtrrValidAddressMask;
  INTN                  LowBitPosition;
  INTN                  HighBitPosition;
  UINTN                 NumberOfTseg;
  UINTN                 Index;
  UINTN                 BitIndex;
  UINTN                 RecordIndex;
  CHAR8                 TempString[MAX_STRING_SIZE];
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  *TempBuffer;

  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));

  MtrrValidBitsMask    = 0;
  MtrrValidAddressMask = 0;

  InitializeMtrrMask (&MtrrValidBitsMask, &MtrrValidAddressMask);

  DEBUG ((DEBUG_INFO, "%a MTRR valid bits 0x%016lx, address mask: 0x%016lx\n", __FUNCTION__, MtrrValidBitsMask, MtrrValidAddressMask));

  if (!StandardSignatureIsAuthenticAMD ()) {
    Status = LookupSmrrIntel (&mSmrrPhysBaseMsr, &mSmrrPhysMaskMsr);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a Intel SMRR base and mask cannot be queried! Bail from here!\n", __FUNCTION__));
      return Status;
    }

    // This is a 64-bit read, but the SMRR registers bits 63:32 are reserved.
    SmrrBase = AsmReadMsr64 (mSmrrPhysBaseMsr);
    SmrrMask = AsmReadMsr64 (mSmrrPhysMaskMsr);
    // Extend the mask to account for the reserved bits.
    SmrrMask |= 0xffffffff00000000ULL;

    DEBUG ((DEBUG_VERBOSE, "%a SMRR base 0x%016lx, mask: 0x%016lx\n", __FUNCTION__, SmrrBase, SmrrMask));

    // Extend the top bits of the mask to account for the reserved

    Length = ((~(SmrrMask & MtrrValidAddressMask)) & MtrrValidBitsMask) + 1;

    DEBUG ((DEBUG_VERBOSE, "%a Calculated length: 0x%016lx\n", __FUNCTION__, Length));

    // Writing this out in the format of a Memory Map entry (TSEG_EFI_MEMORY_TYPE will map to TSEG)
    AsciiSPrint (
      TempString,
      MAX_STRING_SIZE,
      "TSEG,0x%016lx,0x%016lx,0x%016lx,0x%016lx,0x%016lx,0x%016lx\n",
      TSEG_EFI_MEMORY_TYPE,
      (SmrrBase & MtrrValidAddressMask),
      0,
      EFI_SIZE_TO_PAGES (Length),
      0,
      NONE_GCD_MEMORY_TYPE
      );
    AppendToMemoryInfoDatabase (TempString);
  } else {
    Status = LookupSmrrAMD (&mSmrrPhysBaseMsr, &mSmrrPhysMaskMsr);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a AMD SMRR base and mask cannot be queried! Bail from here!\n", __FUNCTION__));
      return Status;
    }

    // This is a 64-bit read, but the SMRR registers bits 63:32 are reserved.
    SmrrBase = AsmReadMsr64 (mSmrrPhysBaseMsr);
    SmrrMask = AsmReadMsr64 (mSmrrPhysMaskMsr);

    // Apply the bit mask according to AMD64 Architecture Programmer's Manual Vol. 2-Rev. 3.33 Section 10.2.5:
    // Each CPU memory access is in the TSeg range if the following is true:
    // Phys Addr[51:17] & SMM_MASK[51:17] = SMM_ADDR[51:17] & SMM_MASK[51:17].
    SmrrBase &= (VALID_SMRR_BIT_MASK & MtrrValidAddressMask);
    SmrrMask &= (VALID_SMRR_BIT_MASK & MtrrValidAddressMask);
    DEBUG ((DEBUG_INFO, "%a SMRR base 0x%016lx, mask: 0x%016lx\n", __FUNCTION__, SmrrBase, SmrrMask));

    LowBitPosition  = LowBitSet64 (SmrrMask);
    HighBitPosition = HighBitSet64 (SmrrMask);
    ASSERT (LowBitPosition > 0 && HighBitPosition > 0);

    // For simplicity, let's not allow SMM_ADDR[51:17] != SMM_ADDR[51:17] & SMM_MASK[51:17]
    // Since it means there are bits in SMM BASE ADDR that cannot be honored, why bother setting it...
    ASSERT (SmrrBase == (SmrrBase & SmrrMask));

    // Base: | 0 0 1 0 1 1 0 1 1 1 1 0 0 0 0 0 |
    // Mask: | 0 1 1 0 1 1 0 1 1 1 1 1 0 0 0 0 |
    // HiBitPos--^                   ^--LoBitPos
    // Addr: | 0 0 1 ? 1 1 ? 1 1 1 1 0 x x x x |
    // Ranges:       ^     ^          | Length |

    // So the length for each TSEG range will be (1 << LowBitPosition) - 1
    Length = ((UINT64)1 << LowBitPosition);
    DEBUG ((DEBUG_INFO, "%a Calculated length: 0x%016lx\n", __FUNCTION__, Length));

    // The number of ranges will be 2 ^ (number of 0 bits in the valid region)
    NumberOfTseg = 1;
    for (BitIndex = LowBitPosition + 1; BitIndex <= (UINTN)HighBitPosition; BitIndex++) {
      if ((((UINT64)BIT0<<BitIndex) & SmrrMask) == 0) {
        NumberOfTseg <<= 1;
      }
    }

    TempBuffer = AllocatePool (NumberOfTseg * sizeof (EFI_PHYSICAL_ADDRESS));
    ASSERT (TempBuffer != NULL);

    RecordIndex             = 0;
    TempBuffer[RecordIndex] = SmrrBase;
    // Writing this out in the format of a Memory Map entry (TSEG_EFI_MEMORY_TYPE will map to TSEG)
    AsciiSPrint (
      TempString,
      MAX_STRING_SIZE,
      "TSEG,0x%016lx,0x%016lx,0x%016lx,0x%016lx,0x%016lx,0x%016lx\n",
      TSEG_EFI_MEMORY_TYPE,
      TempBuffer[RecordIndex++],
      0,
      EFI_SIZE_TO_PAGES (Length),
      0,
      NONE_GCD_MEMORY_TYPE
      );
    AppendToMemoryInfoDatabase (TempString);

    for (BitIndex = LowBitPosition + 1; BitIndex <= (UINTN)HighBitPosition; BitIndex++) {
      if ((((UINT64)BIT0<<BitIndex) & SmrrMask) == 0) {
        // Mask is 0 here, basically memory being 0 or 1 can both fit the equations above
        for (Index = 0; Index < RecordIndex; Index++) {
          // Double the content here
          TempBuffer[Index + RecordIndex] = (TempBuffer[Index] | ((UINT64)BIT0<<BitIndex));

          // Writing this out in the format of a Memory Map entry (TSEG_EFI_MEMORY_TYPE will map to TSEG)
          AsciiSPrint (
            TempString,
            MAX_STRING_SIZE,
            "TSEG,0x%016lx,0x%016lx,0x%016lx,0x%016lx,0x%016lx,0x%016lx\n",
            TSEG_EFI_MEMORY_TYPE,
            TempBuffer[Index + RecordIndex],
            0,
            EFI_SIZE_TO_PAGES (Length),
            0,
            NONE_GCD_MEMORY_TYPE
            );
          AppendToMemoryInfoDatabase (TempString);
        }

        RecordIndex <<= 1;
      }
    }

    if (TempBuffer != NULL) {
      FreePool (TempBuffer);
    }
  }

  return EFI_SUCCESS;
}

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
  EFI_STATUS                      Status = EFI_SUCCESS;
  PAGE_MAP_AND_DIRECTORY_POINTER  *Work;
  PAGE_MAP_AND_DIRECTORY_POINTER  *Pml4;
  PAGE_TABLE_1G_ENTRY             *Pte1G;
  PAGE_TABLE_ENTRY                *Pte2M;
  PAGE_TABLE_4K_ENTRY             *Pte4K;
  UINTN                           Index1;
  UINTN                           Index2;
  UINTN                           Index3;
  UINTN                           Index4;
  UINTN                           MyGuardCount        = 0;
  UINTN                           MyPdeCount          = 0;
  UINTN                           My4KCount           = 0;
  UINTN                           My2MCount           = 0;
  UINTN                           My1GCount           = 0;
  UINTN                           NumPage4KNotPresent = 0;
  UINTN                           NumPage2MNotPresent = 0;
  UINTN                           NumPage1GNotPresent = 0;
  UINT64                          Address;

  //
  // First, fail fast if some of the parameters don't look right.
  //
  // ALL count parameters should be provided.
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

  //
  // Alright, let's get to work.
  //
  Pml4 = (PAGE_MAP_AND_DIRECTORY_POINTER *)AsmReadCr3 ();
  // Increase the count.
  // If we have room for more PDE Entries, add one.
  MyPdeCount++;
  if (MyPdeCount <= *PdeCount) {
    PdeEntries[MyPdeCount-1] = (UINT64)(UINTN)Pml4;
  }

  for (Index4 = 0x0; Index4 < 0x200; Index4++) {
    if (!Pml4[Index4].Bits.Present) {
      continue;
    }

    Pte1G = (PAGE_TABLE_1G_ENTRY *)(UINTN)(Pml4[Index4].Bits.PageTableBaseAddress << 12);
    // Increase the count.
    // If we have room for more PDE Entries, add one.
    MyPdeCount++;
    if (MyPdeCount <= *PdeCount) {
      PdeEntries[MyPdeCount-1] = (UINT64)(UINTN)Pte1G;
    }

    for (Index3 = 0x0; Index3 < 0x200; Index3++ ) {
      if (!Pte1G[Index3].Bits.Present) {
        NumPage1GNotPresent++;
        continue;
      }

      //
      // MustBe1 is the bit that indicates whether the pointer is a directory
      // pointer or a page table entry.
      //
      if (!(Pte1G[Index3].Bits.MustBe1)) {
        //
        // We have to cast 1G and 2M directories to this to
        // get all of their address bits.
        //
        Work  = (PAGE_MAP_AND_DIRECTORY_POINTER *)Pte1G;
        Pte2M = (PAGE_TABLE_ENTRY *)(UINTN)(Work[Index3].Bits.PageTableBaseAddress << 12);
        // Increase the count.
        // If we have room for more PDE Entries, add one.
        MyPdeCount++;
        if (MyPdeCount <= *PdeCount) {
          PdeEntries[MyPdeCount-1] = (UINT64)(UINTN)Pte2M;
        }

        for (Index2 = 0x0; Index2 < 0x200; Index2++ ) {
          if (!Pte2M[Index2].Bits.Present) {
            NumPage2MNotPresent++;
            continue;
          }

          if (!(Pte2M[Index2].Bits.MustBe1)) {
            Work  = (PAGE_MAP_AND_DIRECTORY_POINTER *)Pte2M;
            Pte4K = (PAGE_TABLE_4K_ENTRY *)(UINTN)(Work[Index2].Bits.PageTableBaseAddress << 12);
            // Increase the count.
            // If we have room for more PDE Entries, add one.
            MyPdeCount++;
            if (MyPdeCount <= *PdeCount) {
              PdeEntries[MyPdeCount-1] = (UINT64)(UINTN)Pte4K;
            }

            for (Index1 = 0x0; Index1 < 0x200; Index1++ ) {
              if (!Pte4K[Index1].Bits.Present) {
                NumPage4KNotPresent++;
                Address = IndexToAddress (Index4, Index3, Index2, Index1);
                if ((mMemoryProtectionProtocol != NULL) && (mMemoryProtectionProtocol->IsGuardPage (Address))) {
                  MyGuardCount++;
                  if (MyGuardCount <= *GuardCount) {
                    GuardEntries[MyGuardCount - 1] = Address;
                  }

                  continue;
                }
              }

              // Increase the count.
              // If we have room for more Page Table entries, add one.
              My4KCount++;
              if (My4KCount <= *Pte4KCount) {
                Pte4KEntries[My4KCount-1] = Pte4K[Index1].Uint64;
              }
            }
          } else {
            // Increase the count.
            // If we have room for more Page Table entries, add one.
            My2MCount++;
            if (My2MCount <= *Pte2MCount) {
              Pte2MEntries[My2MCount-1] = Pte2M[Index2].Uint64;
            }
          }
        }
      } else {
        // Increase the count.
        // If we have room for more Page Table entries, add one.
        My1GCount++;
        if (My1GCount <= *Pte1GCount) {
          Pte1GEntries[My1GCount-1] = Pte1G[Index3].Uint64;
        }
      }
    }
  }

  DEBUG ((DEBUG_ERROR, "Pages used for Page Tables   = %d\n", MyPdeCount));
  DEBUG ((DEBUG_ERROR, "Number of   4K Pages active  = %d - NotPresent = %d\n", My4KCount, NumPage4KNotPresent));
  DEBUG ((DEBUG_ERROR, "Number of   2M Pages active  = %d - NotPresent = %d\n", My2MCount, NumPage2MNotPresent));
  DEBUG ((DEBUG_ERROR, "Number of   1G Pages active  = %d - NotPresent = %d\n", My1GCount, NumPage1GNotPresent));
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
} // GetFlatPageTableData()

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
  // Dump TSEG Handlers for x64 platforms
  TSEGDumpHandler ();
}
