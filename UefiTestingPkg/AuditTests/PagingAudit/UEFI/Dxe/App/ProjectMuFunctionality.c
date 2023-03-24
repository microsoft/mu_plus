#include "DxePagingAuditTestApp.h"
#include <Protocol/MemoryProtectionSpecialRegionProtocol.h>
#include <Protocol/MemoryProtectionDebug.h>
#include <Protocol/CpuMpDebug.h>
#include <Protocol/MemoryProtectionDebug.h>
#include <Library/DxeMemoryProtectionHobLib.h>
#include <Library/HobLib.h>

MEMORY_PROTECTION_SPECIAL_REGION  *mSpecialRegions           = NULL;
IMAGE_RANGE_DESCRIPTOR            *mNonProtectedImageList    = NULL;
UINTN                             mSpecialRegionCount        = 0;
MEMORY_PROTECTION_DEBUG_PROTOCOL  *mMemoryProtectionProtocol = NULL;
CPU_MP_DEBUG_PROTOCOL             *mCpuMpDebugProtocol       = NULL;

/**
  Populates the heap guard protocol global

  @retval EFI_SUCCESS Protocol is already populated or was successfully populated
  @retval other       Return value of LocateProtocol
**/
EFI_STATUS
PopulateHeapGuardDebugProtocol (
  VOID
  )
{
  if (mMemoryProtectionProtocol != NULL) {
    return EFI_SUCCESS;
  }

  return gBS->LocateProtocol (&gMemoryProtectionDebugProtocolGuid, NULL, (VOID **)&mMemoryProtectionProtocol);
}

/**
  Populates the CPU MP debug protocol global

  @retval EFI_SUCCESS Protocol is already populated or was successfully populated
  @retval other       Return value of LocateProtocol
**/
EFI_STATUS
PopulateCpuMpDebugProtocol (
  VOID
  )
{
  if (mCpuMpDebugProtocol != NULL) {
    return EFI_SUCCESS;
  }

  return gBS->LocateProtocol (&gCpuMpDebugProtocolGuid, NULL, (VOID **)&mCpuMpDebugProtocol);
}

/*
  Writes the NULL page and stack information to the memory info database
 */
VOID
ProjectMuSpecialMemoryDump (
  VOID
  )
{
  CHAR8                      TempString[MAX_STRING_SIZE];
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryHob;
  EFI_STATUS                 Status;
  LIST_ENTRY                 *List;
  CPU_MP_DEBUG_PROTOCOL      *Entry;
  EFI_PHYSICAL_ADDRESS       StackBase;
  UINT64                     StackLength;

  // Capture the NULL address
  AsciiSPrint (
    TempString,
    MAX_STRING_SIZE,
    "Null,0x%016lx\n",
    NULL
    );
  AppendToMemoryInfoDatabase (TempString);

  Hob.Raw = GetHobList ();

  while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw)) != NULL) {
    MemoryHob = Hob.MemoryAllocation;
    if (CompareGuid (&gEfiHobMemoryAllocStackGuid, &MemoryHob->AllocDescriptor.Name)) {
      StackBase   = (EFI_PHYSICAL_ADDRESS)((MemoryHob->AllocDescriptor.MemoryBaseAddress / EFI_PAGE_SIZE) * EFI_PAGE_SIZE);
      StackLength = (EFI_PHYSICAL_ADDRESS)(EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (MemoryHob->AllocDescriptor.MemoryLength)));

      // Capture the stack guard
      if (gDxeMps.CpuStackGuard == TRUE) {
        AsciiSPrint (
          TempString,
          MAX_STRING_SIZE,
          "StackGuard,0x%016lx,0x%x\n",
          StackBase,
          EFI_PAGE_SIZE
          );
        AppendToMemoryInfoDatabase (TempString);
        StackBase   += EFI_PAGE_SIZE;
        StackLength -= EFI_PAGE_SIZE;
      }

      // Capture the stack
      AsciiSPrint (
        TempString,
        MAX_STRING_SIZE,
        "Stack,0x%016lx,0x%016lx\n",
        StackBase,
        StackLength
        );
      AppendToMemoryInfoDatabase (TempString);

      break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  Status = PopulateCpuMpDebugProtocol ();

  // The protocol should only be published if CpuStackGuard is active
  if (!EFI_ERROR (Status)) {
    for (List = mCpuMpDebugProtocol->Link.ForwardLink; List != &mCpuMpDebugProtocol->Link; List = List->ForwardLink) {
      Entry = CR (
                List,
                CPU_MP_DEBUG_PROTOCOL,
                Link,
                CPU_MP_DEBUG_SIGNATURE
                );
      StackBase   = (EFI_PHYSICAL_ADDRESS)((Entry->ApStackBuffer / EFI_PAGE_SIZE) * EFI_PAGE_SIZE);
      StackLength = (EFI_PHYSICAL_ADDRESS)(EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (Entry->ApStackSize)));

      if (!Entry->IsSwitchStack) {
        if (gDxeMps.CpuStackGuard == TRUE) {
          // Capture the AP stack guard
          AsciiSPrint (
            TempString,
            MAX_STRING_SIZE,
            "ApStackGuard,0x%016lx,0x%016lx,0x%x\n",
            StackBase,
            EFI_PAGE_SIZE,
            Entry->CpuNumber
            );
          AppendToMemoryInfoDatabase (TempString);

          StackBase   += EFI_PAGE_SIZE;
          StackLength -= EFI_PAGE_SIZE;
        }

        // Capture the AP stack
        AsciiSPrint (
          TempString,
          MAX_STRING_SIZE,
          "ApStack,0x%016lx,0x%016lx,0x%x\n",
          StackBase,
          StackLength,
          Entry->CpuNumber
          );
        AppendToMemoryInfoDatabase (TempString);
      } else {
        // Capture the AP switch stack
        AsciiSPrint (
          TempString,
          MAX_STRING_SIZE,
          "ApSwitchStack,0x%016lx,0x%016lx,0x%x\n",
          StackBase,
          StackLength,
          Entry->CpuNumber
          );
        AppendToMemoryInfoDatabase (TempString);
      }
    }
  }
}

/**
  Populates the non protected image list global

  @retval EFI_SUCCESS            The non-protected image list was populated
  @retval EFI_INVALID_PARAMETER  The non-protected image list was already populated
  @retval other                  The non-protected image list was not populated
**/
EFI_STATUS
GetNonProtectedImageList (
  VOID
  )
{
  EFI_STATUS                        Status;
  MEMORY_PROTECTION_DEBUG_PROTOCOL  *MemoryProtectionProtocol;

  MemoryProtectionProtocol = NULL;

  if (mNonProtectedImageList != NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->LocateProtocol (
                  &gMemoryProtectionDebugProtocolGuid,
                  NULL,
                  (VOID **)&MemoryProtectionProtocol
                  );

  if (!EFI_ERROR (Status)) {
    Status = MemoryProtectionProtocol->GetImageList (
                                         &mNonProtectedImageList,
                                         NonProtected
                                         );
  }

  return Status;
}

/**
  Populates the special region array global
**/
EFI_STATUS
GetSpecialRegions (
  VOID
  )
{
  EFI_STATUS                                 Status;
  MEMORY_PROTECTION_SPECIAL_REGION_PROTOCOL  *SpecialRegionProtocol;

  SpecialRegionProtocol = NULL;

  if (mSpecialRegions != NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->LocateProtocol (
                  &gMemoryProtectionSpecialRegionProtocolGuid,
                  NULL,
                  (VOID **)&SpecialRegionProtocol
                  );

  if (!EFI_ERROR (Status)) {
    Status = SpecialRegionProtocol->GetSpecialRegions (
                                      &mSpecialRegions,
                                      &mSpecialRegionCount
                                      );
  }

  return Status;
}

/**
  Checks if the address is a guard page.

  @param[in] Address            Address to check

  @retval TRUE                  The address is a guard page
  @retval FALSE                 The address is not a guard page
**/
BOOLEAN
IsGuardPage (
  IN UINT64  Address
  )
{
  if (mMemoryProtectionProtocol != NULL) {
    return mMemoryProtectionProtocol->IsGuardPage (Address);
  }

  return FALSE;
}

/**
  In processors implementing the AMD64 architecture, SMBASE relocation is always supported.
  However, there are some virtual platforms that does not really support them. This PCD check
  is to allow these virtual platforms to skip the SMRR check.

  @retval TRUE                  Skip the SMRR check
  @retval FALSE                 Do not skip the SMRR check
**/
BOOLEAN
SkipSmrr (
  VOID
  )
{
  return FixedPcdGetBool (PcdPlatformSmrrUnsupported);
}

/**
  Checks if a region is allowed to be read/write/execute based on the special region array
  and non protected image list

  @param[in] Address            Start address of the region
  @param[in] Length             Length of the region

  @retval TRUE                  The region is allowed to be read/write/execute
  @retval FALSE                 The region is not allowed to be read/write/execute
**/
BOOLEAN
CheckProjectMuRWXExemption (
  IN UINT64  Address,
  IN UINT64  Length
  )
{
  LIST_ENTRY              *NonProtectedImageLink;
  IMAGE_RANGE_DESCRIPTOR  *NonProtectedImage;
  UINTN                   SpecialRegionIndex;

  if (mSpecialRegions != NULL) {
    for (SpecialRegionIndex = 0; SpecialRegionIndex < mSpecialRegionCount; SpecialRegionIndex++) {
      if (CHECK_SUBSUMPTION (
            mSpecialRegions[SpecialRegionIndex].Start,
            mSpecialRegions[SpecialRegionIndex].Start + mSpecialRegions[SpecialRegionIndex].Length,
            Address,
            Address + Length
            ) &&
          (mSpecialRegions[SpecialRegionIndex].EfiAttributes == 0))
      {
        return TRUE;
      }
    }
  }

  if (mNonProtectedImageList != NULL) {
    for (NonProtectedImageLink = mNonProtectedImageList->Link.ForwardLink;
         NonProtectedImageLink != &mNonProtectedImageList->Link;
         NonProtectedImageLink = NonProtectedImageLink->ForwardLink)
    {
      NonProtectedImage = CR (NonProtectedImageLink, IMAGE_RANGE_DESCRIPTOR, Link, IMAGE_RANGE_DESCRIPTOR_SIGNATURE);
      if (CHECK_SUBSUMPTION (
            NonProtectedImage->Base,
            NonProtectedImage->Base + NonProtectedImage->Length,
            Address,
            Address + Length
            ))
      {
        return TRUE;
      }
    }
  }

  return FALSE;
}
