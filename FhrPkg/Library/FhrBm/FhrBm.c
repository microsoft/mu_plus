/**
  The FHR boot manager library.

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

/**
  Prepares the OS resume vector for execution. This includes ensuring that the
  resume page is executable.

  @param[in]  Vector      The physical address of the OS resume vector.
  @param[in]  VectorSize  The size in bytes of the OS resume vector.

  @retval   EFI_SUCCESS   The resume vector was successfully prepared for execution.
  @retval   Other         Error returned by subroutine.
**/
EFI_STATUS
FhrPrepareVectorExecution (
  IN EFI_PHYSICAL_ADDRESS  Vector,
  IN UINT64                VectorSize
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
  // TODO: reduce to only one page once test app is made more robust.
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

/**
  Checks if two descriptors overlap.

  @param[in]  Memory1   The first memory descriptor.
  @param[in]  Memory2   The second memory descriptor.

  @retval   TRUE    The descriptors overlap.
  @retval   FALSE   The descriptors do not overlap.
**/
BOOLEAN
DescriptorsOverlap (
  EFI_MEMORY_DESCRIPTOR  *Memory1,
  EFI_MEMORY_DESCRIPTOR  *Memory2
  )
{
  EFI_PHYSICAL_ADDRESS  Memory1End;
  EFI_PHYSICAL_ADDRESS  Memory2End;

  Memory1End = Memory1->PhysicalStart + EFI_PAGES_TO_SIZE (Memory1->NumberOfPages);
  Memory2End = Memory2->PhysicalStart + EFI_PAGES_TO_SIZE (Memory2->NumberOfPages);

  ASSERT (Memory1End > Memory1->PhysicalStart);
  ASSERT (Memory2End > Memory2->PhysicalStart);

  return ((Memory1->PhysicalStart < Memory2End) &&
          (Memory2->PhysicalStart < Memory1End));
}

/**
  Compares the memory map from the cold boot to the current final memory map
  checking that the current memory map is compatible for a FHR resume.

  @param[in]  StoredMemoryMap         The associated cold boot memory map.
  @param[in]  StoredMemoryMapSize     Size of the cold boot memory map.
  @param[in]  StoredDescriptorSize    Size of the cold boot memory descriptor.
  @param[in]  FinalMemoryMap          The current final memory map.
  @param[in]  FinalMemoryMapSize      The size of the final memory map.
  @param[in]  FinalDescriptorSize     The size of the current memory descriptor.

  @retval   EFI_SUCCESS         The memory maps are compatible.
  @retval   EFI_MEDIA_CHANGED   The memory maps are not compatible.
**/
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
    DEBUG ((DEBUG_VERBOSE, "COMPARING:\n"));

    DEBUG ((
      DEBUG_VERBOSE,
      "     STORED  0x%llx  0x%llx  %d\n",
      StoredEntry->PhysicalStart,
      StoredEntry->NumberOfPages,
      StoredEntry->Type
      ));

    DEBUG ((
      DEBUG_VERBOSE,
      "     FINAL   0x%llx  0x%llx  %d\n",
      FinalEntry->PhysicalStart,
      FinalEntry->NumberOfPages,
      FinalEntry->Type
      ));

    //
    // Check that there is no unexpected gaps. This is done by tracking the
    // expected start of one of the descriptors. If neither are set then they
    // are both new and should align.
    //

    if (StoredExpectedStart != MAX_UINT64) {
      ASSERT (FinalExpectedStart == MAX_UINT64);
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

/**
  Resumes the system from an FHR. This function will exit boot services and
  transition to the OS resume vector.

  @param[in]  FhrHob        The hob providing FHR information.
**/
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
  FHR_RESUME_DATA  ResumeData;

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

  Status = FhrPrepareVectorExecution (
             FhrHob->ResetData.ResumeCodeBase,
             FhrHob->ResetData.ResumeCodeSize
             );

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

  //
  // Build the OS resume data.
  //

  ZeroMem (&ResumeData, sizeof (ResumeData));
  ResumeData.Signature      = FHR_RESUME_DATA_SIGNATURE;
  ResumeData.Length         = sizeof (ResumeData);
  ResumeData.Revision       = FHR_RESUME_DATA_REVISION;
  ResumeData.ResumeCodeBase = FhrHob->ResetData.ResumeCodeBase;
  ResumeData.ResumeCodeSize = FhrHob->ResetData.ResumeCodeSize;
  ResumeData.OsDataBase     = FhrHob->ResetData.OsDataBase;
  ResumeData.OsDataSize     = FhrHob->ResetData.OsDataSize;
  ResumeData.Flags          = FHR_MEMORY_PRESERVED;
  ResumeData.Checksum       = CalculateCheckSum8 ((UINT8 *)(&ResumeData), sizeof (ResumeData));

  //
  // Log some useful information.
  //

  DEBUG ((DEBUG_INFO, "[FHR] ResumeData =                 0x%llx\n", (EFI_PHYSICAL_ADDRESS)&ResumeData));
  DEBUG ((DEBUG_INFO, "[FHR] ResumeData.Length =          0x%x\n", ResumeData.Length));
  DEBUG ((DEBUG_INFO, "[FHR] ResumeData.Revision =        0x%x\n", ResumeData.Revision));
  DEBUG ((DEBUG_INFO, "[FHR] ResumeData.ResumeCodeBase =  0x%llx\n", ResumeData.ResumeCodeBase));
  DEBUG ((DEBUG_INFO, "[FHR] ResumeData.ResumeCodeSize =  0x%llx\n", ResumeData.ResumeCodeSize));
  DEBUG ((DEBUG_INFO, "[FHR] ResumeData.OsDataBase =      0x%llx\n", ResumeData.OsDataBase));
  DEBUG ((DEBUG_INFO, "[FHR] ResumeData.OsDataSize =      0x%llx\n", ResumeData.OsDataSize));
  DEBUG ((DEBUG_INFO, "[FHR] ResumeData.Flags =           0x%llx\n", ResumeData.Flags));

  //
  // Resume to the OS.
  //

  ResumeVector = (OS_RESET_VECTOR)FhrHob->ResetData.ResumeCodeBase;
  DEBUG ((DEBUG_INFO, "[FHR] Resuming to OS vector.\n"));
  ResumeVector (0, gST, &ResumeData);

  //
  // This should never be reached.
  //

Exit:
  // TODO - diagnostics.

  DEBUG ((DEBUG_ERROR, "[FHR] FHR resume failed! (%r) \n", Status));
  gRT->ResetSystem (EfiResetWarm, Status, 0, NULL);
  CpuDeadLoop ();
}

/**
  Handles the FHR resume process. This routine will not return if this is an
  FHR resume.

  @retval   EFI_SUCCESS     Not an FHR resume, no work needed.
**/
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

  DEBUG ((DEBUG_INFO, "[FHR] Not an FHR boot, exiting.\n"));
  return EFI_SUCCESS;
}
