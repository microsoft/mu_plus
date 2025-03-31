#include <Uefi.h>
#include <Library/PerformanceLib.h>
#include <Protocol/SimpleFileSystem.h>

#include "MemoryPatterns.h"
#include "AllocatePagePerformance.h"

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
  )
{
  EFI_STATUS  Status;

  PERF_FUNCTION_BEGIN ();

  Status = AllocatePagesBlocksStressTest (EFI_SIZE_TO_PAGES (ONE_GIGABYTE_BYTES), 1, AllocateAnyPages, EfiBootServicesData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AlignedAllocatePages(..) Failed: %r\n", Status));
    goto Exit;
  }

  Status = EFI_SUCCESS;

Exit:
  PERF_FUNCTION_END ();

  return Status;
}

/**
 * @brief Measures the time taken to allocate and deallocate 2GB of memory.
 *
 * This function allocates 1GB of memory, performs a simple operation to ensure
 * the memory is actually allocated, and then deallocates the memory. It measures
 * the time taken for these operations and returns the duration in milliseconds.
 *
 * @return The duration in milliseconds taken to allocate and deallocate 1GB of memory.
 */
EFI_STATUS
EFIAPI
AllocateAnyPages2GbPage (
  )
{
  EFI_STATUS  Status;

  PERF_FUNCTION_BEGIN ();

  Status = AllocatePagesBlocksStressTest (EFI_SIZE_TO_PAGES (ONE_GIGABYTE_BYTES), 2, AllocateAnyPages, EfiBootServicesData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AlignedAllocatePages(..) Failed: %r\n", Status));
    goto Exit;
  }

  Status = EFI_SUCCESS;

Exit:
  PERF_FUNCTION_END ();

  return Status;
}

/**
 * @brief Measures the time taken to allocate and deallocate 4GB of memory.
 *
 * This function allocates 1GB of memory, performs a simple operation to ensure
 * the memory is actually allocated, and then deallocates the memory. It measures
 * the time taken for these operations and returns the duration in milliseconds.
 *
 * @return The duration in milliseconds taken to allocate and deallocate 1GB of memory.
 */
EFI_STATUS
EFIAPI
AllocateAnyPages4GbPage (
  )
{
  EFI_STATUS  Status;

  PERF_FUNCTION_BEGIN ();

  Status = AllocatePagesBlocksStressTest (EFI_SIZE_TO_PAGES (ONE_GIGABYTE_BYTES), 4, AllocateAnyPages, EfiBootServicesData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AlignedAllocatePages(..) Failed: %r\n", Status));
    goto Exit;
  }

  Status = EFI_SUCCESS;

Exit:
  PERF_FUNCTION_END ();

  return Status;
}

/**
 * @brief Measures the time taken to allocate and deallocate 8GB of memory.
 *
 * This function allocates 1GB of memory, performs a simple operation to ensure
 * the memory is actually allocated, and then deallocates the memory. It measures
 * the time taken for these operations and returns the duration in milliseconds.
 *
 * @return The duration in milliseconds taken to allocate and deallocate 1GB of memory.
 */
EFI_STATUS
EFIAPI
AllocateAnyPages8GbPage (
  )
{
  EFI_STATUS  Status;

  PERF_FUNCTION_BEGIN ();

  Status = AllocatePagesBlocksStressTest ((EFI_SIZE_TO_PAGES (ONE_GIGABYTE_BYTES)), 8, AllocateAnyPages, EfiBootServicesData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AlignedAllocatePages(..) Failed: %r\n", Status));
    goto Exit;
  }

  Status = EFI_SUCCESS;

Exit:
  PERF_FUNCTION_END ();

  return Status;
}

/**
 * @brief Measures the time taken to allocate and deallocate 1GB of memory at a specific address.
 *
 * @param[in] StartAddress The starting address for the allocations.
 *
 * @return The duration in milliseconds taken to allocate and deallocate 1GB of memory.
 */
EFI_STATUS
EFIAPI
AllocateByAddress1GbPage (
  EFI_PHYSICAL_ADDRESS  StartAddress
  )
{
  EFI_STATUS  Status;

  PERF_FUNCTION_BEGIN ();

  Status = AllocateByAddressStressTest (EFI_SIZE_TO_PAGES (ONE_GIGABYTE_BYTES), 1, EfiBootServicesData, StartAddress);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AllocateByAddressStressTest(..) Failed: %r\n", Status));
    goto Exit;
  }

  Status = EFI_SUCCESS;

Exit:
  PERF_FUNCTION_END ();

  return Status;
}

/**
 * @brief Measures the time taken to allocate and deallocate 2GB of memory at a specific address.
 *
 * @param[in] StartAddress The starting address for the allocations.
 *
 * @return The duration in milliseconds taken to allocate and deallocate 2GB of memory.
 */
EFI_STATUS
EFIAPI
AllocateByAddress2GbPage (
  EFI_PHYSICAL_ADDRESS  StartAddress
  )
{
  EFI_STATUS  Status;

  PERF_FUNCTION_BEGIN ();

  Status = AllocateByAddressStressTest (EFI_SIZE_TO_PAGES (ONE_GIGABYTE_BYTES), 2, EfiBootServicesData, StartAddress);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AllocateByAddressStressTest(..) Failed: %r\n", Status));
    goto Exit;
  }

  Status = EFI_SUCCESS;

Exit:
  PERF_FUNCTION_END ();

  return Status;
}

/**
 * @brief Measures the time taken to allocate and deallocate 4GB of memory at a specific address.
 *
 * @param[in] StartAddress The starting address for the allocations.
 *
 * @return The duration in milliseconds taken to allocate and deallocate 4GB of memory.
 */
EFI_STATUS
EFIAPI
AllocateByAddress4GbPage (
  EFI_PHYSICAL_ADDRESS  StartAddress
  )
{
  EFI_STATUS  Status;

  PERF_FUNCTION_BEGIN ();

  Status = AllocateByAddressStressTest (EFI_SIZE_TO_PAGES (ONE_GIGABYTE_BYTES), 4, EfiBootServicesData, StartAddress);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AllocateByAddressStressTest(..) Failed: %r\n", Status));
    goto Exit;
  }

  Status = EFI_SUCCESS;

Exit:
  PERF_FUNCTION_END ();

  return Status;
}

/**
 * @brief Measures the time taken to allocate and deallocate 8GB of memory at a specific address.
 *
 * @param[in] StartAddress The starting address for the allocations.
 *
 * @return The duration in milliseconds taken to allocate and deallocate 8GB of memory.
 */
EFI_STATUS
EFIAPI
AllocateByAddress8GbPage (
  EFI_PHYSICAL_ADDRESS  StartAddress
  )
{
  EFI_STATUS  Status;

  PERF_FUNCTION_BEGIN ();

  Status = AllocateByAddressStressTest (EFI_SIZE_TO_PAGES (ONE_GIGABYTE_BYTES), 8, EfiBootServicesData, StartAddress);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AllocateByAddressStressTest(..) Failed: %r\n", Status));
    goto Exit;
  }

  Status = EFI_SUCCESS;

Exit:
  PERF_FUNCTION_END ();

  return Status;
}
