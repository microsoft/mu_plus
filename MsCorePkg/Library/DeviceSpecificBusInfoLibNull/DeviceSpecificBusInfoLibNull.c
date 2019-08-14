/** @file 
DeviceSpecificBusInfoLibNull.c

Implements the DeviceSpecificBusInfoLib.h header to provide the CheckHardwareConnected.c file with
the pci bus info for pci devices which the user wants to verify are connected at boot

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Uefi.h>

#include <Library/DeviceSpecificBusInfoLib.h>

/**
  Returns the number of DEVICE_PCI_INFO pointers within allocated array

  Within the function *DevicesArray SHOULD BE ALLOCATED via BootServices. The allocated array
  WILL BE FREED BY THE CALLER The only data within the pool should be pointers to 
  DEVICE_PCI_INFO structs

  The DEVICE_PCI_INFO structs within the pool SHOULD BE STATIC because
  they WILL NOT BE FREED by the caller if allocated

  @param[in]        DevicesArray  Pointer to an array of DEVICE_PCI_INFO* which
                                  should be allocated within this function and populated with
                                  pointers to static DEVICE_PCI_INFO structs

  @retval           UINTN         Number of DEVICE_PCI_INFO* within the allocated array

**/
UINTN
GetPciCheckDevices(
  IN OUT DEVICE_PCI_INFO ***DevicesArray
  )
{
  return 0;
}