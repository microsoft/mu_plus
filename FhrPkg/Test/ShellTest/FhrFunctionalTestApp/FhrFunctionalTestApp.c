/** @file
UEFI Shell based application for unit testing the Firmware Hot Restart feature.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
***/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/ShellParameters.h>
#include <Protocol/Shell.h>

#include <Fhr.h>

//
// Structures used for the test.
//

#pragma pack(1)

typedef struct _FHR_RESET_PARAMETERS {
  UINT16            FriendlyString;
  EFI_GUID          ResetTypeGuid;
  UINT16            Alignment[3];
  FHR_RESET_DATA    FhrResetData;
} FHR_RESET_PARAMETERS;

#pragma pack()

//
// Test constants.
//

#define RESET_STRING    L"FHR TEST"
#define MEMORY_PATTERN  (0x5A5A5A5A5A5A5A5Allu)
#define SCRATCH_PAGES   (10)
#define SCRATCH_SIZE    (SCRATCH_PAGES * EFI_PAGE_SIZE)
#define EXIT_RETRIES    (3)

//
// Test globals. These should persist across the FHR.
//

VOID                   *Scratch;
UINT32                 ScratchCrc;
EFI_MEMORY_DESCRIPTOR  *MemoryMap;
UINTN                  DescriptorSize;
UINTN                  MemoryMapSize;
UINT32                 RebootCount;
CONST EFI_GUID         ResetTypeGuid = FHR_RESET_TYPE_GUID;

//
// Test configuration globals.
//

BOOLEAN  TestSkipMemory;
BOOLEAN  TestPatternFullPage;
UINTN    TestRebootCount = 3;

//
// Function prototypes.
//

VOID
EFIAPI
InitiateFhr (
  EFI_RUNTIME_SERVICES  *RuntimeServices
  );

/**
  Checks if a given memory type should be treated as OS reclaimable for memory
  patterning.

  @param[in]    MemoryType      The memory type to check.

  @retval   TRUE    The memory should be treated as OS reclaimable.
  @retval   TRUE    The memory should not be treated as OS reclaimable.
**/
BOOLEAN
IsOsUsableMemory (
  EFI_MEMORY_TYPE  MemoryType
  )
{
  switch (MemoryType) {
    // case EfiBootServicesCode: // TEMP, till paging attributes fixed
    case EfiConventionalMemory:
    case EfiACPIReclaimMemory:
    case EfiPersistentMemory:
      return TRUE;

    // We must leave data pages alone or else
    // we will stomp on our page tables.
    case EfiBootServicesData:

    // Exclude EfiLoader to make sure not to break ourselves.
    case EfiLoaderCode:
    case EfiLoaderData:
    default:
      return FALSE;
  }
}

/**
  Scans through the memory map and either applies a memory pattern to validates
  the memory pattern still exists.

  @param[in]    Verify      If FALSE a memory pattern will be applied. If TRUE
                            the memory pattern will be validated.

  @retval   EFI_SUCCESS             The memory was successfully patterned or checked.
  @retval   EFI_INVALID_PARAMETER   The memory map is not set.
  @retval   EFI_VOLUME_CORRUPTED    The memory pattern check failed.
**/
EFI_STATUS
EFIAPI
CheckMemory (
  BOOLEAN  Verify
  )

{
  EFI_MEMORY_DESCRIPTOR  *Entry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;
  EFI_STATUS             Status;
  UINTN                  Page;
  UINTN                  PageErrors;
  UINT64                 *Blocks;
  UINT32                 BlockIndex;
  UINT64                 Pattern;

  //
  // Check if skipping memory was requested.
  //

  if (TestSkipMemory) {
    DEBUG ((DEBUG_INFO, "Skipping memory check.\n"));
    return EFI_SUCCESS;
  }

  //
  // For all memory that is OS usable, pattern it. Make sure not to pattern this
  // application or it's data.
  //

  if (MemoryMap == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;
  if (Verify) {
    DEBUG ((DEBUG_INFO, "VERIFYING MEMORY PATTERN:\n"));
  } else {
    DEBUG ((DEBUG_INFO, "APPLYING MEMORY PATTERN:\n"));
  }

  Entry        = MemoryMap;
  MemoryMapEnd = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + MemoryMapSize);
  while (Entry < MemoryMapEnd) {
    if (!IsOsUsableMemory (Entry->Type)) {
      Entry = NEXT_MEMORY_DESCRIPTOR (Entry, DescriptorSize);
      continue;
    }

    DEBUG ((
      DEBUG_INFO,
      "    Base: %016llx  Pages: %x  Type:  %d\n",
      Entry->PhysicalStart,
      Entry->NumberOfPages,
      Entry->Type
      ));

    PageErrors = 0;
    for (Page = 0; Page < Entry->NumberOfPages; Page++) {
      Pattern = MEMORY_PATTERN ^ ((Entry->PhysicalStart >> EFI_PAGE_SHIFT) + Page);
      Blocks  = (UINT64 *)(Entry->PhysicalStart + (Page << EFI_PAGE_SHIFT));

      // skip the 0 page to avoid faulting on memory protections.
      if (Blocks == NULL) {
        continue;
      }

      //
      // Pattern the memory in 64 bit chunks.
      //

      for (BlockIndex = 0; BlockIndex < (EFI_PAGE_SIZE / sizeof (Blocks[0])); BlockIndex += 1) {
        if (Verify) {
          if (Blocks[BlockIndex] != Pattern) {
            DEBUG ((DEBUG_ERROR, "    MEMORY FAILURE: 0x%x\n", &Blocks[BlockIndex]));
            Status = EFI_VOLUME_CORRUPTED;
            break;
          }
        } else {
          Blocks[BlockIndex] = Pattern;
        }

        //
        // As an optimization, check only the first block of each page.
        //

        if (!TestPatternFullPage) {
          break;
        }
      }
    }

    Entry = NEXT_MEMORY_DESCRIPTOR (Entry, DescriptorSize);
  }

  DEBUG ((DEBUG_INFO, "DONE\n"));
  return Status;
}

/**
  Runs through the cold boot memory map to check for incompatible configurations.

  @retval   EFI_SUCCESS     Memory map successfully validated.
**/
EFI_STATUS
EFIAPI
CheckMemoryMap (
  VOID
  )

{
  EFI_MEMORY_DESCRIPTOR  *Entry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;
  EFI_STATUS             Status;

  if (MemoryMap == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "[FHR TEST] Validating memory map types.\n"));
  DEBUG ((DEBUG_INFO, "[FHR TEST]     Start             Pages             MemoryType\n"));
  DEBUG ((DEBUG_INFO, "[FHR TEST]     -----------------------------------------------------\n"));

  Status       = EFI_SUCCESS;
  Entry        = MemoryMap;
  MemoryMapEnd = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + MemoryMapSize);
  while (Entry < MemoryMapEnd) {
    DEBUG ((DEBUG_INFO, "[FHR TEST]     %016llx  %016llx  %016llx \n", Entry->PhysicalStart, Entry->NumberOfPages, Entry->Type));
    Entry = NEXT_MEMORY_DESCRIPTOR (Entry, DescriptorSize);
  }

  return Status;
}

/**
  The entry point for an FHR resume. Checks that memory is intact and initiates
  another FHR if more are left in the test.

  @param[in]  Handle            Unused for FHR.
  @param[in]  SystemTable       Pointer to the updated system table.
  @param[in]  ResetData         The FHR resume data structure.
**/
VOID
EFIAPI
FhrTestPostReboot (
  IN EFI_HANDLE        Handle,
  IN EFI_SYSTEM_TABLE  *SystemTable,
  IN FHR_RESUME_DATA   *ResumeData
  )

{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "[FHR TEST] Starting post-FHR code.\n"));

  if (RebootCount == 0) {
    DEBUG ((DEBUG_ERROR, "[FHR TEST] Unexpected zero reboot count!\n"));
    CpuDeadLoop ();
  }

  if (ResumeData->Signature != FHR_RESUME_DATA_SIGNATURE) {
    DEBUG ((
      DEBUG_ERROR,
      "[FHR TEST] Resume signature is incorrect! Expected: 0x%x Actual: 0x%x\n",
      FHR_RESUME_DATA_SIGNATURE,
      ResumeData->Signature
      ));
    CpuDeadLoop ();
  }

  if (ResumeData->OsDataBase != (EFI_PHYSICAL_ADDRESS)Scratch) {
    DEBUG ((
      DEBUG_ERROR,
      "[FHR TEST] OsDataBase pointer is incorrect! Expected: 0x%x Actual: 0x%x\n",
      (EFI_PHYSICAL_ADDRESS)Scratch,
      ResumeData->OsDataBase
      ));
    CpuDeadLoop ();
  }

  if (ResumeData->OsDataSize != SCRATCH_SIZE) {
    DEBUG ((
      DEBUG_ERROR,
      "[FHR TEST] OsDataSize is incorrect! Expected: 0x%x Actual: 0x%x\n",
      SCRATCH_SIZE,
      ResumeData->OsDataSize
      ));
    CpuDeadLoop ();
  }

  if (ScratchCrc != CalculateCrc32 (Scratch, SCRATCH_SIZE)) {
    DEBUG ((DEBUG_ERROR, "[FHR TEST] Scratch memory CRC does not match!\n"));
    CpuDeadLoop ();
  }

  Status = CheckMemory (TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR TEST] Failed to verify memory! (%r)\n", Status));
    CpuDeadLoop ();
  }

  DEBUG ((DEBUG_INFO, "[FHR TEST] Reboot successful! (%d/%d)\n", RebootCount, TestRebootCount));
  if (RebootCount < TestRebootCount) {
    InitiateFhr (SystemTable->RuntimeServices);
    DEBUG ((DEBUG_ERROR, "[FHR TEST] Unexpected return from InitiateFhr.\n"));
    CpuDeadLoop ();
  }

  DEBUG ((DEBUG_INFO, "[FHR TEST] Success!\n"));
  CpuDeadLoop ();
}

/**
  Initiates a firmware hot restart. This function does not return.

  @param[in]    RuntimeServices   The current runtime services tables.
**/
VOID
EFIAPI
InitiateFhr (
  EFI_RUNTIME_SERVICES  *RuntimeServices
  )

{
  FHR_RESET_PARAMETERS  ResetParams;

  //
  // Store the CRC of the scratch, makes sure we dont hit unexpected errors.
  //

  ScratchCrc = CalculateCrc32 (Scratch, SCRATCH_SIZE);

  //
  // Initiate FHR;
  //

  RebootCount++;
  ZeroMem (&ResetParams, sizeof (ResetParams));
  ResetParams.FriendlyString          = '\0';
  ResetParams.ResetTypeGuid           = ResetTypeGuid;
  ResetParams.FhrResetData.Signature  = FHR_RESET_DATA_SIGNATURE;
  ResetParams.FhrResetData.Length     = sizeof (ResetParams.FhrResetData);
  ResetParams.FhrResetData.OsEntry    = (EFI_PHYSICAL_ADDRESS)FhrTestPostReboot;
  ResetParams.FhrResetData.OsDataBase = (EFI_PHYSICAL_ADDRESS)Scratch;
  ResetParams.FhrResetData.OsDataSize = SCRATCH_SIZE;

  ResetParams.FhrResetData.Checksum = CalculateCheckSum8 ((UINT8 *)(&ResetParams.FhrResetData), sizeof (ResetParams.FhrResetData));

  DEBUG ((
    DEBUG_INFO,
    "[FHR TEST] ResumeVector: %p ResetData: %p DataSize: 0x%x\n",
    ResetParams.FhrResetData.OsEntry,
    ResetParams.FhrResetData.OsDataBase,
    ResetParams.FhrResetData.OsDataSize
    ));

  DEBUG ((DEBUG_INFO, "[FHR TEST] Initiating FHR! (%d/%d)\n", RebootCount, TestRebootCount));
  RuntimeServices->ResetSystem (
                     EfiResetPlatformSpecific,
                     EFI_SUCCESS,
                     sizeof (ResetParams),
                     &ResetParams
                     );

  DEBUG ((DEBUG_ERROR, "[FHR TEST] Unexpected return from ResetSystem!\n"));
  CpuDeadLoop ();
}

/**
  Prepares the test for the first FHR byt initializing reset data, getting and
  validating the memory map, and calling ExitBootServices. After these steps it
  will call to initiate the FHR.

  Does not return on success.
  @retval   ANY     Error returned by subroutine.
**/
EFI_STATUS
EFIAPI
FhrTestPreReboot (
  VOID
  )

{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Memory;
  UINTN                 MapKey;
  UINT32                DescriptorVersion;
  UINT32                Retries;

  //
  // Initialize the persisted memory block. This serves the dual purpose of
  // providing space for the memory map and other data as well as being used as
  // the persisted data.
  //
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiLoaderData,
                  SCRATCH_PAGES,
                  &Memory
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR TEST] Failed to allocate scratch! (%r) \n", Status));
    return Status;
  }

  Scratch = (VOID *)Memory;
  Retries = 0;

  //
  // Get the final memory map.
  //

  Print (L"Exiting boot services.\n\r");
  do {
    ZeroMem (Scratch, SCRATCH_SIZE);
    MemoryMap     = Scratch;
    MemoryMapSize = SCRATCH_SIZE;
    DEBUG ((
      DEBUG_INFO,
      "[FHR TEST] Getting final memory map. (%d/%d)\n",
      Retries + 1,
      EXIT_RETRIES
      ));

    Status = gBS->GetMemoryMap (
                    &MemoryMapSize,
                    MemoryMap,
                    &MapKey,
                    &DescriptorSize,
                    &DescriptorVersion
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[FHR TEST] Failed to get memory map! (%r) \n", Status));
      return Status;
    }

    ASSERT (DescriptorVersion == EFI_MEMORY_DESCRIPTOR_VERSION);

    //
    // Check memory types.
    //
    Status = CheckMemoryMap ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[FHR TEST] Failed memory types check! (%r) \n", Status));
      return Status;
    }

    //
    // exit boot services in preparation for doing FHR.
    //
    DEBUG ((DEBUG_INFO, "[FHR TEST] Exiting boot services.\n"));
    Status = gBS->ExitBootServices (gImageHandle, MapKey);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[FHR TEST] Failed ExitBootServices! (%r) \n", Status));
    }
  } while (EFI_ERROR (Status) && Retries++ < EXIT_RETRIES);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Off into the unknown! No more returns!
  //

  DEBUG ((DEBUG_INFO, "[FHR TEST] Running post boot services steps!\n"));

  Status = CheckMemory (FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR TEST] Failed to pattern memory! (%r) \n", Status));
    CpuDeadLoop ();
  }

  // Self-check
  Status = CheckMemory (TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[FHR TEST] Failed to verify memory! (%r) \n", Status));
    CpuDeadLoop ();
  }

  //
  // Initiate the FHR.
  //

  InitiateFhr (gRT);

  //
  // It is not safe to return, spin.
  //
  DEBUG ((DEBUG_ERROR, "[FHR TEST] Unexpected end of FhrTestPreReboot.\n"));
  CpuDeadLoop ();
  return EFI_SUCCESS;
}

/**
  The applications entry point.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.
**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                     Status;
  EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters;
  UINTN                          Argc;
  CHAR16                         **Argv;
  UINTN                          Index;

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiShellParametersProtocolGuid,
                  (VOID **)&ShellParameters
                  );

  if (EFI_ERROR (Status)) {
    Print (L"Failed to get parameters protocol! (%r)\n\r", Status);
  } else {
    Argc = ShellParameters->Argc;
    Argv = ShellParameters->Argv;
    for (Index = 1; Index < Argc; Index++) {
      if ((StrCmp (Argv[Index], L"-nomemory") == 0)) {
        TestSkipMemory = TRUE;
      } else if ((StrCmp (Argv[Index], L"-fullpage") == 0)) {
        TestPatternFullPage = TRUE;
      } else if ((StrCmp (Argv[Index], L"-reboots") == 0)) {
        TestRebootCount = 0;
        if (Index + 1 < Argc) {
          TestRebootCount = StrDecimalToUintn (Argv[Index + 1]);
          Index++;
        }

        if ((TestRebootCount == 0) || (TestRebootCount == MAX_UINTN)) {
          Print (L"Invalid reboot count.\n\r");
          return EFI_INVALID_PARAMETER;
        }
      } else {
        Print (L"Unrecognized parameter '%ls'.\n\r", Argv[Index]);
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  Status = FhrTestPreReboot ();
  Print (L"Test failed. %r \n\r", Status);
  Print (L"See logs for more details.\n\r");
  return Status;
}
