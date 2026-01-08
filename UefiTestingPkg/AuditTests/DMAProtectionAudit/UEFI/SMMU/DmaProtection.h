/** @file -- DmaProtection.h

Header file for SMMU DMA protection tests.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DMA_PROTECTION_H_
#define _DMA_PROTECTION_H_

#include <Uefi.h>
#include <IndustryStandard/Acpi.h>

//
// IORT Table Signature
//
#define EFI_ACPI_6_0_IO_REMAPPING_TABLE_SIGNATURE  SIGNATURE_32('I', 'O', 'R', 'T')

//
// SMMUv3 Register Offsets
//
#define SMMU_CR0            0x0020
#define SMMU_CR0ACK         0x0024
#define SMMU_GERROR         0x0060
#define SMMU_STRTAB_BASE    0x0080

//
// SMMUv3 CR0 Register Bits
//
#define SMMU_CR0_SMMUEN   BIT0  // SMMU Enable bit
#define SMMU_CR0_EVTQEN   BIT2  // Event Queue Enable bit
#define SMMU_CR0_CMDQEN   BIT3  // Command Queue Enable bit

//
// STRTAB_BASE lower bits mask (bits [5:0] are reserved/config)
//
#define SMMU_STRTAB_BASE_ADDR_MASK  0x3FULL

//
// RMR List Node Structure for tracking Reserved Memory Ranges
//
typedef struct _RMRListNode {
  UINT64                 BaseAddress;
  UINT64                 Length;
  struct _RMRListNode    *Next;
} RMRListNode;

//
// Function Prototypes
//

/**
  Get the IORT ACPI table from the system.

  @param[out] IortTable         Pointer to receive the IORT table pointer.

  @retval EFI_SUCCESS           The IORT table was found.
  @retval EFI_NOT_FOUND         The IORT table was not found.
  @retval EFI_INVALID_PARAMETER IortTable is NULL.
**/
EFI_STATUS
EFIAPI
GetIortAcpiTable (
  OUT EFI_ACPI_DESCRIPTION_HEADER  **IortTable
  );

/**
  Parse the IORT table to find all SMMUv3 nodes.

  @param[in]  IortTable         Pointer to the IORT table.
  @param[out] SmmuCount         Pointer to receive the count of SMMUv3 nodes.
  @param[out] SmmuBaseAddresses Pointer to receive the array of SMMU base addresses.
                                Caller must free this memory when done.

  @retval EFI_SUCCESS           SMMUv3 nodes were found and parsed.
  @retval EFI_NOT_FOUND         No SMMUv3 nodes found.
  @retval EFI_INVALID_PARAMETER A required parameter is NULL.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
ParseIortAcpiTableSmmu (
  IN  EFI_ACPI_DESCRIPTION_HEADER  *IortTable,
  OUT UINTN                        *SmmuCount,
  OUT UINT64                       **SmmuBaseAddresses
  );

/**
  Parse the IORT table to find all RMR (Reserved Memory Range) nodes.

  @param[in] IortTable          Pointer to the IORT table.

  @retval Pointer to head of linked list of RMR entries, or NULL if none found.
**/
RMRListNode*
EFIAPI
GetIortAcpiTableRmrList (
  IN EFI_ACPI_DESCRIPTION_HEADER  *IortTable
  );

#endif // _DMA_PROTECTION_H_
