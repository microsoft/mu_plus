/** @file
  Advanced Logger Cache As Ram (CAR) initialization for SEC


  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <AdvancedLoggerInternal.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/HobLib.h>

#include <Register/Intel/Cpuid.h>
#include <Register/Intel/Msr.h>

#include "AdvancedLoggerSecDebugAgent.h"

#define CACHE_FILL_DATA  0xA5C35A3C

//
// Memory cache types
//
typedef enum {
  CacheUncacheable    = 0,
  CacheWriteCombining = 1,
  CacheWriteThrough   = 4,
  CacheWriteProtected = 5,
  CacheWriteBack      = 6,
  CacheInvalid        = 7
} MTRR_MEMORY_CACHE_TYPE;

/**
  Initializes the valid bits mask and valid address mask for MTRRs.

  This function initializes the valid bits mask and valid address mask for MTRRs.

  @param[out]  MtrrValidBitsMask     The mask for the valid bit of the MTRR
  @param[out]  MtrrValidAddressMask  The valid address mask for the MTRR

**/
VOID
InitializeMtrrMask (
  OUT UINT64 *MtrrValidBitsMask,
  OUT UINT64 *MtrrValidAddressMask
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

    *MtrrValidBitsMask = LShiftU64 (1, VirPhyAddressSize.Bits.PhysicalAddressBits) - 1;
    *MtrrValidAddressMask = *MtrrValidBitsMask & 0xfffffffffffff000ULL;
}

/**
  Get the variable MTRR count for the CPU.

  @return Variable MTRR count

**/
UINT32
GetVariableMtrrCount (
  VOID
  )
{
  MSR_IA32_MTRRCAP_REGISTER MtrrCap;                   // cspell:disable-line

  MtrrCap.Uint64 = AsmReadMsr64 (MSR_IA32_MTRRCAP);    // cspell:disable-line
  return MtrrCap.Bits.VCNT;
}

/**
  AllocateRamForSEC

  For Intel, use Cache as RAM space for the AdvancedLogger memory buffer.

  Uses TempRamBase for SCRATCH_BUFFER_SIZE bytes

  Returns:

    TRUE - Cache as RAM enabled at memory requested.

**/
EFI_PHYSICAL_ADDRESS
EFIAPI
AllocateRamForSEC (
    EFI_PHYSICAL_ADDRESS  CarAddress,
    UINTN                 CarSize
  ) {
    BOOLEAN                 Extendable;
    UINT32                  Index;
    UINT64                  MtrrValidBitsMask;
    UINT64                  MtrrValidAddressMask;
    UINT32                 *p;
    UINT32                 *p2;
    RETURN_STATUS           Status;
    UINT32                  VariableMtrrCount;

    CarAddress = (EFI_PHYSICAL_ADDRESS) FixedPcdGet64 (PcdAdvancedLoggerCarBase);
    if (CarAddress == 0ULL) {
        DEBUG((DEBUG_ERROR, "%a - CAR not allowed.  Base not specified\n", __FUNCTION__));
        return 0ULL;
    }

    ASSERT(CarSize > EFI_PAGE_SIZE);
    if (CarSize <= EFI_PAGE_SIZE) {
        DEBUG((DEBUG_ERROR, "%a - CAR not allowed.  Size too small.\n", __FUNCTION__));
        return 0ULL;
    }

    /**
       This code is not going to alter the current cache map.  It has been set up
       to run this code from cached ROM already.  This code is only going to add the
       In Memory logger address range.  However, the MtrrLib functions to do this
       cannot be used without flushing caches.  That would destroy the current
       environment.

       This code will search the variable MTRR registers looking for an unused MTRR, and
       set that MTRR to point to the memory being used for the in memory log.
    **/

    //
    // Find an empty MTRR and add the CAR range for the in memory buffer
    //
    InitializeMtrrMask (&MtrrValidBitsMask, &MtrrValidAddressMask);
    VariableMtrrCount = GetVariableMtrrCount ();

    Status = RETURN_NOT_FOUND;
    for (Index = 0; Index < VariableMtrrCount; Index++) {
        MSR_IA32_MTRR_PHYSMASK_REGISTER   Mask;
        MSR_IA32_MTRR_PHYSBASE_REGISTER   Base;

        Mask.Uint64 = AsmReadMsr64 (MSR_IA32_MTRR_PHYSMASK0 + (Index << 1));
        if (Mask.Bits.V == 0) {
            Base.Uint64 = CarAddress & MtrrValidAddressMask;
            Base.Bits.Type = CacheWriteBack;
            Mask.Uint64 =  (~((UINT64) (CarSize - 1))) & MtrrValidAddressMask;
            Mask.Bits.V = 1;

            DEBUG ((DEBUG_INFO, "AdvLogger set MTRR[%d] Base = %16.16lx\n", Index, Base));
            DEBUG ((DEBUG_INFO, "AdvLogger set MTRR[%d] Mask = %16.16lx\n", Index, Mask));

            AsmWriteMsr64 (
                MSR_IA32_MTRR_PHYSBASE0 + (Index << 1),
                Base.Uint64
                );

            AsmWriteMsr64 (
                MSR_IA32_MTRR_PHYSMASK0 + (Index << 1),
                Mask.Uint64
                );

            Status = RETURN_SUCCESS;
            break;
        }
    }

    Extendable = FALSE;

    if (Status == RETURN_SUCCESS) {

        Extendable = TRUE;

        DEBUG((DEBUG_ERROR, "Read to fill cache and set cache tags\n"));

        //
        // MTRR has been set. lets run a cache fill by reading the memory logger
        // range.  The data will likely be 0xFF's, but it is important to set the
        // cache tags for the memory range.
        //
        p = (UINT32 *) PTR_FROM_PA (CarAddress);
        p2 = AsmRepLodsd (p, CarSize);

        DEBUG_CODE (
            if (p2 != p+(CarSize / sizeof(UINT32))) {
                DEBUG((DEBUG_ERROR,"RepLodsd did not work correctly. Result is %p\n", p2));
                ASSERT(FALSE);
                Extendable = FALSE;
             }
        );

        // Fill the cache with valid data.
        SetMem32( (VOID *) p, (UINTN) CarSize, CACHE_FILL_DATA);


        DEBUG_CODE (
            // Verify the whole cache range is valid, and we don't crash.
            p = (UINT32 *) PTR_FROM_PA (CarAddress);
            for (Index=0; Index < CarSize; Index+=sizeof(UINT32)) {
                if (*p++ != CACHE_FILL_DATA) {
                    DEBUG((DEBUG_ERROR,"Ba-------- %p ----------ad value %p\n", p, *p));
                    ASSERT(FALSE);
                    Extendable = FALSE;
                }
            }
        );

        if (!Extendable) {
            // Undo possible setting of Physbase3
            FreeRamForSEC (CarAddress);
        } else {
            DEBUG((DEBUG_ERROR, "Cache fill complete for AdvLogger buffer at %16.16lx\n", CarAddress));
        }

    } else {
        DEBUG((DEBUG_ERROR, "Cache failed.  AdvLogger memroy not available\n"));
        Extendable = FALSE;
    }

    if (!Extendable) {
        return 0ULL;
    }

    return CarAddress;
}

/**
  FreeRamForSEC

  Used to dispose of the pre-mem log buffer.

  Disable the Cache as RAM space that was allocated for the AdvancedLogger.

**/
VOID
EFIAPI
FreeRamForSEC (
    EFI_PHYSICAL_ADDRESS Address
  ) {

    MSR_IA32_MTRR_PHYSBASE_REGISTER   Base;
    UINT32                            Index;
    MSR_IA32_MTRR_PHYSMASK_REGISTER   Mask;
    UINT64                            MtrrValidAddressMask;
    UINT64                            MtrrValidBitsMask;
    UINT32                            VariableMtrrCount;

    ASSERT (Address == FixedPcdGet64 (PcdAdvancedLoggerCarBase));

    InitializeMtrrMask (&MtrrValidBitsMask, &MtrrValidAddressMask);

    VariableMtrrCount = GetVariableMtrrCount ();

    // Zero out the borrowed MTRR

    for (Index = 0; Index < VariableMtrrCount; Index++) {
        Base.Uint64 = AsmReadMsr64 (MSR_IA32_MTRR_PHYSBASE0 + (Index << 1));
        Mask.Uint64 = AsmReadMsr64 (MSR_IA32_MTRR_PHYSMASK0 + (Index << 1));

        if ((Mask.Bits.V == 1) &&
            ((Base.Uint64 && MtrrValidAddressMask) == Address))
        {
            AsmWriteMsr64 (
                MSR_IA32_MTRR_PHYSMASK0 + (Index << 1),
                0ULL
            );

            AsmWriteMsr64 (
                MSR_IA32_MTRR_PHYSBASE0 + (Index << 1),
                0ULL
            );

            DEBUG((DEBUG_ERROR, "%a: MTRR[%d] Cleared\n", __FUNCTION__, Index));
            break;
        };
    }
}
