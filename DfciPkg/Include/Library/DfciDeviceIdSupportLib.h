/** @file
DfciDeviceIdSupportLib - Library supports getting the device Id elements.


Copyright (C) 2016 Microsoft Corporation. All Rights Reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

**/

#ifndef __DFCI_DEVICE_ID_SUPPORT_LIB_H__
#define __DFCI_DEVICE_ID_SUPPORT_LIB_H__

typedef struct {
    CHAR8   *Manufacturer;
    UINTN    ManufacturerSize;
    CHAR8   *ProductName;
    UINTN    ProductNameSize;
    CHAR8   *SerialNumber;
    UINTN    SerialNumberSize;
    CHAR8   *Uuid;
    UINTN    UuidSize;
} DFCI_DEVICE_ID_ELEMENTS;

// *-------------------------------------------------------------------*
//  TO DO - Remove this fucntion when V2 packet support is complete    *
// *-------------------------------------------------------------------*
/**
Gets the serial number for this device.

@para[out] SerialNumber - UINTN value of serial number

@return EFI_SUCCESS - SerialNumber has been updated to equal the serial number of the device
@return EFI_ERROR   - Error getting number

**/
EFI_STATUS
EFIAPI
GetSerialNumber(
  OUT UINTN*  SerialNumber
  );

/**
 *
 * Get the Device Id elements for the Dfci Packets
 *
 * @param DeviceId   Place to store a pointer to the DeviceId elements
 *
 * @return EFI_STATUS  EFI_SUCCESS - DeviceId elements is populated
 */
EFI_STATUS
EFIAPI
DfciSupportGetDeviceId (
  OUT DFCI_DEVICE_ID_ELEMENTS **DeviceId
  );


#endif //__DFCI_DEVICE_ID_SUPPORT_LIB_H__
