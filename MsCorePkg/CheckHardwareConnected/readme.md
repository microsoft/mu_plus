# CheckHardwareConnected

## About
This driver determines at boot if the specified pci devices found in the DeviceSpecificBusInfoLib library are properly connected

## Usage
To employ this driver, simply build it and supply the DeviceSpecificBusInfoLib which implements the DeviceSpecificBusInfoLib.h interface.

---------------

The info for each PCI device will be contained within the DEVICE_PCI_INFO struct which contains fields:

**DeviceName:** A friendly name for the device. This Ascii name will be contained within the AdditionalInfo2 field of the MU_TELEMETRY_CPER_SECTION_DATA telemetry struct.

**IsFatal:** A boolean which if true states that the pci device being absent crashes the device upon OS boot

**SegmentNumber**, **BusNumber**, **DeviceNumber**, **FunctionNumber :** Info required to locate the PCI device

**MinimumLinkSpeed** The minimum link speed expected for the PCI device

---------------

The library interface consists of two functions:

**GetPciCheckDevices() :** Populates an array of pointers to DEVICE_PCI_INFO structs. The pointer to the unallocated
array is passed in as an argument and should be allocated within the function. The DEVICE_PCI_INFO structs should be
global variables in the library and the array should contain their addresses. Function returns the number of
DEVICE_PCI_INFO struct pointers within the allocated array.

**ProcessPciDeviceResults() :** Accepts an array of PCI device check results in the form of a pointer to a type of
DEVICE_PCI_CHECK_RESULT and performs custom actions based upon the results.

If there are specific cases when you do not want to check for certain PCI devices (such as when a device has been
purposefully disabled), simply exclude the DEVICE_PCI_INFO associated with that device when allocating and returning
the array.

---------------

## Copyright
Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
