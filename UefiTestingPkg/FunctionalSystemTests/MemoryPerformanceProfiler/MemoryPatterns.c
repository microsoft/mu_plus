#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include "MemoryPatterns.h"

/**
  Measures memory allocations by pool.

  @param[in] AllocationSize       The size of each allocation in bytes.
  @param[in] AllocationsCount     The number of allocations to measure.
  @param[in] MemoryType           The type of memory to allocate (EfiBootServicesData, EfiRuntimeServicesData, etc.).

  @retval EFI_SUCCESS             The allocations were measured successfully.
  @retval EFI_INVALID_PARAMETER   One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES    There are not enough resources to complete the measurement.
**/
EFI_STATUS
EFIAPI
AlignedAllocatePool (
  UINT32           AllocationSize,
  UINTN            AllocationsCount,
  EFI_MEMORY_TYPE  MemoryType
  )
{
  EFI_STATUS  Status;
  VOID        *Memory;
  UINTN       Index;

  VOID  **PointerArray;

  // Allocate an array of pointers to hold the allocated memory blocks

  PointerArray = AllocatePool (AllocationsCount * sizeof (VOID *));
  if (PointerArray == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Initialize the pointer array to NULL
  for (Index = 0; Index < AllocationsCount; Index++) {
    PointerArray[Index] = NULL;
  }

  // allocate memory blocks and store the pointers in the array
  for (Index = 0; Index < AllocationsCount; Index++) {
    // Allocate memory
    Status = gBS->AllocatePool (MemoryType, AllocationSize, &Memory);
    if (EFI_ERROR (Status)) {
      FreePool (PointerArray); // Free the pointer array before returning
      return Status;           // Allocation failed
    }

    PointerArray[Index] = Memory; // Store the pointer in the array
  }

  // Free the allocated memory blocks
  for (Index = 0; Index < AllocationsCount; Index++) {
    if (PointerArray[Index] != NULL) {
      gBS->FreePool (PointerArray[Index]);
    }
  }

  return EFI_SUCCESS;
}

/**
  Measures memory allocations by pages.

  @param[in] PagesCount           The number of pages to allocate.
  @param[in] AllocationsCount     The number of allocations to measure.
  @param[in] AllocateType         The type of allocation to perform (AllocateAnyPages, AllocateMaxAddress, etc.).
  @param[in] MemoryType           The type of memory to allocate (EfiBootServicesData, EfiRuntimeServicesData, etc.).

  @retval EFI_SUCCESS             The allocations were measured successfully.
  @retval EFI_INVALID_PARAMETER   One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES    There are not enough resources to complete the measurement.
**/
EFI_STATUS
EFIAPI
AlignedAllocatePages (
  UINTN              PagesCount,
  UINTN              AllocationsCount,
  EFI_ALLOCATE_TYPE  AllocateType,
  EFI_MEMORY_TYPE    MemoryType
  )
{
  EFI_STATUS  Status;
  VOID        *Memory;
  UINTN       Index;

  VOID  **PointerArray;

  // Allocate an array of pointers to hold the allocated memory blocks

  PointerArray = AllocatePool (AllocationsCount * sizeof (VOID *));
  if (PointerArray == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Initialize the pointer array to NULL
  for (Index = 0; Index < AllocationsCount; Index++) {
    PointerArray[Index] = NULL;
  }

  // allocate memory blocks and store the pointers in the array
  for (Index = 0; Index < AllocationsCount; Index++) {
    // Allocate memory
    Status = gBS->AllocatePages (AllocateType, MemoryType, PagesCount, (EFI_PHYSICAL_ADDRESS *)&Memory);
    if (EFI_ERROR (Status)) {
      FreePool (PointerArray); // Free the pointer array before returning
      return Status;           // Allocation failed
    }

    PointerArray[Index] = Memory; // Store the pointer in the array
  }

  // Free the allocated memory blocks
  for (Index = 0; Index < AllocationsCount; Index++) {
    if (PointerArray[Index] != NULL) {
      gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)PointerArray[Index], PagesCount);
    }
  }

  return EFI_SUCCESS;
}
