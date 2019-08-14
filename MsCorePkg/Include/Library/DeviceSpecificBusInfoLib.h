/** @file 
DeviceSpecificBusInfoLib.h

An interface for creators to define PCI devices for which they want to check at boot.
Simply create a DEVICE_PCI_INFO struct for every device you want read, and a not found error
will be logged if it is not read. In cases where the device won't boot to the OS, this can
quickly identify if the cause is a PCI device not being detected.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/**
    CHAR8       DeviceName[8]
    BOOLEAN     IsFatal
    UINTN       SegmentNumber
    UINTN       BusNumber
    UINTN       DeviceNumber
    UINTN       FunctionNumber
**/
typedef struct DEVICE_PCI_INFO
{ 
    CHAR8                 DeviceName[8]; //So it fits within the 64 bits of Additional Code 2 in section data
    BOOLEAN               IsFatal;
    UINTN                 SegmentNumber;
    UINTN                 BusNumber;
    UINTN                 DeviceNumber;
    UINTN                 FunctionNumber;
} DEVICE_PCI_INFO;

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
  );
