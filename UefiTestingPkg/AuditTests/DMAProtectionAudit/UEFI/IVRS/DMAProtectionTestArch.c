/** @file -- DMAProtectionTestArch.c

This file contains architecture specific DMA protection tests:
1) Check the Global Status Registers of the DRHDs to verify VTd is enabled
2) Check IVMD memory ranges are set as reserved

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

#include "IVRS.h"
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
  IVMDListNode*               Head;
  IVMDListNode*               Current;

  //
  // Step 1: Get IVRS Table
  //
  Status = GetIvrsAcpiTable();
  UT_ASSERT_NOT_EFI_ERROR(Status);

  //
  // Step 2: Get the IVMD Headers from IVRS Table
  //
  Head = GetIvrsAcpiTableIvmd();
  if (Head == NULL)
  {
    UT_LOG_INFO("No IVMDs Found\n");
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
  //         IVMD memory ranges are marked reserved
  //
  EfiMemNext = EfiMemoryMap;
  EfiMemoryMapEnd = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)EfiMemoryMap + EfiMemoryMapSize);

  while (EfiMemNext < EfiMemoryMapEnd) {
    //Check if memory range fully encompasses IVMD
    if((EfiMemNext->PhysicalStart <= Current->IVMD->IVMDStartAddress)
    && ((EfiMemNext->PhysicalStart + EFI_PAGE_SIZE * EfiMemNext->NumberOfPages) >= (Current->IVMD->IVMDStartAddress + Current->IVMD->IVMDMemoryBlockLength))) {
      //Verify memory range is marked as reserved
      UT_ASSERT_EQUAL(EfiMemNext->Type, EfiACPIMemoryNVS);
      UT_LOG_INFO("IVMDs between %X and %X found with type EfiACPIMemoryNVS\n", Current->IVMD->IVMDStartAddress, Current->IVMD->IVMDStartAddress + Current->IVMD->IVMDMemoryBlockLength);

      //Move on to next IVMD and start search at beginning of memory map again
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

      //IVMD Not found in memory map
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
  UINT32                      IommuEn;

  //
  // Step 1: Get IVRS Table
  //
  Status = GetIvrsAcpiTable();
  UT_ASSERT_NOT_EFI_ERROR(Status);

  //
  // Step 2: Find memory offset of IVHDs
  //
  Status = ParseIvrsAcpiTableIvhd();
  UT_ASSERT_NOT_EFI_ERROR(Status);

  //
  // Step 3: Check Translation Enabled bit of each status register
  //
  for(iterator = 0; iterator < mIvhdUnitNumber; iterator++)
  {
    UT_LOG_INFO("Global Status Register %X\n", mIvhdUnitInformation[iterator].IOMMUBaseAddress);
    IommuEn = (MmioRead64(mIvhdUnitInformation[iterator].IOMMUBaseAddress + IOMMU_CONTROL_REG)) & BIT0;
    UT_ASSERT_EQUAL(IommuEn, 1);
  }

  return UNIT_TEST_PASSED;
} // CheckDMAEnabled()
