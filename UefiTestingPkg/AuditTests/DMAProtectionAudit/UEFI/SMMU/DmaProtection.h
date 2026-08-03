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
#define SMMU_CR0          0x0020
#define SMMU_CR0ACK       0x0024
#define SMMU_GBPA         0x0044
#define SMMU_GERROR       0x0060
#define SMMU_STRTAB_BASE  0x0080

//
// SMMUv3 CR0 Register Bits
//
#define SMMU_CR0_SMMUEN  BIT0   // SMMU Enable bit
#define SMMU_CR0_EVTQEN  BIT2   // Event Queue Enable bit
#define SMMU_CR0_CMDQEN  BIT3   // Command Queue Enable bit

//
// SMMUv3 GBPA Register Bits
//
#define SMMU_GBPA_ABORT  BIT20  // Global Bypass Abort bit (abort all DMA when SMMUEN == 0)

//
// STRTAB_BASE lower bits mask (bits [5:0] are reserved/config)
//
#define SMMU_STRTAB_BASE_ADDR_MASK  0x3FULL

//
// Signature and structure used to track Reserved Memory Ranges (RMRs) parsed
// from the IORT. Nodes are linked into a doubly-linked list rooted at a
// caller-provided LIST_ENTRY head. Use RMR_LIST_NODE_FROM_LINK() to recover
// the containing RMR_LIST_NODE from a LIST_ENTRY *.
//
#define RMR_LIST_NODE_SIGNATURE  SIGNATURE_32 ('R', 'M', 'R', 'N')

typedef struct {
  UINT32        Signature;
  LIST_ENTRY    Link;
  UINT64        BaseAddress;
  UINT64        Length;
} RMR_LIST_NODE;

#define RMR_LIST_NODE_FROM_LINK(a)  CR (a, RMR_LIST_NODE, Link, RMR_LIST_NODE_SIGNATURE)

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
  Parse the IORT table and append each Reserved Memory Range (RMR) descriptor
  to the caller-provided doubly-linked list.

  Each entry appended is an RMR_LIST_NODE. The caller is responsible for:
    - Initializing RmrList (e.g. via InitializeListHead) before calling.
    - Freeing every appended RMR_LIST_NODE (e.g. via RemoveEntryList + FreePool)
      when done. Entries appended before an error return must also be freed.

  If no RMR nodes are found, EFI_SUCCESS is returned and RmrList is left empty
  (IsListEmpty returns TRUE).

  @param[in]     IortTable  Pointer to the IORT table.
  @param[in,out] RmrList    List head to append RMR entries to.

  @retval EFI_SUCCESS           IORT was parsed; RmrList may be empty if there
                                are no RMR nodes.
  @retval EFI_INVALID_PARAMETER IortTable or RmrList is NULL.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate an RMR_LIST_NODE. Any entries
                                appended before the failure remain in RmrList.
**/
EFI_STATUS
EFIAPI
GetIortAcpiTableRmrList (
  IN     EFI_ACPI_DESCRIPTION_HEADER  *IortTable,
  IN OUT LIST_ENTRY                   *RmrList
  );

#endif // _DMA_PROTECTION_H_
