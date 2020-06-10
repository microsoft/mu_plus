/** @file
DeviceSpecificBusInfoLibNull.c

An interface for platforms to define PCI devices which is checked
at boot. Simply create an array of DEVICE_PCI_INFO structures for every
device desired, and an error will be logged if it is not found on the bus. In
cases where the device won't boot to the OS, this can help quickly identify
if the cause is due to a PCI device not being detected.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Uefi.h>

#include <Library/DeviceSpecificBusInfoLib.h>

/**
  Returns a pointer to a static array of DEVICE_PCI_INFO structures and the length of the
  array.

  @param[out]       DevicesArray  Pointer to the head of an array of DEVICE_PCI_INFO structures.
                                  The caller shall not free this array.

  @retval           UINTN         Length of the returned array.

**/
UINTN
GetPciCheckDevices(
  OUT DEVICE_PCI_INFO **DevicesArray
  )
{
  return 0;
}

/**
  Performs custom actions in response to the given results from the PCI device check.

  @param[in]        ResultCount   The number of elements in the Results array.
  @param[in]        Results       An array of DEVICE_PCI_CHECK_RESULT elements where each index directly corresponds
                                  to the device index in the array passed to GetPciCheckDevices (). TRUE indicates
                                  the device at that index is present. FALSE indicates the device at that index
                                  is not present.
**/
VOID
ProcessPciDeviceResults (
  IN  UINTN                       ResultCount,
  IN  DEVICE_PCI_CHECK_RESULT     *Results
  )
{
  return;
}
