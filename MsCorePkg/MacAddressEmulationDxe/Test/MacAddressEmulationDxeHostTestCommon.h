#pragma once

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/NetLib.h>
#include <Library/UnitTestLib.h>
#include <Library/DebugLib.h>



#include <Protocol/SimpleNetwork.h>

#include "../MacAddressEmulationDxe.h"

EFI_STATUS
EFIAPI 
LocateHandleBuffer (
  IN     EFI_LOCATE_SEARCH_TYPE       SearchType,
  IN     EFI_GUID                     *Protocol       OPTIONAL,
  IN     VOID                         *SearchKey      OPTIONAL,
  OUT    UINTN                        *NoHandles,
  OUT    EFI_HANDLE                   **Buffer
  );

EFI_STATUS
EFIAPI
HandleProtocol (
  IN  EFI_HANDLE               Handle,
  IN  EFI_GUID                 *Protocol,
  OUT VOID                     **Interface
  );