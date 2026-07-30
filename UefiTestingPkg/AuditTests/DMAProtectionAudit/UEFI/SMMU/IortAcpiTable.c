/** @file -- IortAcpiTable.c

This file contains functions to parse the IORT ACPI table for SMMUv3 nodes
and Reserved Memory Range (RMR) nodes.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/IoRemappingTable.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include "../Acpi.h"
#include "DmaProtection.h"

/**
  Get the IORT ACPI table.

  @param[out] IortTable         Pointer to receive the IORT table pointer.

  @retval EFI_SUCCESS           The IORT ACPI table is got.
  @retval EFI_ALREADY_STARTED   The IORT ACPI table has been got previously.
  @retval EFI_NOT_FOUND         The IORT ACPI table is not found.
  @retval EFI_INVALID_PARAMETER IortTable is NULL.
**/
EFI_STATUS
EFIAPI
GetIortAcpiTable (
  OUT EFI_ACPI_DESCRIPTION_HEADER  **IortTable
  )
{
  if (IortTable == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return GetAcpiTable (EFI_ACPI_6_0_IO_REMAPPING_TABLE_SIGNATURE, (VOID **)IortTable);
}

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
  )
{
  EFI_ACPI_6_0_IO_REMAPPING_TABLE       *Iort;
  EFI_ACPI_6_0_IO_REMAPPING_NODE        *Node;
  EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE  *SmmuNode;
  UINT32                                Count;
  UINTN                                 SmmuIndex;
  UINTN                                 LocalSmmuCount;
  UINT64                                *LocalSmmuBaseAddresses;

  if ((IortTable == NULL) || (SmmuCount == NULL) || (SmmuBaseAddresses == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Iort = (EFI_ACPI_6_0_IO_REMAPPING_TABLE *)IortTable;
  Node = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Iort + Iort->NodeOffset);

  // First pass: count SMMUv3 nodes
  LocalSmmuCount = 0;
  for (Count = 0; Count < Iort->NumNodes; Count++) {
    if (Node->Type == EFI_ACPI_IORT_TYPE_SMMUv3) {
      LocalSmmuCount++;
    }

    Node = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Node + Node->Length);
  }

  if (LocalSmmuCount == 0) {
    DEBUG ((DEBUG_ERROR, "%a: No SMMUv3 nodes found\n", __func__));
    *SmmuCount         = 0;
    *SmmuBaseAddresses = NULL;
    return EFI_NOT_FOUND;
  }

  // Allocate array for SMMU base addresses
  LocalSmmuBaseAddresses = AllocateZeroPool (LocalSmmuCount * sizeof (UINT64));
  if (LocalSmmuBaseAddresses == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory for SMMU base addresses\n", __func__));
    *SmmuCount         = 0;
    *SmmuBaseAddresses = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  // Second pass: collect base addresses
  Node      = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Iort + Iort->NodeOffset);
  SmmuIndex = 0;

  for (Count = 0; Count < Iort->NumNodes; Count++) {
    if (Node->Type == EFI_ACPI_IORT_TYPE_SMMUv3) {
      SmmuNode                          = (EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE *)Node;
      LocalSmmuBaseAddresses[SmmuIndex] = SmmuNode->Base;
      SmmuIndex++;
    }

    Node = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Node + Node->Length);
  }

  *SmmuCount         = LocalSmmuCount;
  *SmmuBaseAddresses = LocalSmmuBaseAddresses;

  return EFI_SUCCESS;
}

/**
  Parse the IORT table and append each Reserved Memory Range (RMR) descriptor
  to the caller-provided doubly-linked list.

  This function iterates through all nodes in the IORT table looking for RMR
  nodes (type 0x6). For each RMR node found, every valid memory range
  descriptor (non-zero base and length) is wrapped in an RMR_LIST_NODE and
  appended to RmrList via InsertTailList.

  The caller must have initialized RmrList (e.g. with InitializeListHead)
  before calling this function, and is responsible for freeing every appended
  entry (e.g. via RemoveEntryList + FreePool) when done. Entries appended
  before an EFI_OUT_OF_RESOURCES return must also be freed.

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
  )
{
  EFI_ACPI_6_0_IO_REMAPPING_TABLE           *Iort;
  EFI_ACPI_6_0_IO_REMAPPING_NODE            *Node;
  EFI_ACPI_6_0_IO_REMAPPING_RMR_NODE        *RmrNode;
  EFI_ACPI_6_0_IO_REMAPPING_MEM_RANGE_DESC  *MemRangeDesc;
  RMR_LIST_NODE                             *NewNode;
  UINT32                                    Count;
  UINT32                                    MemRangeIndex;

  if ((IortTable == NULL) || (RmrList == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Iort = (EFI_ACPI_6_0_IO_REMAPPING_TABLE *)IortTable;
  Node = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Iort + Iort->NodeOffset);

  // Iterate through all nodes looking for RMR nodes
  for (Count = 0; Count < Iort->NumNodes; Count++) {
    if (Node->Type == EFI_ACPI_IORT_TYPE_RMR) {
      RmrNode = (EFI_ACPI_6_0_IO_REMAPPING_RMR_NODE *)Node;

      // Get pointer to memory range descriptor array
      // MemRangeDescRef is offset from the start of the RMR node
      MemRangeDesc = (EFI_ACPI_6_0_IO_REMAPPING_MEM_RANGE_DESC *)((UINT8 *)RmrNode + RmrNode->MemRangeDescRef);

      // Iterate through all memory range descriptors in this RMR node
      for (MemRangeIndex = 0; MemRangeIndex < RmrNode->NumMemRangeDesc; MemRangeIndex++) {
        // Only add valid ranges (non-zero base and length)
        if ((MemRangeDesc[MemRangeIndex].Base > 0) && (MemRangeDesc[MemRangeIndex].Length > 0)) {
          NewNode = AllocateZeroPool (sizeof (RMR_LIST_NODE));
          if (NewNode == NULL) {
            DEBUG ((DEBUG_ERROR, "%a: Failed to allocate RMR_LIST_NODE\n", __func__));
            // Leave already-appended entries in RmrList; caller must free them.
            return EFI_OUT_OF_RESOURCES;
          }

          NewNode->Signature   = RMR_LIST_NODE_SIGNATURE;
          NewNode->BaseAddress = MemRangeDesc[MemRangeIndex].Base;
          NewNode->Length      = MemRangeDesc[MemRangeIndex].Length;

          InsertTailList (RmrList, &NewNode->Link);
        }
      }
    }

    // Move to the next node
    Node = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Node + Node->Length);
  }

  return EFI_SUCCESS;
}
