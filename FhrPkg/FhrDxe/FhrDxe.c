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

BOOLEAN      mIsFhrResume;
FHR_FW_DATA  *mFwData;

/**
Notify function for running and acting on the requests (input, debug, etc)

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
  EFI_STATUS   Status;
  VOID         *MemoryMap;
  UINTN        MemoryMapSize;
  UINTN        DescriptorSize;
  UINT32       DescriptorVersion;

  ASSERT (!mIsFhrResume);
  ASSERT (mFwData != NULL);

  DEBUG ((DEBUG_INFO, "[FHR DXE] Finalized FHR firmware data block.\n"));
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

  mFwData->Size     = (UINT32)(mFwData->MemoryMapOffset + mFwData->MemoryMapSize);
  mFwData->Checksum = 0;
  mFwData->Checksum = CalculateCheckSum64 ((VOID *)mFwData, mFwData->Size);

  //
  // TODO: Implement OS indication for FHR support.
  //

  DEBUG ((DEBUG_INFO, "[FHR DXE] FHR support finalized.\n"));
  return;
}

/**

Routine Description:

  TODO

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
    ASSERT (FALSE);
    DEBUG ((DEBUG_ERROR, "[FHR DXE] Firmware data pointer is NULL!\n"));
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
      DEBUG ((DEBUG_ERROR, "%a - Create Event Ex for PostReadyToBoot. Code = %r\n", __FUNCTION__, Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}
