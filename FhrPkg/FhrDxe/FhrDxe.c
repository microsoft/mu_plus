/** @file
  This module handles preparing the DXE phase of FHR. This includes validating
  FHR state and preparing the final FHR support and data blocks.

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
#include <Library/UefiBootManagerLib.h>

#include <Library/DxeServicesLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/LoadedImage.h>

BOOLEAN      mIsFhrResume;
FHR_FW_DATA  *mFwData;
UINT16       BootOptionNumber = MAX_UINT16;

/**
  Handles a FHR resume critical failure. This routine does not return.

  @param[in]  Failure     The reason for the FHR resume failure.
**/
VOID
FailFhrResume (
  IN FHR_FAILURE_REASON  Failure,
  IN EFI_STATUS          FailureStatus
  )
{
  //
  // TODO: diagnostics.
  //

  DEBUG ((
    DEBUG_ERROR,
    "[FHR DXE] Fatal FHR resume failure! Reason: %d Status: %r\n",
    Failure,
    FailureStatus
    ));

  gRT->ResetSystem (EfiResetWarm, FailureStatus, 0, NULL);
  CpuDeadLoop ();
  ASSERT (FALSE);
}

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
  UINTN       VariableSize;
  UINT16      BootCurrent;

  ASSERT (mFwData != NULL);

  gBS->CloseEvent (Event);

  if (mIsFhrResume) {
    //
    // Validate that BootCurrent is pointer at FhrResume.
    //
    DEBUG ((DEBUG_INFO, "[FHR DXE] Verifying boot option number.\n"));
    ASSERT (BootOptionNumber != MAX_UINT16);

    //
    // Get the number of the Boot#### option that the status code applies to.
    //
    VariableSize = sizeof BootCurrent;
    Status       = gRT->GetVariable (
                          L"BootCurrent",
                          &gEfiGlobalVariableGuid,
                          NULL,
                          &VariableSize,
                          &BootCurrent
                          );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "[FHR DXE] Failed to get BootCurrent to validate number! (%r)\n",
        Status
        ));

      FailFhrResume (FhrFailureUnexpectedBootOption, Status);
    } else if (BootCurrent != BootOptionNumber) {
      DEBUG ((
        DEBUG_ERROR,
        "[FHR DXE] BootCurrent does not match FhrResume option! Found 0x%x Expected 0x%x\n",
        BootCurrent,
        BootOptionNumber
        ));

      FailFhrResume (FhrFailureUnexpectedBootOption, EFI_NOT_STARTED);
    }
  } else {
    //
    // Capture the memory map at boot to evaluate in FHR resume.
    //
    DEBUG ((DEBUG_INFO, "[FHR DXE] Finalizing FHR firmware data block.\n"));

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
      DEBUG ((DEBUG_ERROR, "[FHR DXE] Failed to collect ReadyToBoot memory map! (%r)\n", Status));
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
  }

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
  Finds or creates the FhrResume boot option entry and sets it to BootNext.

  @retval   EFI_SUCCESS     Successfully prepared FhrResume launch.
  @retval   Other           Error returned by subroutine.
**/
EFI_STATUS
PrepareFhrResumeLaunch (
  VOID
  )
{
  EFI_STATUS                         Status;
  EFI_BOOT_MANAGER_LOAD_OPTION       BootOption;
  CHAR16                             *Description;
  UINTN                              DescriptionLength;
  EFI_DEVICE_PATH_PROTOCOL           *DevicePath;
  EFI_LOADED_IMAGE_PROTOCOL          *LoadedImage;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;
  CONST EFI_GUID                     *FileNameGuid = &gFhrResumeFileGuid;

  DevicePath  = NULL;
  Description = NULL;

  //
  // Build the FV load option for the FhrResume application.
  //

  ZeroMem (&BootOption, sizeof (BootOption));
  Status = GetSectionFromFv (
             FileNameGuid,
             EFI_SECTION_USER_INTERFACE,
             0,
             (VOID **)&Description,
             &DescriptionLength
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR DXE] Failed get get FV section! (%r)\n", Status));
    Description = NULL;
    goto Exit;
  }

  EfiInitializeFwVolDevicepathNode (&FileNode, FileNameGuid);
  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR DXE] Failed get LoadedImageProtocolHandle! (%r)\n", Status));
    goto Exit;
  }

  DevicePath = AppendDevicePathNode (
                 DevicePathFromHandle (LoadedImage->DeviceHandle),
                 (EFI_DEVICE_PATH_PROTOCOL *)&FileNode
                 );

  if (DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "[FHR DXE] Failed append path node!\n"));
    goto Exit;
  }

  Status = EfiBootManagerInitializeLoadOption (
             &BootOption,
             0x3FEC,   // TODO - Probably shouldn't use a well known value.
             LoadOptionTypeBoot,
             LOAD_OPTION_CATEGORY_APP | LOAD_OPTION_ACTIVE | LOAD_OPTION_HIDDEN,
             Description,
             DevicePath,
             NULL,
             0
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR DXE] Failed get initialize load option! (%r)\n", Status));
    goto Exit;
  }

  Status = EfiBootManagerLoadOptionToVariable (&BootOption);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR DXE] Failed to create FhrResume boot option variable! (%r)\n", Status));
    goto Exit;
  }

  ASSERT (BootOption.OptionNumber < MAX_UINT16);
  BootOptionNumber = (UINT16)BootOption.OptionNumber;

  //
  // Set BootNext to point to the FhrResume options number.
  //

  Status = gRT->SetVariable (
                  L"BootNext",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  sizeof (UINT16),
                  &BootOption.OptionNumber
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR DXE] Failed to set BootNext (%r).\n", Status));
    goto Exit;
  }

  DEBUG ((DEBUG_INFO, "[FHR DXE] FhrResume added as BootNext.\n"));

Exit:
  if (DevicePath != NULL) {
    FreePool (DevicePath);
  }

  if (Description != NULL) {
    FreePool (Description);
  }

  return Status;
}

/**

  Entry point for FHR DXE module. Prepares FHR data anf resume state.

  @param[in]  ImageHandle -- Handle to this image.
  @param[in]  SystemTable -- Pointer to the system table.

  @retval     EFI_STATUS
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
  // Check if PEI experienced a failure.
  //

  if (FhrHob->IsFhrBoot && (FhrHob->PeiFailureReason != FhrFailureNone)) {
    FailFhrResume (FhrHob->PeiFailureReason, FhrHob->PeiFailureStatus);
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

  if (mIsFhrResume) {
    //
    // If this is an FHR resume, then setup the BootNext target to FhrResume.
    //

    Status = PrepareFhrResumeLaunch ();
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "[FHR DXE] Failed to setup FhrResume launch. (%r)\n",
        Status
        ));

      return Status;
    }
  }

  //
  // Register for post ready to boot. This will be used to evaluate memory usage
  // and capture any final state. For an FHR boot this will ensure the entry
  // being launched is what we expect it to be and reboot if not.
  //

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

  return EFI_SUCCESS;
}
