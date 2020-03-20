/** @file -- DMAProtectionTestArch.c

This file contains architecture specific DMA protection tests:
1) Check the Global Status Registers of the DRHDs to verify VTd is enabled
2) Check RMRR memory ranges are set as reserved

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/UnitTestBootLib.h>

#include "DmaProtection.h"


///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================

UNIT_TEST_STATUS
EFIAPI
CheckExcludedRegions (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS                  Status;
  EFI_MEMORY_DESCRIPTOR       *EfiMemoryMap;
  EFI_MEMORY_DESCRIPTOR       *EfiMemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR       *EfiMemNext;
  UINTN                       EfiMemoryMapSize;
  UINTN                       EfiMapKey;
  UINTN                       EfiDescriptorSize;
  UINT32                      EfiDescriptorVersion;
  RMRRListNode*               Head;
  RMRRListNode*               Current;

  //
  // Step 1: Get DMAR Table
  //
  Status = GetDmarAcpiTable();
  UT_ASSERT_NOT_EFI_ERROR(Status);

  //
  // Step 2: Get the RMRR Headers from DMAR Table
  //
  Head = GetDmarAcpiTableRmrr();
  if (Head == NULL)
  {
    UT_LOG_INFO("No RMRRs Found\n");
    return UNIT_TEST_PASSED;
  }

  Current = Head;

  //
  // Step 3: Get the EFI memory map.
  //
  EfiMemoryMapSize  = 0;
  EfiMemoryMap      = NULL;
  Status = gBS->GetMemoryMap (
                  &EfiMemoryMapSize,
                  EfiMemoryMap,
                  &EfiMapKey,
                  &EfiDescriptorSize,
                  &EfiDescriptorVersion
                  );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    EfiMemoryMap = (EFI_MEMORY_DESCRIPTOR*)AllocateZeroPool(EfiMemoryMapSize + 8*EfiDescriptorSize);

    Status = gBS->GetMemoryMap (
                  &EfiMemoryMapSize,
                  EfiMemoryMap,
                  &EfiMapKey,
                  &EfiDescriptorSize,
                  &EfiDescriptorVersion
                  );
    UT_ASSERT_NOT_EFI_ERROR(Status);
  }
  else {
    UT_LOG_ERROR("GetMemoryMap Failed\n");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  //
  // Step 4: Step through memory map and verify each
  //         RMRR memory ranges are marked reserved
  //
  EfiMemNext = EfiMemoryMap;
  EfiMemoryMapEnd = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)EfiMemoryMap + EfiMemoryMapSize);

  while (EfiMemNext < EfiMemoryMapEnd) {
    //Check if memory range fully encompasses RMRR
    if((EfiMemNext->PhysicalStart <= Current->RMRR->ReservedMemoryRegionBaseAddress)
    && (EfiMemNext->PhysicalStart + (EFI_PAGE_SIZE * EfiMemNext->NumberOfPages) >= Current->RMRR->ReservedMemoryRegionLimitAddress)) {
      //Verify memory range is marked as reserved
      UT_ASSERT_EQUAL(EfiMemNext->Type, EfiReservedMemoryType);
      UT_LOG_INFO("RMRRs between %X and %X found with type EfiReservedMemoryType\n", Current->RMRR->ReservedMemoryRegionBaseAddress, Current->RMRR->ReservedMemoryRegionLimitAddress);

      //Move on to next RMRR and start search at beginning of memory map again
      EfiMemNext = EfiMemoryMap;
      if(Current->Next == NULL)
      {
        return UNIT_TEST_PASSED;
      }
      Current = Current->Next;
    }
    else
    {
      //Move on to next descriptor
      EfiMemNext = NEXT_MEMORY_DESCRIPTOR( EfiMemNext, EfiDescriptorSize );

      //RMRR Not found in memory map
      UT_ASSERT_TRUE(EfiMemNext < EfiMemoryMapEnd);
    }
  }

  //Should never reach here
  return UNIT_TEST_ERROR_TEST_FAILED;
} // CheckExcludedRegions()

UNIT_TEST_STATUS
EFIAPI
CheckIOMMUEnabled (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS                  Status;
  UINTN                       iterator;
  UINT32                      Reg32;

  //
  // Step 1: Get DMAR Table
  //
  Status = GetDmarAcpiTable();
  UT_ASSERT_NOT_EFI_ERROR(Status);

  //
  // Step 2: Find memory offset of DRHDs
  //
  Status = ParseDmarAcpiTableDrhd();
  UT_ASSERT_NOT_EFI_ERROR(Status);

  //
  // Step 3: Check Translation Enabled bit of each status register
  //
  for(iterator = 0; iterator < mVtdUnitNumber; iterator++)
  {
    Reg32 = MmioRead32 (mVtdUnitInformation[iterator].VtdUnitBaseAddress + R_GSTS_REG);
    UT_LOG_INFO("Global Status Register %X\n", Reg32);
    UINT32 DmaBit = (Reg32 & B_GSTS_REG_TE);
    UT_ASSERT_NOT_EQUAL(DmaBit, 0);
  }

  return UNIT_TEST_PASSED;
} // CheckIOMMUEnabled()
