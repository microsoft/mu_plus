#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

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
AllocatePoolBlocksStressTest (
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
      goto EXIT;   // Allocation failed
    }

    PointerArray[Index] = Memory; // Store the pointer in the array
  }

  Status = EFI_SUCCESS;
EXIT:

  // Free the allocated memory blocks
  for (Index = 0; Index < AllocationsCount; Index++) {
    if (PointerArray[Index] != NULL) {
      gBS->FreePool (PointerArray[Index]);
    }
  }

  FreePool (PointerArray); // Free the pointer array before returning

  return Status;
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
AllocatePagesBlocksStressTest (
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

  SetMem (PointerArray, AllocationsCount * sizeof (VOID *), 0); // Initialize the pointer array to NULL

  // allocate memory blocks and store the pointers in the array
  for (Index = 0; Index < AllocationsCount; Index++) {
    Status = gBS->AllocatePages (AllocateType, MemoryType, PagesCount, (EFI_PHYSICAL_ADDRESS *)&Memory);
    if (EFI_ERROR (Status)) {
      goto Exit;   // Allocation failed
    }

    PointerArray[Index] = Memory; // Store the pointer in the array
  }

  Status = EFI_SUCCESS;
Exit:

  // Free the allocated memory blocks
  for (Index = 0; Index < AllocationsCount; Index++) {
    if (PointerArray[Index] != NULL) {
      gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)PointerArray[Index], PagesCount);
    }
  }

  FreePool (PointerArray); // Free the pointer array before returning

  return Status;
}

/**
  Allocates memory at specific addresses to stress the system.

  @param[in] PagesCount           The number of pages to allocate.
  @param[in] AllocationsCount     The number of allocations to measure.
  @param[in] MemoryType           The type of memory to allocate (EfiBootServicesData, EfiRuntimeServicesData, etc.).
  @param[in] StartAddress         The starting address for the allocations.

  @retval EFI_SUCCESS             The allocations were performed successfully.
  @retval EFI_INVALID_PARAMETER   One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES    There are not enough resources to complete the allocations.
**/
EFI_STATUS
EFIAPI
AllocateByAddressStressTest (
  UINTN                 PagesCount,
  UINTN                 AllocationsCount,
  EFI_MEMORY_TYPE       MemoryType,
  EFI_PHYSICAL_ADDRESS  StartAddress
  )
{
  EFI_STATUS            Status;
  UINTN                 Index;
  EFI_PHYSICAL_ADDRESS  Address;
  VOID                  **PointerArray;

  // Allocate an array of pointers to hold the allocated memory blocks
  PointerArray = AllocatePool (AllocationsCount * sizeof (VOID *));
  if (PointerArray == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Initialize the pointer array to NULL using memset
  SetMem (PointerArray, AllocationsCount * sizeof (VOID *), 0);

  // Allocate memory blocks at specific addresses and store the pointers in the array
  for (Index = 0; Index < AllocationsCount; Index++) {
    Address = StartAddress + (Index * PagesCount * EFI_PAGE_SIZE);

    Status = gBS->AllocatePages (AllocateAddress, MemoryType, PagesCount, &Address);
    if (EFI_ERROR (Status)) {
      goto EXIT;   // Allocation failed
    }

    PointerArray[Index] = (VOID *)(UINTN)Address; // Store the pointer in the array
  }

  Status = EFI_SUCCESS;
EXIT:
  // Free the allocated memory blocks
  for (Index = 0; Index < AllocationsCount; Index++) {
    if (PointerArray[Index] != NULL) {
      gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)PointerArray[Index], PagesCount);
    }
  }

  FreePool (PointerArray);
  return Status;
}
