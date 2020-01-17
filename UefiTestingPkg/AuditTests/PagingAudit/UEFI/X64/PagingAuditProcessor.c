/** @file -- DxePagingAuditApp.c
Platform specific memory handler dump function. Handler(s) need to be in compliance
with existed Windows\PagingReportGenerator.py, i.e. TSEG.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
/** 

Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Register/Msr.h>
#include "../PagingAuditCommon.h"


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
  OUT UINT64 *MtrrValidBitsMask,
  OUT UINT64 *MtrrValidAddressMask
  )
{
  UINT32                          MaxExtendedFunction;
  CPUID_VIR_PHY_ADDRESS_SIZE_EAX  VirPhyAddressSize;


  AsmCpuid( CPUID_EXTENDED_FUNCTION, &MaxExtendedFunction, NULL, NULL, NULL );

  if (MaxExtendedFunction >= CPUID_VIR_PHY_ADDRESS_SIZE) {
    AsmCpuid( CPUID_VIR_PHY_ADDRESS_SIZE, &VirPhyAddressSize.Uint32, NULL, NULL, NULL );
  } else {
    VirPhyAddressSize.Bits.PhysicalAddressBits = 36;
  }

  *MtrrValidBitsMask = LShiftU64( 1, VirPhyAddressSize.Bits.PhysicalAddressBits ) - 1;
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
  OUT   UINT32    *SmrrPhysBaseMsr,
  OUT   UINT32    *SmrrPhysMaskMsr
  )
{
  EFI_STATUS  Status;

  UINT32      RegEax;
  UINT32      RegEdx;
  UINTN       FamilyId;
  UINTN       ModelId;

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

  if (FamilyId == 0x06 || FamilyId == 0x0f) {
    ModelId = ModelId | ((RegEax >> 12) & 0xf0);
  }
  DEBUG (( DEBUG_INFO, "%a - FamilyId 0x%02x, ModelId 0x%02x\n", __FUNCTION__, FamilyId, ModelId ));

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
    if (ModelId == 0x1C || ModelId == 0x26 || ModelId == 0x27 || ModelId == 0x35 || ModelId == 0x36) {
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
    if (ModelId == 0x17 || ModelId == 0x0f) {
      *SmrrPhysBaseMsr = MSR_CORE2_SMRR_PHYSBASE;
      *SmrrPhysMaskMsr = MSR_CORE2_SMRR_PHYSMASK;
    }
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
  OUT   UINT32    *SmrrPhysBaseMsr,
  OUT   UINT32    *SmrrPhysMaskMsr
  )
{
  EFI_STATUS  Status;

  UINT32      RegEax;
  UINT32      RegEdx;
  UINTN       FamilyId;
  UINTN       ModelId;

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
    FamilyId = FamilyId + (RegEax >> 20) & 0xff;
    ModelId = ModelId | ((RegEax >> 12) & 0xf0);
  }
  DEBUG (( DEBUG_INFO, "%a - FamilyId 0x%02x, ModelId 0x%02x\n", __FUNCTION__, FamilyId, ModelId ));

  //
  // In processors implementing the AMD64 architecture, SMBASE relocation is always supported
  //
  Status = EFI_SUCCESS;

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
CheckSmrrSupported (
  OUT   UINT32    *SmrrPhysBaseMsr,
  OUT   UINT32    *SmrrPhysMaskMsr
  )
{
  EFI_STATUS  Status;

  UINT32      RegEbx;
  UINT32      RegEcx;
  UINT32      RegEdx;

  DEBUG (( DEBUG_INFO, "%a - Enter\n", __FUNCTION__ ));

  Status = EFI_UNSUPPORTED;
  if ((SmrrPhysBaseMsr == NULL) || (SmrrPhysMaskMsr == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  AsmCpuid (CPUID_SIGNATURE, NULL, &RegEbx, &RegEcx, &RegEdx);
  if ((RegEbx == CPUID_SIGNATURE_GENUINE_INTEL_EBX) &&
      (RegEcx == CPUID_SIGNATURE_GENUINE_INTEL_ECX) &&
      (RegEdx == CPUID_SIGNATURE_GENUINE_INTEL_EDX)) {
    // Deal with intel cpus
    Status = LookupSmrrIntel (SmrrPhysBaseMsr, SmrrPhysMaskMsr);
  }
  else if ((RegEbx == CPUID_SIGNATURE_AUTHENTIC_AMD_EBX) &&
          (RegEcx == CPUID_SIGNATURE_AUTHENTIC_AMD_ECX) &&
          (RegEdx == CPUID_SIGNATURE_AUTHENTIC_AMD_EDX)) {
    // Deal with AMD cpus
    Status = LookupSmrrAMD (SmrrPhysBaseMsr, SmrrPhysMaskMsr);
  }
  else {
    Status = EFI_UNSUPPORTED;
  }

Cleanup:
  DEBUG (( DEBUG_INFO, "%a - Exit %r\n", __FUNCTION__, Status ));
  return Status;
}


STATIC
EFI_STATUS
TSEGDumpHandler (
  VOID
  )
{
  UINT32      mSmrrPhysBaseMsr;
  UINT32      mSmrrPhysMaskMsr;
  UINT64      SmrrBase;
  UINT64      SmrrMask;
  UINT64      Length;
  UINT64      MtrrValidBitsMask;
  UINT64      MtrrValidAddressMask;
  CHAR8       TempString[MAX_STRING_SIZE];
  EFI_STATUS  Status;

  DEBUG(( DEBUG_INFO, "%a()\n", __FUNCTION__ ));

  MtrrValidBitsMask = 0;
  MtrrValidAddressMask = 0;

  InitializeMtrrMask( &MtrrValidBitsMask, &MtrrValidAddressMask );

  DEBUG(( DEBUG_VERBOSE, "%a MTRR valid bits 0x%016lx, address mask: 0x%016lx\n", __FUNCTION__, MtrrValidBitsMask , MtrrValidAddressMask ));

  Status = CheckSmrrSupported(&mSmrrPhysBaseMsr, &mSmrrPhysMaskMsr);
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a SMRR base and mask cannot be queried! Bail from here!\n", __FUNCTION__ ));
    return Status;
  }

  // This is a 64-bit read, but the SMRR registers bits 63:32 are reserved.
  SmrrBase = AsmReadMsr64( mSmrrPhysBaseMsr );
  SmrrMask = AsmReadMsr64( mSmrrPhysMaskMsr );
  // Extend the mask to account for the reserved bits.
  SmrrMask |= 0xffffffff00000000ULL;

  DEBUG(( DEBUG_VERBOSE, "%a SMRR base 0x%016lx, mask: 0x%016lx\n", __FUNCTION__, SmrrBase , SmrrMask ));

  // Extend the top bits of the mask to account for the reserved

  Length = ((~(SmrrMask & MtrrValidAddressMask)) & MtrrValidBitsMask) + 1;

  DEBUG(( DEBUG_VERBOSE, "%a Calculated length: 0x%016lx\n", __FUNCTION__, Length ));

  // Writing this out in the format of a Memory Map entry (Type 16 will map to TSEG)
  AsciiSPrint( TempString, MAX_STRING_SIZE,
               "TSEG,0x%016lx,0x%016lx,0x%016lx,0x%016lx,0x%016lx\n",
               16,
               (SmrrBase & MtrrValidAddressMask),
               0,
               EFI_SIZE_TO_PAGES( Length ),
               0 );
  AppendToMemoryInfoDatabase( TempString );

  return EFI_SUCCESS;
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
  // Dump TSEG Handlers for x64 platforms
  TSEGDumpHandler();
}
