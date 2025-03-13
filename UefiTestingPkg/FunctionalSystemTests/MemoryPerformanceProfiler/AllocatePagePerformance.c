#include <Uefi.h>
#include <Library/PerformanceLib.h>

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
  )
{
  EFI_STATUS  Status;

  PERF_FUNCTION_BEGIN ();

  Status = AlignedAllocatePages (EFI_SIZE_TO_PAGES (ONE_GIGABYTE_BYTES), 1, AllocateAnyPages, EfiBootServicesData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AlignedAllocatePages(..) Failed: %r\n", Status));
    return Status;
  }

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

  Status = AlignedAllocatePages (EFI_SIZE_TO_PAGES (ONE_GIGABYTE_BYTES), 2, AllocateAnyPages, EfiBootServicesData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AlignedAllocatePages(..) Failed: %r\n", Status));
    return Status;
  }

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

  Status = AlignedAllocatePages (EFI_SIZE_TO_PAGES (ONE_GIGABYTE_BYTES), 4, AllocateAnyPages, EfiBootServicesData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AlignedAllocatePages(..) Failed: %r\n", Status));
    return Status;
  }

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

  Status = AlignedAllocatePages ((EFI_SIZE_TO_PAGES (ONE_GIGABYTE_BYTES)), 8, AllocateAnyPages, EfiBootServicesData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AlignedAllocatePages(..) Failed: %r\n", Status));
    return Status;
  }

  PERF_FUNCTION_END ();

  return Status;
}

/**
 * @brief Measures the time taken to allocate and deallocate memory at a specific address.
 *
 * This function allocates memory at a specific address, performs a simple operation to ensure
 * the memory is actually allocated, and then deallocates the memory. It measures
 * the time taken for these operations and returns the duration in milliseconds.
 *
 * @param Address The specific address where memory should be allocated.
 * @param Pages   The number of pages to allocate.
 * @return EFI_STATUS indicating success or failure of the operation.
 */
EFI_STATUS
EFIAPI
AllocateByAddress (
  EFI_PHYSICAL_ADDRESS Address,
  UINTN                Pages
  )
{
  EFI_STATUS Status;

  PERF_FUNCTION_BEGIN ();

  Status = AlignedAllocatePages (Pages, Address, AllocateAddress, EfiBootServicesData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AlignedAllocatePages(..) Failed: %r\n", Status));
    return Status;
  }

  PERF_FUNCTION_END ();

  return Status;
}

EFI_STATUS
EFIAPI
FindFreeAddress (
  UINTN                Pages,
  EFI_PHYSICAL_ADDRESS *FreeAddress
  )
{
  EFI_STATUS                  Status;
  EFI_MEMORY_DESCRIPTOR       *MemoryMap = NULL;
  EFI_MEMORY_DESCRIPTOR       *Descriptor;
  UINTN                       MemoryMapSize = 0;
  UINTN                       MapKey;
  UINTN                       DescriptorSize;
  UINT32                      DescriptorVersion;
  UINTN                       Index;

  // Get the memory map size
  Status = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  // Allocate memory for the memory map
  MemoryMap = AllocatePool(MemoryMapSize);
  if (MemoryMap == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Get the memory map
  Status = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (EFI_ERROR(Status)) {
    FreePool(MemoryMap);
    return Status;
  }

  // Iterate through the memory map to find a free region
  Descriptor = MemoryMap;
  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); Index++) {
    if (Descriptor->Type == EfiConventionalMemory && Descriptor->NumberOfPages >= Pages) {
      *FreeAddress = Descriptor->PhysicalStart;
      FreePool(MemoryMap);
      return EFI_SUCCESS;
    }
    Descriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)Descriptor + DescriptorSize);
  }

  FreePool(MemoryMap);
  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
ExecuteAllocatePagePerformanceProfiles (
  )
{
  EFI_STATUS  Status;
  UINTN       Start;
  UINTN       End;
  UINTN       ElapsedTime;

  AsciiPrint ("----------------------------------------\n");
  AsciiPrint ("Profile:\n");
  AsciiPrint ("\tEFI_ALLOCATION_TYPE: AllocateAnyPages\n");
  AsciiPrint ("\tEFI_MEMORY_TYPE: EfiBootServicesData\n");

  // ==========================================================================

  AsciiPrint ("Testing: AllocateAnyPages1GbPage(..)...");
  Start  = GetPerformanceCounter (); // Record start time
  Status = AllocateAnyPages1GbPage ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AllocateAnyPages1GbPage(..) Failed: %r\n", Status));
    return Status;
  }

  End         = GetPerformanceCounter ();                    // Record end time
  ElapsedTime = GetTimeInNanoSecond (End - Start) / 1000000; // Convert to milliseconds

  AsciiPrint (" Elapsed Time: %llu ms\n", ElapsedTime);
  // ===========================================================================

  AsciiPrint ("Testing: AllocateAnyPages2GbPage(..)...");
  Start  = GetPerformanceCounter (); // Record start time
  Status = AllocateAnyPages2GbPage ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AllocateAnyPages2GbPage(..) Failed: %r\n", Status));
    return Status;
  }

  End         = GetPerformanceCounter ();                            // Record end time
  ElapsedTime = GetTimeInNanoSecond (End - Start) / 1000000; // Convert to milliseconds

  AsciiPrint (" Elapsed Time: %llu ms\n", ElapsedTime);

  // ===========================================================================

  AsciiPrint ("Testing: AllocateAnyPages4GbPage(..)...");
  Start  = GetPerformanceCounter (); // Record start time
  Status = AllocateAnyPages4GbPage ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AllocateAnyPages4GbPage(..) Failed: %r\n", Status));
    return Status;
  }

  End         = GetPerformanceCounter ();                            // Record end time
  ElapsedTime = GetTimeInNanoSecond (End - Start) / 1000000; // Convert to milliseconds

  AsciiPrint (" Elapsed Time: %llu ms\n", ElapsedTime);

  // ===========================================================================

  AsciiPrint ("Testing: AllocateAnyPages8GbPage(..)...");
  Start  = GetPerformanceCounter (); // Record start time
  Status = AllocateAnyPages8GbPage ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AllocateAnyPages8GbPage(..) Failed: %r\n", Status));
    return Status;
  }

  End         = GetPerformanceCounter ();                            // Record end time
  ElapsedTime = GetTimeInNanoSecond (End - Start) / 1000000; // Convert to milliseconds

  AsciiPrint (" Elapsed Time: %llu ms\n", ElapsedTime);

  // ===========================================================================

  AsciiPrint ("Testing: FindFreeAddress(..)...");
  EFI_PHYSICAL_ADDRESS FreeAddress;
  UINTN Pages = EFI_SIZE_TO_PAGES(ONE_GIGABYTE_BYTES);

  Status = FindFreeAddress(Pages, &FreeAddress);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "FindFreeAddress(..) Failed: %r\n", Status));
    return Status;
  }

  AsciiPrint(" Found Free Address: 0x%lx\n", FreeAddress);

  return Status;
}
