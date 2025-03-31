#ifndef MEMORY_PATTERNS_H
#define MEMORY_PATTERNS_H

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#define ONE_GIGABYTE_BYTES  (1024 * 1024 * 1024)

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
  );

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
  );

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
  );

#endif // MEMORY_PATTERNS_H
