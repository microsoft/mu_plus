/** @file
  This module handles re-authentication of existing MFCI
  Policies and ingestion of new policies.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Fhr.h>
#include <Library/FhrLib.h>

BOOLEAN      mIsFhrResume;
FHR_FW_DATA  *mFwData;

/**
  Notify function for PostReadyToBoot event. This routine will capture final
  memory state and determine reported FHR support.

  @param[in]  Event   The Event that is being processed.
  @param[in]  Context The Event Context.
**/
VOID
EFIAPI
OnPostReadyToBootNotification (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;
  VOID        *MemoryMap;
  UINTN       MemoryMapSize;
  UINTN       DescriptorSize;
  UINT32      DescriptorVersion;

  ASSERT (!mIsFhrResume);
  ASSERT (mFwData != NULL);

  DEBUG ((DEBUG_INFO, "[FHR DXE] Finalizing FHR firmware data block.\n"));
  gBS->CloseEvent (Event);

  //
  // Capture the memory map at boot to evaluate in FHR resume.
  //

  MemoryMap     = (VOID *)(mFwData + 1);
  MemoryMapSize = FHR_MAX_FW_DATA_SIZE - sizeof (FHR_FW_DATA);

  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  MemoryMap,
                  NULL,
                  &DescriptorSize,
                  &DescriptorVersion
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR DXE] Failed to collect BM memory map! (%r)\n", Status));
    return;
  }

  mFwData->MemoryMapOffset            = sizeof (FHR_FW_DATA);
  mFwData->MemoryMapSize              = MemoryMapSize;
  mFwData->MemoryMapDescriptorSize    = DescriptorSize;
  mFwData->MemoryMapDescriptorVersion = DescriptorVersion;

  //
  // Update the size and checksum.
  //

  mFwData->Size = (UINT32)(mFwData->MemoryMapOffset + mFwData->MemoryMapSize);
  FhrUpdateFwDataChecksum (mFwData);

  //
  // TODO: Implement OS indication for FHR support. Not yet finalized in spec.
  //

  DEBUG ((DEBUG_INFO, "[FHR DXE] FHR support finalized.\n"));
  return;
}

/**
  Validates that no two PEI allocations overlap. This can occur if a PEI
  allocation moves and intersects with OS memory or the FHR reserved region.
  Such and overlap can cause unexpected use of memory or potential corruption,
  especially during early memory allocation.

  @param[in]  FhrHob      The platform provided FHR HOB.

  @retval   TRUE    PEI memory successfully validated.
  @retval   FALSE   PEI memory allocations failed.
**/
BOOLEAN
FhrValidatePeiAllocations (
  IN FHR_HOB  *FhrHob
  )
{
  EFI_HOB_MEMORY_ALLOCATION  *AllocHob;
  EFI_HOB_MEMORY_ALLOCATION  *CompareHob;
  EFI_PHYSICAL_ADDRESS       AllocBase;
  EFI_PHYSICAL_ADDRESS       CompareBase;
  EFI_PHYSICAL_ADDRESS       AllocEnd;
  EFI_PHYSICAL_ADDRESS       CompareEnd;

  AllocHob = (EFI_HOB_MEMORY_ALLOCATION *)GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
  ASSERT (AllocHob != NULL);
  while (AllocHob != NULL) {
    CompareHob = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, GET_NEXT_HOB (AllocHob));
    while (CompareHob != NULL) {
      AllocBase   = AllocHob->AllocDescriptor.MemoryBaseAddress;
      AllocEnd    = AllocBase + AllocHob->AllocDescriptor.MemoryLength;
      CompareBase = CompareHob->AllocDescriptor.MemoryBaseAddress;
      CompareEnd  = CompareBase + CompareHob->AllocDescriptor.MemoryLength;
      if ((AllocBase < CompareEnd) && (CompareBase < AllocEnd)) {
        if ((AllocBase == CompareBase) &&
            (AllocEnd == CompareEnd) &&
            (AllocHob->AllocDescriptor.MemoryType ==  CompareHob->AllocDescriptor.MemoryType))
        {
          //
          // Duplicates should not be fatal, but might indicate a benign bug.
          //
          DEBUG ((
            DEBUG_WARN,
            "[FHR DXE] Found duplicate PEI allocation. 0x%llx : 0x%llx (%d)\n",
            AllocHob->AllocDescriptor.MemoryBaseAddress,
            AllocHob->AllocDescriptor.MemoryLength,
            AllocHob->AllocDescriptor.MemoryType
            ));
        } else {
          DEBUG ((
            DEBUG_ERROR,
            "[FHR DXE] Found overlapping PEI allocations. [0x%llx : 0x%llx (%d)] [0x%llx : 0x%llx (%d)]\n",
            AllocHob->AllocDescriptor.MemoryBaseAddress,
            AllocHob->AllocDescriptor.MemoryLength,
            AllocHob->AllocDescriptor.MemoryType,
            CompareHob->AllocDescriptor.MemoryBaseAddress,
            CompareHob->AllocDescriptor.MemoryLength,
            CompareHob->AllocDescriptor.MemoryType
            ));
          return FALSE;
        }
      }

      CompareHob = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, GET_NEXT_HOB (CompareHob));
    }

    AllocHob = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, GET_NEXT_HOB (AllocHob));
  }

  return TRUE;
}

/**

Routine Description:

  Entry point for FHR DXE module. Prepares FHR data anf resume state.

Arguments:

  @param[in]  ImageHandle -- Handle to this image.
  @param[in]  SystemTable -- Pointer to the system table.

  @retval EFI_STATUS
**/
EFI_STATUS
EFIAPI
FhrDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  FHR_HOB            *FhrHob;
  EFI_EVENT          PostReadyToBootEvent;
  EFI_STATUS         Status;

  //
  // Check if this is an FHR resume.
  //

  GuidHob = GetFirstGuidHob (&gFhrHobGuid);
  if (GuidHob == NULL ) {
    DEBUG ((DEBUG_ERROR, "[FHR DXE] Failed to find FHR HOB!\n"));
    return EFI_NOT_FOUND;
  }

  FhrHob       = (FHR_HOB *)GET_GUID_HOB_DATA (GuidHob);
  mIsFhrResume = FhrHob->IsFhrBoot;
  mFwData      = (VOID *)FhrHob->FhrReservedBase;

  if (mFwData == NULL) {
    DEBUG ((DEBUG_ERROR, "[FHR DXE] Firmware data pointer is NULL!\n"));
    ASSERT (FALSE);
    return EFI_NOT_FOUND;
  }

  //
  // Check that the PEI module initialized the data.
  //

  ASSERT (mFwData->Signature == FHR_PAGE_SIGNATURE);
  ASSERT (mFwData->FwRegionBase == FhrHob->FhrReservedBase);
  ASSERT (mFwData->FwRegionLength == FhrHob->FhrReservedSize);
  ASSERT ((mIsFhrResume) || (mFwData->HeaderSize == sizeof (FHR_FW_DATA)));

  //
  // Validate PEI allocations to avoid memory conflicts.
  //

  if (!FhrValidatePeiAllocations (FhrHob)) {
    DEBUG ((DEBUG_ERROR, "[FHR DXE] Failed to validate PEI memory allocations!\n"));
    ASSERT (FALSE);
    return EFI_PROTOCOL_ERROR;
  }

  //
  // If this is not an FHR resume, then register for post ready to boot. This
  // will be used to evaluate memory usage and capture any final state.
  //

  if (!mIsFhrResume) {
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    OnPostReadyToBootNotification,
                    NULL,
                    &gEfiEventPostReadyToBootGuid,
                    &PostReadyToBootEvent
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "[FHR DXE] Failed to create event for PostReadyToBoot. (%r)\n",
        Status
        ));

      return Status;
    }
  }

  return EFI_SUCCESS;
}
