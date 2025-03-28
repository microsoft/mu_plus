#include <Uefi.h>
#include <Library/PerformanceLib.h>
#include <Protocol/SimpleFileSystem.h>

#include "MemoryPatterns.h"
#include "AllocatePagePerformance.h"

#define GIGABYTES_TO_MEGABYTES(Gb)  ((Gb) * 1024)

#define INIT_VARIABLES()                     \
  EFI_STATUS  Status = EFI_SUCCESS;          \
  UINTN       Start = 0;                     \
  UINTN       End = 0;                       \
  UINTN       ElapsedTime = 0;               \
  UINT64      AvailableMemory = 0;

#define TEST_ALLOCATE_PAGES(SizeGb, Function)                                \
if (AvailableMemory >= GIGABYTES_TO_MEGABYTES (SizeGb)) {                  \
  AsciiPrint ("Testing: " #Function "(..)...");                            \
  Start  = GetPerformanceCounter ();                                       \
  Status = Function ();                                                    \
  if (EFI_ERROR (Status)) {                                                \
    DEBUG ((DEBUG_ERROR, #Function "(..) Failed: %r\n", Status));          \
    return Status;                                                         \
  }                                                                        \
  End         = GetPerformanceCounter ();                                  \
  ElapsedTime = GetTimeInNanoSecond (End - Start) / 1000000;               \
  AsciiPrint (" Elapsed Time: %llu ms (%llu ns)\n", ElapsedTime,           \
              GetTimeInNanoSecond (End - Start));                          \
}

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

  Status = BlocksOfPagesAllocatePages (EFI_SIZE_TO_PAGES (ONE_GIGABYTE_BYTES), 1, AllocateAnyPages, EfiBootServicesData);
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

  Status = BlocksOfPagesAllocatePages (EFI_SIZE_TO_PAGES (ONE_GIGABYTE_BYTES), 2, AllocateAnyPages, EfiBootServicesData);
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

  Status = BlocksOfPagesAllocatePages (EFI_SIZE_TO_PAGES (ONE_GIGABYTE_BYTES), 4, AllocateAnyPages, EfiBootServicesData);
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

  Status = BlocksOfPagesAllocatePages ((EFI_SIZE_TO_PAGES (ONE_GIGABYTE_BYTES)), 8, AllocateAnyPages, EfiBootServicesData);
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
  UINTN                 Pages,
  EFI_PHYSICAL_ADDRESS  *FreeAddress
  )
{
  EFI_STATUS             Status;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap = NULL;
  EFI_MEMORY_DESCRIPTOR  *Descriptor;
  UINTN                  MemoryMapSize = 0;
  UINTN                  MapKey;
  UINTN                  DescriptorSize;
  UINT32                 DescriptorVersion;
  UINTN                  Index;

  // Get the memory map size
  Status = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  // Allocate memory for the memory map
  MemoryMap = AllocatePool (MemoryMapSize);
  if (MemoryMap == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Get the memory map
  Status = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (EFI_ERROR (Status)) {
    FreePool (MemoryMap);
    return Status;
  }

  // Iterate through the memory map to find a free region
  Descriptor = MemoryMap;
  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); Index++) {
    if ((Descriptor->Type == EfiConventionalMemory) && (Descriptor->NumberOfPages >= Pages)) {
      *FreeAddress = Descriptor->PhysicalStart;
      FreePool (MemoryMap);
      return EFI_SUCCESS;
    }

    Descriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)Descriptor + DescriptorSize);
  }

  FreePool (MemoryMap);
  return EFI_NOT_FOUND;
}

/**
 * @brief Determines the total amount of available memory (RAM) on the platform.
 *
 * This function iterates through the memory map to calculate the total amount
 * of free memory available in the system.
 *
 * @param AvailableMemory A pointer to store the total available memory in bytes.
 * @return EFI_STATUS indicating success or failure of the operation.
 */
EFI_STATUS
EFIAPI
GetAvailableMemory (
  UINT64  *AvailableMemory
  )
{
  EFI_STATUS             Status;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap = NULL;
  EFI_MEMORY_DESCRIPTOR  *Descriptor;
  UINTN                  MemoryMapSize = 0;
  UINTN                  MapKey;
  UINTN                  DescriptorSize;
  UINT32                 DescriptorVersion;
  UINTN                  Index;

  if (AvailableMemory == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *AvailableMemory = 0;

  // Get the memory map size
  Status = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  // Allocate memory for the memory map
  MemoryMap = AllocatePool (MemoryMapSize);
  if (MemoryMap == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Get the memory map
  Status = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (EFI_ERROR (Status)) {
    FreePool (MemoryMap);
    return Status;
  }

  // Iterate through the memory map to calculate available memory
  Descriptor = MemoryMap;
  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); Index++) {
    if (Descriptor->Type == EfiConventionalMemory) {
      *AvailableMemory += Descriptor->NumberOfPages * EFI_PAGE_SIZE;
    }

    Descriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)Descriptor + DescriptorSize);
  }

  FreePool (MemoryMap);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WriteToFile (
  EFI_FILE_PROTOCOL  *File,
  CHAR8              *Buffer
  )
{
  UINTN  BufferSize;

  if ((File == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  BufferSize = AsciiStrSize (Buffer) - 1; // Exclude the null terminator
  return File->Write (File, &BufferSize, Buffer);
}

EFI_STATUS
EFIAPI
RecordMemoryMap (
  CHAR16  *FileName
  )
{
  EFI_STATUS             Status;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap = NULL;
  EFI_MEMORY_DESCRIPTOR  *Descriptor;
  UINTN                  MemoryMapSize = 0;
  UINTN                  MapKey;
  UINTN                  DescriptorSize;
  UINT32                 DescriptorVersion;
  UINTN                  Index;

  EFI_FILE_PROTOCOL                *File;
  EFI_FILE_PROTOCOL                *Root;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SimpleFileSystem;
  CHAR8                            Buffer[256];

  //
  // Locate the Simple File System protocol
  //
  Status = gBS->LocateProtocol (&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID **)&SimpleFileSystem);
  if (EFI_ERROR (Status)) {
    Print (L"Failed to locate Simple File System protocol: %r\n", Status);
    return Status;
  }

  //
  // Open the root directory
  //
  Status = SimpleFileSystem->OpenVolume (SimpleFileSystem, &Root);
  if (EFI_ERROR (Status)) {
    Print (L"Failed to open root directory: %r\n", Status);
    return Status;
  }

  //
  // Create a new file
  //
  Status = Root->Open (Root, &File, FileName, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    Print (L"Failed to create file: %r\n", Status);
    return Status;
  }

  //
  // Get the memory map size
  //
  Status = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    DEBUG ((DEBUG_ERROR, "Failed to get memory map size: %r\n", Status));
    File->Close (File);
    return Status;
  }

  //
  // Allocate memory for the memory map
  //
  MemoryMap = AllocatePool (MemoryMapSize);
  if (MemoryMap == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate memory for memory map\n"));
    File->Close (File);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get the memory map
  //
  Status = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get memory map: %r\n", Status));
    FreePool (MemoryMap);
    File->Close (File);
    return Status;
  }

  //
  // Write the memory map to the file
  //
  WriteToFile (File, "Memory Map:\r\n");
  WriteToFile (File, "Type                PhysicalStart    NumberOfPages    Attribute\r\n");
  WriteToFile (File, "-----------------------------------------------------------------\r\n");

  Descriptor = MemoryMap;
  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); Index++) {
    ZeroMem (Buffer, sizeof (Buffer));

    AsciiSPrint (
      Buffer,
      sizeof (Buffer),
      "%-24a 0x%016lx 0x%08lx 0x%016lx\r\n",
      (Descriptor->Type == EfiConventionalMemory) ? "ConventionalMemory" :
      (Descriptor->Type == EfiLoaderCode) ? "LoaderCode" :
      (Descriptor->Type == EfiLoaderData) ? "LoaderData" :
      (Descriptor->Type == EfiBootServicesCode) ? "BootServicesCode" :
      (Descriptor->Type == EfiBootServicesData) ? "BootServicesData" :
      (Descriptor->Type == EfiRuntimeServicesCode) ? "RuntimeServicesCode" :
      (Descriptor->Type == EfiRuntimeServicesData) ? "RuntimeServicesData" :
      (Descriptor->Type == EfiUnusableMemory) ? "UnusableMemory" :
      (Descriptor->Type == EfiACPIReclaimMemory) ? "ACPIReclaimMemory" :
      (Descriptor->Type == EfiACPIMemoryNVS) ? "ACPIMemoryNVS" :
      (Descriptor->Type == EfiMemoryMappedIO) ? "MemoryMappedIO" :
      (Descriptor->Type == EfiMemoryMappedIOPortSpace) ? "MemoryMappedIOPortSpace" :
      (Descriptor->Type == EfiPalCode) ? "PalCode" :
      (Descriptor->Type == EfiPersistentMemory) ? "PersistentMemory" :
      (Descriptor->Type == EfiMaxMemoryType) ? "MaxMemoryType" :
      (Descriptor->Type == EfiReservedMemoryType) ? "ReservedMemoryType" :
      "Unknown",
      Descriptor->PhysicalStart,
      Descriptor->NumberOfPages,
      Descriptor->Attribute
      );

    WriteToFile (File, Buffer);

    Descriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)Descriptor + DescriptorSize);
  }

  FreePool (MemoryMap);
  File->Close (File);
  return EFI_SUCCESS;
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

  UINT64  AvailableMemory;

  //
  // Find the total available memory
  //
  Status = GetAvailableMemory (&AvailableMemory);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GetAvailableMemory(..) Failed: %r\n", Status));
    return Status;
  }

  //
  // Convert to MB
  //
  AvailableMemory = AvailableMemory / (1024 * 1024);

  AsciiPrint (" Total Available Memory: %llu MB\n", AvailableMemory);

  RecordMemoryMap (L"MemoryMap.txt");

  AsciiPrint ("----------------------------------------\n");
  AsciiPrint ("Profile:\n");
  AsciiPrint ("\tEFI_ALLOCATION_TYPE: AllocateAnyPages\n");
  AsciiPrint ("\tEFI_MEMORY_TYPE: EfiBootServicesData\n");

  TEST_ALLOCATE_PAGES(1, AllocateAnyPages1GbPage);
  TEST_ALLOCATE_PAGES(2, AllocateAnyPages2GbPage);
  TEST_ALLOCATE_PAGES(4, AllocateAnyPages4GbPage);
  TEST_ALLOCATE_PAGES(8, AllocateAnyPages8GbPage);


  return Status;
}
