/**
  A library for FHR helper functions.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/MemoryAttribute.h>

#include <Library/FhrLib.h>
#include <Fhr.h>

volatile BOOLEAN  FhrLoop = TRUE;

EFI_STATUS
FhrPrepareVectorExecution (
  IN EFI_PHYSICAL_ADDRESS  Vector
  )

{
  EFI_MEMORY_ATTRIBUTE_PROTOCOL  *MemoryAttribute;
  EFI_PHYSICAL_ADDRESS           BaseAddress;
  UINT64                         Length;
  EFI_STATUS                     Status;

  Status = gBS->LocateProtocol (&gEfiMemoryAttributeProtocolGuid, NULL, (VOID **)&MemoryAttribute);
  if (Status == EFI_NOT_FOUND) {
    return EFI_SUCCESS;
  } else if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // HACK, make sure the region is executable. This should be constrained later.
  //

  BaseAddress = (EFI_PHYSICAL_ADDRESS)ALIGN_POINTER (Vector, EFI_PAGE_SIZE) - (5 * EFI_PAGE_SIZE);
  Length      = EFI_PAGE_SIZE * 10;
  return MemoryAttribute->ClearMemoryAttributes (
                            MemoryAttribute,
                            BaseAddress,
                            Length,
                            EFI_MEMORY_RO | EFI_MEMORY_RP |  EFI_MEMORY_XP
                            );
}

BOOLEAN
DescriptorsOverlap (
  EFI_MEMORY_DESCRIPTOR  *Memory1,
  EFI_MEMORY_DESCRIPTOR  *Memory2
  )
{
  return ((Memory1->PhysicalStart < (Memory2->PhysicalStart + (Memory2->NumberOfPages << EFI_PAGE_SHIFT))) &&
          (Memory2->PhysicalStart < (Memory1->PhysicalStart + (Memory1->NumberOfPages << EFI_PAGE_SHIFT))));
}

EFI_STATUS
FhrValidateFinalMemoryMap (
  IN VOID   *StoredMemoryMap,
  IN UINTN  StoredMemoryMapSize,
  IN UINTN  StoredDescriptorSize,
  IN VOID   *FinalMemoryMap,
  IN UINTN  FinalMemoryMapSize,
  IN UINTN  FinalDescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *StoredEntry;
  EFI_MEMORY_DESCRIPTOR  *StoredMemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR  *FinalEntry;
  EFI_MEMORY_DESCRIPTOR  *FinalMemoryMapEnd;
  EFI_PHYSICAL_ADDRESS   StoredEntryEnd;
  EFI_PHYSICAL_ADDRESS   FinalEntryEnd;
  EFI_PHYSICAL_ADDRESS   StoredExpectedStart;
  EFI_PHYSICAL_ADDRESS   FinalExpectedStart;

  //
  // Iterate over the stored memory map, and ensure that
  //    1. No memory has disappeared.
  //    2. Memory that is OS owned is unclaimed.
  //    3. Runtime services regions have not moved.
  //

  StoredEntry         = (EFI_MEMORY_DESCRIPTOR *)StoredMemoryMap;
  FinalEntry          = (EFI_MEMORY_DESCRIPTOR *)FinalMemoryMap;
  StoredMemoryMapEnd  = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)StoredEntry + StoredMemoryMapSize);
  FinalMemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)FinalEntry + FinalMemoryMapSize);
  StoredExpectedStart = MAX_UINT64;
  FinalExpectedStart  = MAX_UINT64;

  while ((StoredEntry < StoredMemoryMapEnd) && (FinalEntry < FinalMemoryMapEnd)) {
    DEBUG ((DEBUG_INFO, "COMPARING:\n"));
    DEBUG ((DEBUG_INFO, "     STORED  0x%llx  0x%llx  %d\n", StoredEntry->PhysicalStart, StoredEntry->NumberOfPages, StoredEntry->Type));
    DEBUG ((DEBUG_INFO, "     FINAL   0x%llx  0x%llx  %d\n", FinalEntry->PhysicalStart, FinalEntry->NumberOfPages, FinalEntry->Type));

    //
    // Check that there is no unexpected gaps. This is done by tracking the
    // expected start of one of the descriptors. If neither are set then they
    // are both new and should align.
    //

    if (StoredExpectedStart != MAX_UINT64) {
      if (StoredEntry->PhysicalStart != StoredExpectedStart) {
        DEBUG ((
          DEBUG_WARN,
          "[FHR] New memory range found since cold boot at 0x%llx.\n",
          StoredExpectedStart
          ));
      }
    } else if (FinalExpectedStart != MAX_UINT64) {
      if (FinalEntry->PhysicalStart != FinalExpectedStart) {
        DEBUG ((
          DEBUG_ERROR,
          "[FHR] Memory region removed since cold boot at 0x%llx.\n",
          FinalExpectedStart
          ));

        return EFI_MEDIA_CHANGED;
      }
    } else {
      if (FinalEntry->PhysicalStart != StoredEntry->PhysicalStart) {
        if (FinalEntry->PhysicalStart > StoredEntry->PhysicalStart) {
          DEBUG ((
            DEBUG_WARN,
            "[FHR] New memory range found since cold boot at 0x%llx.\n",
            StoredEntry->PhysicalStart
            ));
        } else {
          DEBUG ((
            DEBUG_ERROR,
            "[FHR] Memory region removed since cold boot at 0x%llx.\n",
            FinalEntry->PhysicalStart
            ));

          return EFI_MEDIA_CHANGED;
        }
      }
    }

    if (DescriptorsOverlap (StoredEntry, FinalEntry)) {
      //
      // Check that no memory is described as runtime that wasn't previously.
      //

      if (!FHR_IS_RUNTIME_MEMORY (StoredEntry->Type) && FHR_IS_RUNTIME_MEMORY (FinalEntry->Type)) {
        DEBUG ((
          DEBUG_ERROR,
          "[FHR] Memory type changed to runtime type! "
          "Original: Base 0x%llx Pages 0x%llx Type %d. "
          "Current: Base 0x%llx Pages 0x%llx Type %d.\n",
          StoredEntry->PhysicalStart,
          StoredEntry->NumberOfPages,
          StoredEntry->Type,
          FinalEntry->PhysicalStart,
          FinalEntry->NumberOfPages,
          FinalEntry->Type
          ));

        return EFI_MEDIA_CHANGED;
      }

      //
      // Check that the runtime types did not change for everything but
      // reserved. This could be loosened to allow dropping runtime ranges,
      // but at the time of writing it seemed best to be strict to avoid
      // unexpected runtime behavior.
      //

      if (FHR_IS_RUNTIME_MEMORY (StoredEntry->Type) &&
          (StoredEntry->Type != EfiReservedMemoryType) &&
          (StoredEntry->Type != FinalEntry->Type))
      {
        DEBUG ((
          DEBUG_ERROR,
          "[FHR] Unexpected change in runtime region! "
          "Original: Base 0x%llx Pages 0x%llx Type %d. "
          "Current: Base 0x%llx Pages 0x%llx Type %d.\n",
          StoredEntry->PhysicalStart,
          StoredEntry->NumberOfPages,
          StoredEntry->Type,
          FinalEntry->PhysicalStart,
          FinalEntry->NumberOfPages,
          FinalEntry->Type
          ));

        return EFI_MEDIA_CHANGED;
      }
    }

    //
    // Progress the lower of the two descriptors. Keep a pointer to the end
    // to check for new or removed memory.
    //
    StoredEntryEnd = StoredEntry->PhysicalStart +
                     (StoredEntry->NumberOfPages << EFI_PAGE_SHIFT);
    FinalEntryEnd = FinalEntry->PhysicalStart +
                    (FinalEntry->NumberOfPages << EFI_PAGE_SHIFT);

    StoredExpectedStart = MAX_UINT64;
    FinalExpectedStart  = MAX_UINT64;
    if (FinalEntryEnd < StoredEntryEnd) {
      FinalEntry         = NEXT_MEMORY_DESCRIPTOR (FinalEntry, FinalDescriptorSize);
      FinalExpectedStart = FinalEntryEnd;
    } else if (StoredEntryEnd < FinalEntryEnd) {
      StoredEntry         = NEXT_MEMORY_DESCRIPTOR (StoredEntry, FinalDescriptorSize);
      StoredExpectedStart = StoredEntryEnd;
    } else {
      FinalEntry  = NEXT_MEMORY_DESCRIPTOR (FinalEntry, FinalDescriptorSize);
      StoredEntry = NEXT_MEMORY_DESCRIPTOR (StoredEntry, FinalDescriptorSize);
    }
  }

  return EFI_SUCCESS;
}

VOID
FhrBmResume (
  IN FHR_HOB  *FhrHob
  )
{
  EFI_STATUS       Status;
  VOID             *MemoryMap;
  UINTN            CurrentSize;
  UINTN            MemoryMapSize;
  UINTN            DescriptorSize;
  UINT32           DescriptorVersion;
  UINTN            MapKey;
  FHR_FW_DATA      *FhrData;
  OS_RESET_VECTOR  ResumeVector;

  MemoryMap      = NULL;
  MemoryMapSize  = 0;
  DescriptorSize = 0;
  CurrentSize    = 0;
  FhrData        = (FHR_FW_DATA *)FhrHob->FhrReservedBase;

  Status = FhrValidateFwData (FhrData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR] Failed to validate FW data! (%r) \n", Status));
    goto Exit;
  }

  //
  // We need to make sure the resume vector is executable.
  //

  Status = FhrPrepareVectorExecution (FhrHob->ResetVector);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR] Failed to prepare reset vector for execution! (%r) \n", Status));
    goto Exit;
  }

  do {
    if (MemoryMapSize > 0) {
      MemoryMap   = ReallocatePool (CurrentSize, MemoryMapSize, MemoryMap);
      CurrentSize = MemoryMapSize;
      if (MemoryMap == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }
    }

    Status = gBS->GetMemoryMap (
                    &MemoryMapSize,
                    MemoryMap,
                    &MapKey,
                    &DescriptorSize,
                    &DescriptorVersion
                    );
  } while ((Status == EFI_BUFFER_TOO_SMALL) && (MemoryMapSize > CurrentSize));

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR] Failed to get memory map! (%r) \n", Status));
    goto Exit;
  }

  //
  // exit boot services in preparation for doing FHR.
  //
  DEBUG ((DEBUG_INFO, "[FHR] Exiting boot services.\n"));

  Status = gBS->ExitBootServices (gImageHandle, MapKey);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR] Failed ExitBootServices! (%r) \n", Status));
    goto Exit;
  }

  //
  // Compare the final memory map with the saved memory map to make sure there
  // are no unexpected memory type changes.
  //

  Status = FhrValidateFinalMemoryMap (
             (UINT8 *)FhrData + FhrData->MemoryMapOffset,
             (UINTN)FhrData->MemoryMapSize,
             (UINTN)FhrData->MemoryMapDescriptorSize,
             MemoryMap,
             MemoryMapSize,
             DescriptorSize
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR] Failed to validate memory maps! (%r) \n", Status));
    goto Exit;
  }

  ResumeVector = (OS_RESET_VECTOR)FhrHob->ResetVector;
  DEBUG ((DEBUG_INFO, "[FHR] Resuming to OS vector.\n"));
  ResumeVector (gST, (VOID *)FhrHob->ResetData, FhrHob->ResetDataSize);

  //
  // This should never be reached.
  //

Exit:
  // TODO - diagnostics.

  DEBUG ((DEBUG_ERROR, "[FHR] FHR resume failed! (%r) \n", Status));

  // TODO: remove
  while (FhrLoop) {
  }

  gRT->ResetSystem (EfiResetWarm, Status, 0, NULL);
  CpuDeadLoop ();
}

EFI_STATUS
EFIAPI
FhrBootManager (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  FHR_HOB            *FhrHob;

  //
  // Check if this is an FHR resume.
  //

  GuidHob = GetFirstGuidHob (&gFhrHobGuid);
  if (GuidHob == NULL) {
    DEBUG ((DEBUG_INFO, "[FHR] FHR HOB not found, skipping FHR boot manager.\n"));
    return EFI_SUCCESS;
  }

  FhrHob = (FHR_HOB *)GET_GUID_HOB_DATA (GuidHob);
  if (FhrHob->IsFhrBoot) {
    FhrBmResume (FhrHob);

    //
    // This should never return and it is not safe to continue.
    //

    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  return EFI_SUCCESS;
}
