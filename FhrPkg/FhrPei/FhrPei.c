/** @file
  The PEI module for supporting FHR. This module will enforce

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <PiPei.h>
#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/UefiLib.h>

#include <Fhr.h>
#include <Library/FhrLib.h>

/**
  Reserves OS reclaimable memory from the associated cold boot to prevent future
  allocations from overlapping with OS memory.

  @param[in]  FwData      The firmware data stored by the associated cold boot.

  @retval   EFI_SUCCESS             OS memory successfully reserved.
  @retval   EFI_INVALID_PARAMETER   Firmware data is invalid.
**/
EFI_STATUS
ReserveOsMemory (
  IN FHR_FW_DATA  *FwData
  )
{
  EFI_MEMORY_DESCRIPTOR  *Entry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;
  EFI_PHYSICAL_ADDRESS   RegionStart;
  UINT64                 RegionLength;

  if ((FwData == NULL) ||
      (FwData->MemoryMapOffset == 0) ||
      (FwData->MemoryMapSize == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  Entry        = (EFI_MEMORY_DESCRIPTOR *)(((UINT8 *)FwData) + FwData->MemoryMapOffset);
  MemoryMapEnd = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)Entry + FwData->MemoryMapSize);
  RegionStart  = MAX_UINT64;
  RegionLength = 0;

  while (Entry < MemoryMapEnd) {
    //
    // Reserve this chunk if the continuos memory had ended.
    //

    if (FHR_IS_RUNTIME_MEMORY (Entry->Type) ||
        (Entry->PhysicalStart != RegionStart + RegionLength))
    {
      if (RegionLength > 0) {
        ASSERT (RegionStart != MAX_UINT64);
        DEBUG ((DEBUG_INFO, "[FHR PEI] Reserving OS owned memory. [0x%llx : 0x%llx]\n", RegionStart, RegionLength));
        BuildMemoryAllocationHob (RegionStart, RegionLength, EfiLoaderData);
        RegionStart  = MAX_UINT64;
        RegionLength = 0;
      }
    }

    //
    // Start or continue the region if this is OS reclaimable memory.
    //

    if (!FHR_IS_RUNTIME_MEMORY (Entry->Type)) {
      if (RegionStart == MAX_UINT64) {
        RegionStart = Entry->PhysicalStart;
      }

      RegionLength += Entry->NumberOfPages * EFI_PAGE_SIZE;
    }

    Entry = NEXT_MEMORY_DESCRIPTOR (Entry, FwData->MemoryMapDescriptorSize);
  }

  if (RegionLength > 0) {
    ASSERT (RegionStart != MAX_UINT64);
    DEBUG ((DEBUG_INFO, "[FHR PEI] Reserving OS owned memory. [0x%llx : 0x%llx]\n", RegionStart, RegionLength));
    BuildMemoryAllocationHob (RegionStart, RegionLength, EfiLoaderData);
  }

  return EFI_SUCCESS;
}

/**
  Prepares the system for a resume from FHR.

  @param[in]  FhrHob      The platform provided FHR HOB.

  @retval   EFI_SUCCESS                 FHR resume successfully prepared.
  @retval   EFI_INVALID_PARAMETER       FHR firmware data is invalid.
**/
EFI_STATUS
EFIAPI
PrepareFhrResume (
  IN FHR_HOB  *FhrHob
  )

{
  EFI_STATUS                  Status;
  FHR_FW_DATA                 *FwData;
  EFI_HOB_HANDOFF_INFO_TABLE  *HandOffHob;

  DEBUG ((DEBUG_INFO, "[FHR PEI] Preparing FHR resume.\n"));

  ASSERT (FhrHob->IsFhrBoot);

  //
  // Validate that the PEI memory exists within the FHR region.
  //

  HandOffHob = (EFI_HOB_HANDOFF_INFO_TABLE *)GetHobList ();
  if ((HandOffHob->EfiMemoryBottom < FhrHob->FhrReservedBase) ||
      (HandOffHob->EfiMemoryTop > (FhrHob->FhrReservedBase + FhrHob->FhrReservedSize)))
  {
    DEBUG ((
      DEBUG_ERROR,
      "[FHR PEI] PEI memory outside of reserved region! "
      "Memory: [0x%llx - 0x%llx] Reserved: [0x%llx - 0x%llx]\n",
      HandOffHob->EfiMemoryBottom,
      HandOffHob->EfiMemoryTop,
      FhrHob->FhrReservedBase,
      (FhrHob->FhrReservedBase + FhrHob->FhrReservedSize)
      ));
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  //
  // Validate the FW data at the beginning of the FHR region.
  //

  FwData = (FHR_FW_DATA *)(VOID *)(UINTN)FhrHob->FhrReservedBase;
  Status = FhrValidateFwData (FwData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR PEI] Invalid FW data, failing to resume FHR.\n"));
    goto Exit;
  }

  if ((FwData->FwRegionBase != FhrHob->FhrReservedBase) ||
      (FwData->FwRegionLength != FhrHob->FhrReservedSize))
  {
    DEBUG ((
      DEBUG_ERROR,
      "[FHR PEI] Mismatched firmware region. HOB: [0x%llx, 0x%llx] Stored: [0x%llx, 0x%llx]\n",
      FhrHob->FhrReservedBase,
      FhrHob->FhrReservedSize,
      FwData->FwRegionBase,
      FwData->FwRegionLength
      ));
    goto Exit;
  }

  //
  // Create an allocation hob to ensure the FW data is preserved.
  //

  BuildMemoryAllocationHob (FwData->FwRegionBase, ALIGN_VALUE (FwData->Size, EFI_PAGE_SIZE), EfiReservedMemoryType);

  // Create the allocation hob for the FW data to keep is untouched.
  Status = ReserveOsMemory (FwData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR PEI] Failed to create allocation HOBs for OS memory.\n"));
    goto Exit;
  }

Exit:
  return Status;
}

/**
  Entry point for the FHR PEI module. On FHR resume, this is responsible for
  doing initial validation and reserving OS owned memory on the system. On cold
  boot, this routine will ensure that the FHR hob exists and allocate the
  firmware reserved region.

  @param[in]  FileHandle        Unused.
  @param[in]  PeiServices       Unused.

  @retval   EFI_SUCCESS             Successfully configured PEI FHR support.
  @retval   EFI_INVALID_PARAMETER   Invalid platform configuration.
  @retval   EFI_OUT_OF_RESOURCES    Unable to build HOBs.
**/
EFI_STATUS
EFIAPI
FhrPeiEntry (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_HOB_GUID_TYPE     *GuidHob;
  FHR_HOB               *FhrHob;
  EFI_PHYSICAL_ADDRESS  ReservedBase;
  UINT64                ReservedLength;
  FHR_FW_DATA           *FwData;

  FhrHob = NULL;

  //
  // Check if this is an FHR boot.
  //

  GuidHob = GetFirstGuidHob (&gFhrHobGuid);
  if (GuidHob != NULL) {
    FhrHob = (FHR_HOB *)GET_GUID_HOB_DATA (GuidHob);
    if (FhrHob->IsFhrBoot) {
      return PrepareFhrResume (FhrHob);
    }
  }

  //
  // This is not an FHR resume, prepare FHR support.
  //

  ReservedBase   = PcdGet64 (PcdFhrReservedBlockBase);
  ReservedLength = PcdGet64 (PcdFhrReservedBlockLength);
  DEBUG ((DEBUG_INFO, "[FHR PEI] Preparing FHR reserved region. Base 0x%llx Length: 0x%llx\n", ReservedBase, ReservedLength));

  if ((ReservedBase == 0) || (ReservedLength == 0) ||
      ((ReservedBase & EFI_PAGE_MASK) != 0) ||
      ((ReservedLength & EFI_PAGE_MASK) != 0))
  {
    DEBUG ((DEBUG_ERROR, "[FHR PEI] Invalid FHR reserved region PCDs!\n"));
    return EFI_INVALID_PARAMETER;
  }

  if ((ReservedBase + ReservedLength) > MAX_UINTN) {
    DEBUG ((DEBUG_ERROR, "[FHR PEI] Reserved region exceeds addressable memory!\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Create the allocation HOB to ensure this stays reserved. Note, this memory
  // may not actually exist yet.
  //

  BuildMemoryAllocationHob (ReservedBase, ReservedLength, EfiReservedMemoryType);

  //
  // Initialize the FHR Data section. The checksum will be done when it is
  // finalized in DXE.
  //

  FwData = (VOID *)(UINTN)ReservedBase;
  ZeroMem (FwData, sizeof (FHR_FW_DATA));
  FwData->Signature      = FHR_PAGE_SIGNATURE;
  FwData->FwRegionBase   = ReservedBase;
  FwData->FwRegionLength = ReservedLength;
  FwData->HeaderSize     = sizeof (FHR_FW_DATA);
  FwData->Size           = sizeof (FHR_FW_DATA);

  //
  // If the HOB doesn't exist, then add it to indicate FHR support.
  //

  if (FhrHob == NULL) {
    FhrHob = BuildGuidHob (&gFhrHobGuid, sizeof (FHR_HOB));
    if (FhrHob == NULL) {
      DEBUG ((DEBUG_ERROR, "[FHR PEI] Failed to create FHR hob!\n"));
      return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem (FhrHob, sizeof (FhrHob));
  }

  //
  // Ensure the FHR HOB information is accurate.
  //

  FhrHob->IsFhrBoot       = FALSE;
  FhrHob->FhrReservedBase = ReservedBase;
  FhrHob->FhrReservedSize = ReservedLength;
  return EFI_SUCCESS;
}
