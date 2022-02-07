/** @file
DfciDeviceIdSupportLib.h

Library supports getting the device Id elements.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DFCI_DEVICE_ID_SUPPORT_LIB_H__
#define __DFCI_DEVICE_ID_SUPPORT_LIB_H__

// *-------------------------------------------------------------------*
// * This function is now DEPRECATED.  Always return 0                 *
// *-------------------------------------------------------------------*

/**
Gets the serial number for this device.

@para[out] SerialNumber - UINTN value of serial number

@return EFI_SUCCESS - SerialNumber has been updated to equal the serial number of the device
@return EFI_ERROR   - Error getting number

**/
EFI_STATUS
EFIAPI
DfciIdSupportV1GetSerialNumber (
  OUT UINTN  *SerialNumber
  );

/**
 * Get the Manufacturer Name
 *
 * @param Manufacturer
 * @param ManufacturerSize
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DfciIdSupportGetManufacturer (
  CHAR8  **Manufacturer,
  UINTN  *ManufacturerSize  OPTIONAL
  );

/**
 *
 * Get the Product Name
 *
 * @param ProductName
 * @param ProductNameSize
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DfciIdSupportGetProductName (
  CHAR8  **ProductName,
  UINTN  *ProductNameSize  OPTIONAL
  );

/**
 * Get the SerialNumber
 *
 * @param SerialNumber
 * @param SerialNumberSize
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DfciIdSupportGetSerialNumber (
  CHAR8  **SerialNumber,
  UINTN  *SerialNumberSize  OPTIONAL
  );

#endif //__DFCI_DEVICE_ID_SUPPORT_LIB_H__
