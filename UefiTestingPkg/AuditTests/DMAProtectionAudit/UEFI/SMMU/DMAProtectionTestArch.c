/** @file -- DMAProtectionTestArch.c

This file contains architecture specific DMA protection tests for ARM SMMU (SMMUv3):
1) Check the CR0 registers of the SMMUv3 nodes to verify SMMU translation is enabled
2) Check that Command Queue is enabled (CMDQEN bit in CR0)
3) Check that Event Queue is enabled (EVTQEN bit in CR0)
4) Check that Stream Table Base is configured (STRTAB_BASE is not NULL)
5) Check that GERROR register is 0 (no global errors)
6) Check RMR (Reserved Memory Range) regions from IORT are set as reserved in memory map

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UnitTestLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>

#include "DmaProtection.h"

/// ================================================================================================
/// ================================================================================================
///
/// TEST CASES
///
/// ================================================================================================
/// ================================================================================================

/**
  Test to verify that the Reserved Memory Range (RMR) regions defined in the IORT
  are properly marked as reserved in the EFI memory map.

  @param[in] Context  The unit test context (not used).

  @retval UNIT_TEST_PASSED           All RMR regions are properly marked as reserved.
  @retval UNIT_TEST_ERROR_TEST_FAILED A RMR region was not found or not marked as reserved.
**/
UNIT_TEST_STATUS
EFIAPI
CheckExcludedRegions (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                   Status;
  EFI_MEMORY_DESCRIPTOR        *EfiMemoryMap;
  EFI_MEMORY_DESCRIPTOR        *EfiMemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR        *EfiMemNext;
  UINTN                        EfiMemoryMapSize;
  UINTN                        EfiMapKey;
  UINTN                        EfiDescriptorSize;
  UINT32                       EfiDescriptorVersion;
  EFI_ACPI_DESCRIPTION_HEADER  *IortTable;
  RMRListNode                  *Head;
  RMRListNode                  *Current;
  BOOLEAN                      Found;
  UNIT_TEST_STATUS             TestStatus;

  //
  // Step 1: Get IORT Table
  //
  IortTable = NULL;
  Status    = GetIortAcpiTable (&IortTable);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Step 2: Get the RMR (Reserved Memory Range) nodes from IORT Table
  //
  Head = GetIortAcpiTableRmrList (IortTable);
  if (Head == NULL) {
    UT_LOG_INFO ("No RMRs Found in IORT\n");
    DEBUG ((DEBUG_INFO, "%a: No RMRs Found in IORT\n", __func__));
    return UNIT_TEST_PASSED;
  }

  Current = Head;

  //
  // Step 3: Get the EFI memory map.
  //
  EfiMemoryMapSize = 0;
  EfiMemoryMap     = NULL;
  Status           = gBS->GetMemoryMap (
                            &EfiMemoryMapSize,
                            EfiMemoryMap,
                            &EfiMapKey,
                            &EfiDescriptorSize,
                            &EfiDescriptorVersion
                            );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    EfiMemoryMap = (EFI_MEMORY_DESCRIPTOR *)AllocateZeroPool (EfiMemoryMapSize + 8*EfiDescriptorSize);
    UT_ASSERT_NOT_NULL (EfiMemoryMap);

    Status = gBS->GetMemoryMap (
                    &EfiMemoryMapSize,
                    EfiMemoryMap,
                    &EfiMapKey,
                    &EfiDescriptorSize,
                    &EfiDescriptorVersion
                    );
    UT_ASSERT_NOT_EFI_ERROR (Status);
  } else {
    UT_LOG_ERROR ("GetMemoryMap Failed\n");
    DEBUG ((DEBUG_ERROR, "%a: GetMemoryMap Failed\n", __func__));
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  //
  // Step 4: Step through memory map and verify each
  //         RMR memory range is marked reserved
  //
  EfiMemoryMapEnd = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)EfiMemoryMap + EfiMemoryMapSize);
  TestStatus      = UNIT_TEST_PASSED;

  while (Current != NULL) {
    Found      = FALSE;
    EfiMemNext = EfiMemoryMap;

    UT_LOG_INFO ("Checking RMR region: Base=0x%lX, Length=0x%lX\n", Current->BaseAddress, Current->Length);
    DEBUG ((DEBUG_INFO, "%a: Checking RMR region: Base=0x%lX, Length=0x%lX\n", __func__, Current->BaseAddress, Current->Length));

    while (EfiMemNext < EfiMemoryMapEnd) {
      // Check if memory range fully encompasses RMR
      if ((EfiMemNext->PhysicalStart <= Current->BaseAddress) &&
          ((EfiMemNext->PhysicalStart + (EFI_PAGE_SIZE * EfiMemNext->NumberOfPages)) >= (Current->BaseAddress + Current->Length)))
      {
        UT_LOG_INFO (
          "Found encompassing memory range: Base=0x%lX, Length=0x%lX, Type=%d\n",
          EfiMemNext->PhysicalStart,
          EFI_PAGE_SIZE * EfiMemNext->NumberOfPages,
          EfiMemNext->Type
          );
        DEBUG ((
          DEBUG_INFO,
          "%a: Found encompassing memory range: Base=0x%lX, Length=0x%lX, Type=%d\n",
          __func__,
          EfiMemNext->PhysicalStart,
          EFI_PAGE_SIZE * EfiMemNext->NumberOfPages,
          EfiMemNext->Type
          ));

        // Verify memory range is marked as reserved
        if (EfiMemNext->Type == EfiReservedMemoryType) {
          UT_LOG_INFO (
            "RMR between 0x%lX and 0x%lX found with reserved memory type %d\n",
            Current->BaseAddress,
            Current->BaseAddress + Current->Length,
            EfiMemNext->Type
            );
          DEBUG ((
            DEBUG_INFO,
            "%a: RMR between 0x%lX and 0x%lX found with reserved memory type %d\n",
            __func__,
            Current->BaseAddress,
            Current->BaseAddress + Current->Length,
            EfiMemNext->Type
            ));
          Found = TRUE;
        }

        break;
      }

      // Move on to next descriptor
      EfiMemNext = NEXT_MEMORY_DESCRIPTOR (EfiMemNext, EfiDescriptorSize);
    }

    if (!Found) {
      UT_LOG_ERROR (
        "RMR between 0x%lX and 0x%lX NOT found with reserved memory type!\n",
        Current->BaseAddress,
        Current->BaseAddress + Current->Length
        );
      DEBUG ((
        DEBUG_ERROR,
        "%a: RMR between 0x%lX and 0x%lX NOT found with reserved memory type!\n",
        __func__,
        Current->BaseAddress,
        Current->BaseAddress + Current->Length
        ));
      TestStatus = UNIT_TEST_ERROR_TEST_FAILED;
    }

    Current = Current->Next;
  }

  UT_LOG_INFO ("%a: Result=%d\n", __func__, TestStatus);
  DEBUG ((DEBUG_INFO, "%a: Result=%d\n", __func__, TestStatus));

  return TestStatus;
} // CheckExcludedRegions()

/**
  Test to verify that all SMMUv3 units found in the IORT have translation enabled.
  This checks:
  1) CR0 register's SMMUEN bit to confirm the SMMU is actively translating
  2) CR0 register's CMDQEN bit to confirm the command queue is enabled
  3) CR0 register's EVTQEN bit to confirm the event queue is enabled
  4) STRTAB_BASE register is not NULL (stream table must be configured)
  5) GERROR register is 0 (no global errors)

  @param[in] Context  The unit test context (not used).

  @retval UNIT_TEST_PASSED           All SMMU units are properly configured.
  @retval UNIT_TEST_ERROR_TEST_FAILED An SMMU unit is not properly configured.
**/
UNIT_TEST_STATUS
EFIAPI
CheckIOMMUEnabled (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                   Status;
  EFI_ACPI_DESCRIPTION_HEADER  *IortTable;
  UINTN                        SmmuCount;
  UINT64                       *SmmuBaseAddresses;
  UINTN                        Iterator;
  UINT32                       Cr0Value;
  UINT32                       SmmuEnBit;
  UINT32                       CmdQEnBit;
  UINT32                       EvtQEnBit;
  UINT64                       StrTabBase;
  UINT64                       StrTabBaseAddr;
  UINT32                       GError;
  UNIT_TEST_STATUS             TestStatus;

  //
  // Step 1: Get IORT Table
  //
  IortTable = NULL;
  Status    = GetIortAcpiTable (&IortTable);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Step 2: Find and parse SMMUv3 nodes from IORT
  //
  SmmuCount         = 0;
  SmmuBaseAddresses = NULL;
  Status            = ParseIortAcpiTableSmmu (IortTable, &SmmuCount, &SmmuBaseAddresses);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Step 3: Check that we found at least one SMMU
  //
  UT_ASSERT_TRUE (SmmuCount > 0);
  UT_LOG_INFO ("Found %d SMMUv3 units in IORT\n", SmmuCount);
  DEBUG ((DEBUG_INFO, "%a: Found %d SMMUv3 units in IORT\n", __func__, SmmuCount));

  TestStatus = UNIT_TEST_PASSED;

  //
  // Step 4: For each SMMU, check:
  //         - SMMU Enable bit (SMMUEN) in CR0 register
  //         - Command Queue Enable bit (CMDQEN) in CR0 register
  //         - Event Queue Enable bit (EVTQEN) in CR0 register
  //         - Stream Table Base address is not NULL
  //         - GERROR register is 0
  //
  for (Iterator = 0; Iterator < SmmuCount; Iterator++) {
    UT_LOG_INFO ("Checking SMMUv3 at base address 0x%lX\n", SmmuBaseAddresses[Iterator]);
    DEBUG ((DEBUG_INFO, "%a: Checking SMMUv3 at base address 0x%lX\n", __func__, SmmuBaseAddresses[Iterator]));

    //
    // Read CR0 register
    //
    Cr0Value = MmioRead32 ((UINTN)(SmmuBaseAddresses[Iterator] + SMMU_CR0));
    UT_LOG_INFO ("CR0 Register Value: 0x%X\n", Cr0Value);
    DEBUG ((DEBUG_INFO, "%a: CR0 Register Value: 0x%X\n", __func__, Cr0Value));

    //
    // Check SMMUEN bit (bit 0)
    //
    SmmuEnBit = Cr0Value & SMMU_CR0_SMMUEN;
    UT_LOG_INFO ("SMMUEN bit: %d\n", SmmuEnBit);
    DEBUG ((DEBUG_INFO, "%a: SMMUEN bit: %d\n", __func__, SmmuEnBit));
    if (SmmuEnBit == 0) {
      UT_LOG_ERROR ("SMMUEN bit is disabled for SMMUv3 at base address 0x%lX\n", SmmuBaseAddresses[Iterator]);
      DEBUG ((DEBUG_ERROR, "%a: SMMUEN bit is disabled for SMMUv3 at base address 0x%lX\n", __func__, SmmuBaseAddresses[Iterator]));
      TestStatus = UNIT_TEST_ERROR_TEST_FAILED;
    }

    //
    // Check CMDQEN bit (bit 3) - Command Queue must be enabled
    //
    CmdQEnBit = Cr0Value & SMMU_CR0_CMDQEN;
    UT_LOG_INFO ("CMDQEN bit: %d\n", CmdQEnBit ? 1 : 0);
    DEBUG ((DEBUG_INFO, "%a: CMDQEN bit: %d\n", __func__, CmdQEnBit ? 1 : 0));
    if (CmdQEnBit == 0) {
      UT_LOG_ERROR ("CMDQEN bit is disabled for SMMUv3 at base address 0x%lX\n", SmmuBaseAddresses[Iterator]);
      DEBUG ((DEBUG_ERROR, "%a: CMDQEN bit is disabled for SMMUv3 at base address 0x%lX\n", __func__, SmmuBaseAddresses[Iterator]));
      TestStatus = UNIT_TEST_ERROR_TEST_FAILED;
    }

    //
    // Check EVTQEN bit (bit 2) - Event Queue must be enabled
    //
    EvtQEnBit = Cr0Value & SMMU_CR0_EVTQEN;
    UT_LOG_INFO ("EVTQEN bit: %d\n", EvtQEnBit ? 1 : 0);
    DEBUG ((DEBUG_INFO, "%a: EVTQEN bit: %d\n", __func__, EvtQEnBit ? 1 : 0));
    if (EvtQEnBit == 0) {
      UT_LOG_ERROR ("EVTQEN bit is disabled for SMMUv3 at base address 0x%lX\n", SmmuBaseAddresses[Iterator]);
      DEBUG ((DEBUG_ERROR, "%a: EVTQEN bit is disabled for SMMUv3 at base address 0x%lX\n", __func__, SmmuBaseAddresses[Iterator]));
      TestStatus = UNIT_TEST_ERROR_TEST_FAILED;
    }

    //
    // Read STRTAB_BASE register and check it's not NULL
    // If NULL, no stream IDs can undergo translation
    //
    StrTabBase = MmioRead64 ((UINTN)(SmmuBaseAddresses[Iterator] + SMMU_STRTAB_BASE));
    UT_LOG_INFO ("STRTAB_BASE Register Value: 0x%lX\n", StrTabBase);
    DEBUG ((DEBUG_INFO, "%a: STRTAB_BASE Register Value: 0x%lX\n", __func__, StrTabBase));

    // Extract the address portion by masking out lower 6 bits (bits [5:0] are reserved/config)
    StrTabBaseAddr = StrTabBase & ~SMMU_STRTAB_BASE_ADDR_MASK;
    UT_LOG_INFO ("STRTAB_BASE Address: 0x%lX\n", StrTabBaseAddr);
    DEBUG ((DEBUG_INFO, "%a: STRTAB_BASE Address: 0x%lX\n", __func__, StrTabBaseAddr));
    if (StrTabBaseAddr == 0) {
      UT_LOG_ERROR ("STRTAB_BASE is NULL for SMMUv3 at base address 0x%lX\n", SmmuBaseAddresses[Iterator]);
      DEBUG ((DEBUG_ERROR, "%a: STRTAB_BASE is NULL for SMMUv3 at base address 0x%lX\n", __func__, SmmuBaseAddresses[Iterator]));
      TestStatus = UNIT_TEST_ERROR_TEST_FAILED;
    }

    //
    // Read GERROR register and check it's 0 (no global errors)
    //
    GError = MmioRead32 ((UINTN)(SmmuBaseAddresses[Iterator] + SMMU_GERROR));
    UT_LOG_INFO ("GERROR Register Value: 0x%X\n", GError);
    DEBUG ((DEBUG_INFO, "%a: GERROR Register Value: 0x%X\n", __func__, GError));
    if (GError != 0) {
      UT_LOG_ERROR ("GERROR register is non-zero for SMMUv3 at base address 0x%lX\n", SmmuBaseAddresses[Iterator]);
      DEBUG ((DEBUG_ERROR, "%a: GERROR register is non-zero for SMMUv3 at base address 0x%lX\n", __func__, SmmuBaseAddresses[Iterator]));
      TestStatus = UNIT_TEST_ERROR_TEST_FAILED;
    }
  }

  UT_LOG_INFO ("%a: Result=%d\n", __func__, TestStatus);
  DEBUG ((DEBUG_INFO, "%a: Result=%d\n", __func__, TestStatus));

  return TestStatus;
} // CheckIOMMUEnabled()
