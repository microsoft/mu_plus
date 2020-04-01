/** @file
CheckHardwareConnected.c

Checks if PCI devices defined in DeviceSpecificBusInfoLib were discovered
on the bus.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/DeviceSpecificBusInfoLib.h>
#include <Library/MuTelemetryHelperLib.h>
#include <Library/PciExpressLib.h>

#include <Protocol/PciIo.h>

/**
  Checks if PCI devices defined in DeviceSpecificBusInfoLib are connected

  @retval VOID
**/
VOID
CheckPCIDevices(VOID)
{

  // Nums for finding bus, seg, etc. and keeping count of how many devices found
  UINTN                  ProtocolCount, Seg, Bus, Dev, Fun, NumDevices, AdditionalData1, OuterLoop, InnerLoop;

  // To store protocols
  EFI_PCI_IO_PROTOCOL**  ProtocolList = NULL;

  // Pointer to the head of an array of DEVICE_PCI_INFO structures.
  DEVICE_PCI_INFO        *Devices = NULL;

  // Array parallel to Devices which we will use to check off which devices we've found
  BOOLEAN*               DeviceFound  = NULL;

  // Get devices that will be checked from the platform library.
  NumDevices = GetPciCheckDevices (&Devices);
  if (Devices == NULL) {
    return;
  }

  // Array to track which devices we've found
  DeviceFound = AllocateZeroPool(sizeof(BOOLEAN) * NumDevices);

  // Ensure that all necessary pointers have been populated, abort to cleanup if not
  if(Devices == NULL || DeviceFound == NULL || NumDevices == 0 ||
     EFI_ERROR(EfiLocateProtocolBuffer(&gEfiPciIoProtocolGuid, &ProtocolCount, (VOID*)&ProtocolList))) {
    goto CLEANUP;
  }

  // For each device protocol found...
  for(OuterLoop = 0; OuterLoop < ProtocolCount; OuterLoop++) {

    // Get device location
    if(EFI_ERROR(ProtocolList[OuterLoop]->GetLocation(ProtocolList[OuterLoop],&Seg,&Bus,&Dev,&Fun))) {
      continue;
    }

    // For each device supplied by DeviceSpecificBusInfoLib...
    for(InnerLoop = 0; InnerLoop < NumDevices; InnerLoop++) {
      // Check if that device matches the current protocol in OuterLoop
      if(Seg == Devices[InnerLoop].SegmentNumber && Bus == Devices[InnerLoop].BusNumber &&
         Dev == Devices[InnerLoop].DeviceNumber  && Fun == Devices[InnerLoop].FunctionNumber) {

        // If it matches, check it off in the parallel array
        DeviceFound[InnerLoop] = TRUE;
      }
    }
  }


  // For each device supplied by DeviceSpecificBusInfoLib...
  for(OuterLoop = 0; OuterLoop < NumDevices; OuterLoop++) {

    // Check if the previous loop found that device
    if(DeviceFound[OuterLoop] == FALSE) {

      // Get the BDF for AdditionalData1
      AdditionalData1 = PCI_ECAM_ADDRESS(Devices[OuterLoop].BusNumber,
                                         Devices[OuterLoop].DeviceNumber,
                                         Devices[OuterLoop].FunctionNumber,
                                         0
                                        );

      // Report an error if not
      LogTelemetry(Devices[OuterLoop].IsFatal,
                   NULL,
                   (EFI_IO_BUS_PCI | EFI_IOB_EC_NOT_DETECTED),
                   &gDeviceSpecificBusInfoLibTelemetryGuid,
                   NULL,
                   AdditionalData1,
                   *((UINT64*)Devices[OuterLoop].DeviceName));

      DEBUG ((DEBUG_INFO,"%a - %a not found. Expected Segment: %d  Bus: %d  Device: %d  Function: %d\n", __FUNCTION__,
        Devices[OuterLoop].DeviceName, Devices[OuterLoop].SegmentNumber, Devices[OuterLoop].BusNumber,
        Devices[OuterLoop].DeviceNumber, Devices[OuterLoop].FunctionNumber));

    }
  }

  // Make sure everything is freed
  CLEANUP:
    if(DeviceFound != NULL) {
      FreePool(DeviceFound);
    }
    if(ProtocolList != NULL) {
      FreePool(ProtocolList);
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
  CheckPCIDevices();

  return EFI_SUCCESS;
}
