/** @file
  CheckHardwareConnected

  Checks if a set of platform-defined PCI devices are discovered.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DeviceSpecificBusInfoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MuTelemetryHelperLib.h>
#include <Library/PciExpressLib.h>
#include <Library/UefiLib.h>

#include <Protocol/PciIo.h>

/**
  Checks if the platform provided set of PCI devices are currently present.

**/
VOID
CheckPciDevicePresence (
  VOID
  )
{
  EFI_STATUS            Status;
  BOOLEAN               *DeviceFound;
  DEVICE_PCI_INFO       *Devices;
  EFI_PCI_IO_PROTOCOL   **ProtocolList;

  UINTN   ProtocolCount;
  UINTN   Seg;
  UINTN   Bus;
  UINTN   Dev;
  UINTN   Fun;
  UINTN   NumDevices;
  UINTN   AdditionalData1;
  UINTN   Index;
  UINTN   DeviceIndex;

  DeviceFound   = NULL;
  Devices       = NULL;
  ProtocolList  = NULL;

  // Get the set of platform-defined PCI devices
  NumDevices = GetPciCheckDevices (&Devices);
  if (NumDevices == 0 || Devices == NULL) {
    return;
  }

  DeviceFound = AllocateZeroPool (sizeof (BOOLEAN) * NumDevices);
  if (DeviceFound == NULL) {
    return;
  }

  Status = EfiLocateProtocolBuffer (&gEfiPciIoProtocolGuid, &ProtocolCount, (VOID ***) &ProtocolList);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  for (Index = 0; Index < ProtocolCount; Index++) {
    Status = ProtocolList[Index]->GetLocation (ProtocolList[Index], &Seg, &Bus, &Dev, &Fun);
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (DeviceIndex = 0; DeviceIndex < NumDevices; DeviceIndex++) {
      // Check if that device matches the current protocol
      if (Seg == Devices[DeviceIndex].SegmentNumber &&
        Bus == Devices[DeviceIndex].BusNumber &&
        Dev == Devices[DeviceIndex].DeviceNumber &&
        Fun == Devices[DeviceIndex].FunctionNumber) {
        DeviceFound[DeviceIndex] = TRUE;
      }
    }
  }

  for (Index = 0; Index < NumDevices; Index++) {
    if (DeviceFound[Index] == FALSE) {
      // Get the BDF for AdditionalData1
      AdditionalData1 = PCI_ECAM_ADDRESS (
                          Devices[Index].BusNumber,
                          Devices[Index].DeviceNumber,
                          Devices[Index].FunctionNumber,
                          0
                          );

      LogTelemetry (
        Devices[Index].IsFatal,
        NULL,
        (EFI_IO_BUS_PCI | EFI_IOB_EC_NOT_DETECTED),
        &gDeviceSpecificBusInfoLibTelemetryGuid,
        NULL,
        AdditionalData1,
        *((UINT64 *) Devices[Index].DeviceName)
        );

      DEBUG ((
        DEBUG_ERROR,
        "%a - %a not found. Expected Segment: %d  Bus: %d  Device: %d  Function: %d\n",
        __FUNCTION__,
        Devices[Index].DeviceName,
        Devices[Index].SegmentNumber,
        Devices[Index].BusNumber,
        Devices[Index].DeviceNumber,
        Devices[Index].FunctionNumber
        ));
    }
  }

Cleanup:
  if (DeviceFound != NULL) {
    FreePool (DeviceFound);
  }
  if (ProtocolList != NULL) {
    FreePool (ProtocolList);
  }

  return;
}

/**
  The driver's entry point.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
CheckHardwareConnectedEntryPoint (
  IN    EFI_HANDLE                  ImageHandle,
  IN    EFI_SYSTEM_TABLE            *SystemTable
  )
{
  CheckPciDevicePresence ();

  return EFI_SUCCESS;
}
