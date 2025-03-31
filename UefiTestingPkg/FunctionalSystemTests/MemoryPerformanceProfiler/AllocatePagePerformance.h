#ifndef ALLOCATE_PAGE_PERFORMANCE_H
#define ALLOCATE_PAGE_PERFORMANCE_H

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PerformanceLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiLib.h>

//
// Macro to convert gigabytes to megabytes and initialize variables for performance testing
//
#define GIGABYTES_TO_MEGABYTES(Gb)  ((Gb) * 1024)

/**
 * @brief Measures the time taken to allocate and deallocate 1GB of memory.
 *
 * This function allocates 1GB of memory, performs a simple operation to ensure
 * the memory is actually allocated, and then deallocates the memory. It measures
 * the time taken for these operations and returns the duration in milliseconds.
 *
 * @return The duration in milliseconds taken to allocate and deallocate 1GB of memory.
 */
EFI_STATUS
EFIAPI
AllocateAnyPages1GbPage (
  VOID
  );

/**
 * @brief Measures the time taken to allocate and deallocate 2GB of memory.
 *
 * This function allocates 2GB of memory, performs a simple operation to ensure
 * the memory is actually allocated, and then deallocates the memory. It measures
 * the time taken for these operations and returns the duration in milliseconds.
 *
 * @return The duration in milliseconds taken to allocate and deallocate 1GB of memory.
 */
EFI_STATUS
EFIAPI
AllocateAnyPages2GbPage (
  VOID
  );

/**
 * @brief Measures the time taken to allocate and deallocate 4GB of memory.
 *
 * This function allocates 4GB of memory, performs a simple operation to ensure
 * the memory is actually allocated, and then deallocates the memory. It measures
 * the time taken for these operations and returns the duration in milliseconds.
 *
 * @return The duration in milliseconds taken to allocate and deallocate 1GB of memory.
 */
EFI_STATUS
EFIAPI
AllocateAnyPages4GbPage (
  VOID
  );

/**
 * @brief Measures the time taken to allocate and deallocate 8GB of memory.
 *
 * This function allocates 8GB of memory, performs a simple operation to ensure
 * the memory is actually allocated, and then deallocates the memory. It measures
 * the time taken for these operations and returns the duration in milliseconds.
 *
 * @return The duration in milliseconds taken to allocate and deallocate 1GB of memory.
 */
EFI_STATUS
EFIAPI
AllocateAnyPages8GbPage (
  VOID
  );

/**
 * @brief Measures the time taken to allocate and deallocate 1GB of memory at a specific address.
 *
 * @param[in] StartAddress The starting address for the allocations.
 *
 */
EFI_STATUS
EFIAPI
AllocateByAddress1GbPage (
  EFI_PHYSICAL_ADDRESS  StartAddress
  );

/**
 * @brief Measures the time taken to allocate and deallocate 2GB of memory at a specific address.
 *
 * @param[in] StartAddress The starting address for the allocations.
 *
 */
EFI_STATUS
EFIAPI
AllocateByAddress2GbPage (
  EFI_PHYSICAL_ADDRESS  StartAddress
  );

/**
 * @brief Measures the time taken to allocate and deallocate 4GB of memory at a specific address.
 *
 * @param[in] StartAddress The starting address for the allocations.
 *
 */
EFI_STATUS
EFIAPI
AllocateByAddress4GbPage (
  EFI_PHYSICAL_ADDRESS  StartAddress
  );

/**
 * @brief Measures the time taken to allocate and deallocate 8GB of memory at a specific address.
 *
 * @param[in] StartAddress The starting address for the allocations.
 *
 */
EFI_STATUS
EFIAPI
AllocateByAddress8GbPage (
  EFI_PHYSICAL_ADDRESS  StartAddress
  );

#endif // ALLOCATE_PAGE_PERFORMANCE_H
