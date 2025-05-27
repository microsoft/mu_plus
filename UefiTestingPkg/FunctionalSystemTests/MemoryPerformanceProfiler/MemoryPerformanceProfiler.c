#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/PerformanceLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiApplicationEntryPoint.h>

#include "AllocatePagePerformance.h"

#define ONE_GIGABYTE           (1024 * 1024 * 1024)
#define ONE_GIGABYTE_IN_PAGES  (ONE_GIGABYTE / EFI_PAGE_SIZE)

typedef
EFI_STATUS
(EFIAPI *ALLOCATE_ANY_PAGES_FUNC)(
  VOID
  );

typedef
EFI_STATUS
(EFIAPI *ALLOCATE_BY_ADDRESS_FUNC)(
  EFI_PHYSICAL_ADDRESS  StartAddress
  );

typedef struct {
  UINT64                      RequiredMemoryInGB;
  ALLOCATE_ANY_PAGES_FUNC     AllocateAnyPagesFunc;
  ALLOCATE_BY_ADDRESS_FUNC    AllocateByAddressFunc;
} AllocatePageTests;

VOID
EFIAPI
SetupConsole (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *ConOut
  )
{
  ConOut->SetAttribute (ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
  ConOut->ClearScreen (ConOut);
  ConOut->SetCursorPosition (ConOut, 0, 0);
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
RunStressTest (
  UINT64                    RequiredMemoryInGB,
  ALLOCATE_ANY_PAGES_FUNC   AllocateAnyPagesFunc,
  ALLOCATE_BY_ADDRESS_FUNC  AllocateByAddressFunc,
  UINT64                    AvailableMemory
  )
{
  EFI_STATUS            Status;
  UINT64                Start;
  UINT64                End;
  UINT64                ElapsedTime;
  EFI_PHYSICAL_ADDRESS  FreeAddress;
  UINT64                PagesPerGB;

  PagesPerGB = EFI_SIZE_TO_PAGES (RequiredMemoryInGB * ONE_GIGABYTE);

  if (AvailableMemory >= GIGABYTES_TO_MEGABYTES (RequiredMemoryInGB)) {
    AsciiPrint ("Testing: AllocateAnyPages%lluGbPage(..)...", RequiredMemoryInGB);
    Start  = GetPerformanceCounter ();
    Status = AllocateAnyPagesFunc ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "AllocateAnyPages%lluGbPage(..) Failed: %r\n", RequiredMemoryInGB, Status));
      return Status;
    }

    End         = GetPerformanceCounter ();
    ElapsedTime = GetTimeInNanoSecond (End - Start) / 1000000;
    AsciiPrint (
      " Elapsed Time: %llu ms (%llu ns)\n",
      ElapsedTime,
      GetTimeInNanoSecond (End - Start)
      );

    AsciiPrint ("----------------------------------------\n");

    Status = FindFreeAddress (PagesPerGB, &FreeAddress);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "FindFreeAddress(..) Failed: %r\n", Status));
      return Status;
    }

    AsciiPrint ("Testing: AllocateByAddress%lluGbPage(..)...", RequiredMemoryInGB);
    Start  = GetPerformanceCounter ();
    Status = AllocateByAddressFunc (FreeAddress);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "AllocateByAddress%lluGbPage(..) Failed: %r\n", RequiredMemoryInGB, Status));
      return Status;
    }

    End         = GetPerformanceCounter ();
    ElapsedTime = GetTimeInNanoSecond (End - Start) / 1000000;

    AsciiPrint (
      " Elapsed Time: %llu ms (%llu ns)\n",
      ElapsedTime,
      GetTimeInNanoSecond (End - Start)
      );
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
ExecuteAllocatePagePerformanceProfiles (
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINT64      AvailableMemory;

  AllocatePageTests  StressTests[4] = {
    { 1, AllocateAnyPages1GbPage, AllocateByAddress1GbPage },
    { 2, AllocateAnyPages2GbPage, AllocateByAddress2GbPage },
    { 4, AllocateAnyPages4GbPage, AllocateByAddress4GbPage },
    { 8, AllocateAnyPages8GbPage, AllocateByAddress8GbPage }
  };

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

  AsciiPrint ("----------------------------------------\n");
  for (UINTN i = 0; i < sizeof (StressTests) / sizeof (StressTests[0]); i++) {
    Status = RunStressTest (
               StressTests[i].RequiredMemoryInGB,
               StressTests[i].AllocateAnyPagesFunc,
               StressTests[i].AllocateByAddressFunc,
               AvailableMemory
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return Status;
}

/**
  The entry point for the UEFI application.

  @param[in] ImageHandle   The firmware allocated handle for the UEFI image.
  @param[in] SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS      The entry point is executed successfully.
  @retval Other            Some error occurred when executing this entry point.
**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // Initialize the console output protocol
  //
  SetupConsole (SystemTable->ConOut);

  //
  // Print the header information
  //

  AsciiPrint ("Memory Performance Profiler\n");
  AsciiPrint ("----------------------------------------\n");
  AsciiPrint ("This application measures memory allocation performance.\n");
  AsciiPrint ("It allocates and frees memory blocks of different sizes in varying patterns.\n");
  AsciiPrint ("The results are displayed in milliseconds.\n");
  AsciiPrint ("----------------------------------------\n");

  RecordMemoryMap (L"MemoryMap.txt");

  ExecuteAllocatePagePerformanceProfiles ();

  AsciiPrint ("Memory Attributes Performance Test App completed.\n");
  AsciiPrint ("----------------------------------------\n");
  AsciiPrint ("Press any key to exit...\n");
  gBS->WaitForEvent (1, &SystemTable->ConIn->WaitForKey, NULL);
  gBS->CloseProtocol (ImageHandle, &gEfiSimpleTextInProtocolGuid, ImageHandle, NULL);

  return EFI_SUCCESS;
}
