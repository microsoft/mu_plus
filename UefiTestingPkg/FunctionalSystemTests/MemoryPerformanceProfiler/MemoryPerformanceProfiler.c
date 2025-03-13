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

#define ONE_GIGABYTE (1024 * 1024 * 1024)
#define ONE_GIGABYTE_IN_PAGES (ONE_GIGABYTE / EFI_PAGE_SIZE)


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

  ExecuteAllocatePagePerformanceProfiles ();

  AsciiPrint ("Memory Attributes Performance Test App completed.\n");
  AsciiPrint ("----------------------------------------\n");
  AsciiPrint ("Press any key to exit...\n");
  gBS->WaitForEvent (1, &SystemTable->ConIn->WaitForKey, NULL);
  gBS->CloseProtocol (ImageHandle, &gEfiSimpleTextInProtocolGuid, ImageHandle, NULL);

  return EFI_SUCCESS;
}
