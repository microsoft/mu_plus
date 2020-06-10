/** @file
  CheckHardwareConnected

  This driver performs a set of hardware checks on devices defined by the platform.

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

#include "CheckHardwareConnected.h"

/**
  Checks if the platform provided set of PCI devices are currently present.

**/
VOID
PerformPciChecks (
  VOID
  )
{
  EFI_STATUS                Status;
  PCIE_LINK_SPEED           DeviceLinkSpeed;
  DEVICE_PCI_CHECK_RESULT   *DeviceCheckResult;
  DEVICE_PCI_INFO           *Devices;
  EFI_PCI_IO_PROTOCOL       **ProtocolList;

  UINTN   ProtocolCount;
  UINTN   Seg;
  UINTN   Bus;
  UINTN   Dev;
  UINTN   Fun;
  UINTN   NumDevices;
  UINTN   AdditionalData1;
  UINTN   Index;
  UINTN   DeviceIndex;

  Devices       = NULL;
  ProtocolList  = NULL;

  // Get the set of platform-defined PCI devices
  NumDevices = GetPciCheckDevices (&Devices);
  if (NumDevices == 0 || Devices == NULL) {
    return;
  }

  DeviceCheckResult = AllocateZeroPool (sizeof (DEVICE_PCI_CHECK_RESULT) * NumDevices);
  if (DeviceCheckResult == NULL) {
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
        DeviceCheckResult[DeviceIndex].DevicePresent = TRUE;
      }
    }
  }

  for (Index = 0; Index < NumDevices; Index++) {
    // Get the BDF for AdditionalData1 to be used in potential telemetry calls
    AdditionalData1 = PCI_ECAM_ADDRESS (
                        Devices[Index].BusNumber,
                        Devices[Index].DeviceNumber,
                        Devices[Index].FunctionNumber,
                        0
                        );

    if (Devices[Index].MinimumLinkSpeed != Ignore) {
      Status = GetPciExpressDeviceLinkSpeed (ProtocolList[Index], &DeviceLinkSpeed);
      if (EFI_ERROR (Status)) {
        DeviceCheckResult[Index].LinkSpeedResult.ActualSpeed = Unknown;

        // Log to telemetry that an error prevented an unignored link speed from being read
        LogTelemetry (
          Devices[Index].IsFatal,
          NULL,
          (EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR),
          &gDeviceSpecificBusInfoLibTelemetryGuid,
          NULL,
          AdditionalData1,
          *((UINT64 *) Devices[Index].DeviceName)
          );
      } else {
        DeviceCheckResult[Index].LinkSpeedResult.ActualSpeed = DeviceLinkSpeed;
        if (Devices[Index].MinimumLinkSpeed <= DeviceLinkSpeed && DeviceLinkSpeed != Unknown) {
          DeviceCheckResult[Index].LinkSpeedResult.MinimumSatisfied = TRUE;
        } else {
          // Log to telemetry that a specified minimum link speed was not satisfied
          LogTelemetry (
            Devices[Index].IsFatal,
            NULL,
            (EFI_IO_BUS_PCI | EFI_IOB_EC_NOT_SUPPORTED),
            &gDeviceSpecificBusInfoLibTelemetryGuid,
            NULL,
            AdditionalData1,
            *((UINT64 *) Devices[Index].DeviceName)
            );
        }
      }
    }

    if (DeviceCheckResult[Index].DevicePresent == FALSE) {
      LogTelemetry (
        Devices[Index].IsFatal,
        NULL,
        (EFI_IO_BUS_PCI | EFI_IOB_EC_NOT_DETECTED),
        &gDeviceSpecificBusInfoLibTelemetryGuid,
        NULL,
        AdditionalData1,
        *((UINT64 *) Devices[Index].DeviceName)
        );
    }
  }
  ProcessPciDeviceResults (NumDevices, DeviceCheckResult);

Cleanup:
  if (DeviceCheckResult != NULL) {
    FreePool (DeviceCheckResult);
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
  PerformPciChecks ();

  return EFI_SUCCESS;
}
