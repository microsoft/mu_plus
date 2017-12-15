/** @file
DfciSerialNumberSupportLib - Library supports getting the device serial number.


Copyright (C) 2016 Microsoft Corporation. All Rights Reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

**/

#ifndef __DFCI_SERIAL_NUMBER_SUPPORT_LIB_H__
#define __DFCI_SERIAL_NUMBER_SUPPORT_LIB_H__

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


#endif //__DFCI_SERIAL_NUMBER_SUPPORT_LIB_H__
