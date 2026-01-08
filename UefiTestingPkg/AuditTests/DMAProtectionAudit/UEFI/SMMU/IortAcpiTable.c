/** @file -- IortAcpiTable.c

This file contains functions to parse the IORT ACPI table for SMMUv3 nodes
and Reserved Memory Range (RMR) nodes.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/IoRemappingTable.h>
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
    *SmmuCount = 0;
    *SmmuBaseAddresses = NULL;
    return EFI_NOT_FOUND;
  }

  // Allocate array for SMMU base addresses
  LocalSmmuBaseAddresses = AllocateZeroPool (LocalSmmuCount * sizeof (UINT64));
  if (LocalSmmuBaseAddresses == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory for SMMU base addresses\n", __func__));
    *SmmuCount = 0;
    *SmmuBaseAddresses = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  // Second pass: collect base addresses
  Node = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Iort + Iort->NodeOffset);
  SmmuIndex = 0;

  for (Count = 0; Count < Iort->NumNodes; Count++) {
    if (Node->Type == EFI_ACPI_IORT_TYPE_SMMUv3) {
      SmmuNode = (EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE *)Node;
      LocalSmmuBaseAddresses[SmmuIndex] = SmmuNode->Base;
      DEBUG ((DEBUG_INFO, "%a: Found SMMUv3 at base 0x%lX\n", __func__, SmmuNode->Base));
      SmmuIndex++;
    }
    Node = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Node + Node->Length);
  }

  *SmmuCount = LocalSmmuCount;
  *SmmuBaseAddresses = LocalSmmuBaseAddresses;

  return EFI_SUCCESS;
}

/**
  Parse the IORT table to find all RMR (Reserved Memory Range) nodes
  and return a linked list of memory ranges.

  This function iterates through all nodes in the IORT table looking for
  RMR nodes (type 0x6). For each RMR node found, it extracts all memory
  range descriptors and adds them to the returned linked list.

  @param[in] IortTable          Pointer to the IORT table.

  @retval Pointer to head of linked list of RMR entries, or NULL if none found.
**/
RMRListNode*
EFIAPI
GetIortAcpiTableRmrList (
  IN EFI_ACPI_DESCRIPTION_HEADER  *IortTable
  )
{
  EFI_ACPI_6_0_IO_REMAPPING_TABLE          *Iort;
  EFI_ACPI_6_0_IO_REMAPPING_NODE           *Node;
  EFI_ACPI_6_0_IO_REMAPPING_RMR_NODE       *RmrNode;
  EFI_ACPI_6_0_IO_REMAPPING_MEM_RANGE_DESC *MemRangeDesc;
  RMRListNode                              *Head;
  RMRListNode                              *Current;
  RMRListNode                              *NewNode;
  UINT32                                   Count;
  UINT32                                   MemRangeIndex;

  if (IortTable == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: IORT table not available\n", __func__));
    return NULL;
  }

  Iort = (EFI_ACPI_6_0_IO_REMAPPING_TABLE *)IortTable;
  Node = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Iort + Iort->NodeOffset);

  Head    = NULL;
  Current = NULL;

  // Iterate through all nodes looking for RMR nodes
  for (Count = 0; Count < Iort->NumNodes; Count++) {
    if (Node->Type == EFI_ACPI_IORT_TYPE_RMR) {
      RmrNode = (EFI_ACPI_6_0_IO_REMAPPING_RMR_NODE *)Node;
      DEBUG ((DEBUG_INFO, "%a: Found RMR node with %d memory range descriptors\n",
                 __func__, RmrNode->NumMemRangeDesc));

      // Get pointer to memory range descriptor array
      // MemRangeDescRef is offset from the start of the RMR node
      MemRangeDesc = (EFI_ACPI_6_0_IO_REMAPPING_MEM_RANGE_DESC *)((UINT8 *)RmrNode + RmrNode->MemRangeDescRef);

      // Iterate through all memory range descriptors in this RMR node
      for (MemRangeIndex = 0; MemRangeIndex < RmrNode->NumMemRangeDesc; MemRangeIndex++) {
        // Only add valid ranges (non-zero base and length)
        if ((MemRangeDesc[MemRangeIndex].Base > 0) && (MemRangeDesc[MemRangeIndex].Length > 0)) {
          NewNode = AllocateZeroPool (sizeof (RMRListNode));
          if (NewNode == NULL) {
            DEBUG ((DEBUG_ERROR, "%a: Failed to allocate RMRListNode\n", __func__));
            // Return what we have so far
            return Head;
          }

          NewNode->BaseAddress = MemRangeDesc[MemRangeIndex].Base;
          NewNode->Length      = MemRangeDesc[MemRangeIndex].Length;
          NewNode->Next        = NULL;

          DEBUG ((DEBUG_INFO, "%a: Adding RMR range Base=0x%lX, Length=0x%lX\n",
                    __func__, NewNode->BaseAddress, NewNode->Length));

          // Add to linked list
          if (Head == NULL) {
            Head    = NewNode;
            Current = NewNode;
          } else {
            Current->Next = NewNode;
            Current       = NewNode;
          }
        }
      }
    }

    // Move to the next node
    Node = (EFI_ACPI_6_0_IO_REMAPPING_NODE *)((UINT8 *)Node + Node->Length);
  }

  return Head;
}
